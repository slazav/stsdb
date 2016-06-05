/*  Command-line interface for the Simple time series database.
*/

#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <sys/time.h>

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include "db.h"
#include "dbout.h"

using namespace std;

/**********************************************************/
/* global parameters */
class Pars{
  public:
  string dbpath;       /* path to the databases */
  vector<string> pars; /* non-option parameters */

  // defaults
  Pars(){
    dbpath = "/var/lib/stsdb/"; 
  }

  // print help message and exit
  void print_help(){
    Pars p; // default parameters
    cout << "stsdb -- command line interface to Simple Time Series Database\n"
            "Usage: stsdb [options] <command> <parameters>\n"
            "Options:\n"
            "  -d <path> -- database directory (default " << p.dbpath << "\n"
            "  -h        -- write this help message and exit\n"
            "Comands:\n"
            "  create <name> <data_fmt> <description>\n"
            "      -- create a database\n"
            "  delete <name>\n"
            "      -- delete a database\n"
            "  rename <old_name> <new_name>\n"
            "      -- rename a database\n"
            "  set_descr <name> <description>\n"
            "      -- change database description\n"
            "  info <name>\n"
            "      -- print database information, tab-separated time format,\n"
            "         data format and description (if it is not empty)\n"
            "  list\n"
            "      -- list all databases in the data folder\n"
            "  put <name> <time> <value1> ... <valueN>\n"
            "      -- write a data point\n"
            "  get <name>[:N] <time>\n"
            "      -- get previous or interpolated point\n"
            "  get_next <name>[:N] [<time1>]\n"
            "      -- get next point after time1\n"
            "  get_prev <name>[:N] [<time2>]\n"
            "      -- get previous point before time2\n"
            "  get_range <name>[:N] [<time1>] [<time2>] [<dt>]\n"
            "      -- get points in the time range\n"
            "  del <name> <time>\n"
            "      -- delete one data point\n"
            "  del_range <name> [<time1>] [<time2>]\n"
            "      -- delete all points in the time range\n"
    ;
    throw Err();
  }

  // parse options, modify argc/argv
  void parse_cmdline_options(const int argc, char **argv){
    /* parse  options */
    int c;
    while((c = getopt(argc, argv, "+d:h"))!=-1){
      switch (c){
        case '?':
        case ':': throw Err(); /* error msg is printed by getopt*/
        case 'd': dbpath = optarg; break;
        case 'h': print_help();
      }
    }
    pars = vector<string>(argv+optind, argv+argc);
  }

};

/**********************************************************/
// Current time in ms
uint64_t
prectime(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (uint64_t)tv.tv_usec/1000
         + (uint64_t)tv.tv_sec*1000;
}

/**********************************************************/
// Read timestamp from a string.
// String "now" means current time.
uint64_t
str2time(const string & str) {
  if (strcasecmp(str.c_str(), "now")==0) return prectime();
  istringstream s(str);
  uint64_t t;  s >> t;
  if (s.bad() || s.fail() || !s.eof())
  throw Err() << "Not a timestamp: " << str;
  return t;
}

/**********************************************************/
int
main(int argc, char **argv) {

  try {

    Pars p;  /* program parameters */
    p.parse_cmdline_options(argc, argv);


    if (p.pars.size() < 1) p.print_help();
    string cmd = p.pars[0];

    // create new database
    // args: create <name> [<data_fmt>] [<description>]
    if (strcasecmp(cmd.c_str(), "create")==0){
      if (p.pars.size()<2) throw Err() << "database name expected";
      if (p.pars.size()>4) throw Err() << "too many parameters";
      DBinfo info(
         p.pars.size()<3 ? DEFAULT_DATAFMT : DBinfo::str2datafmt(p.pars[2]),
         p.pars.size()<4 ? "" : p.pars[3] );
      DBsts db(p.dbpath, p.pars[1], DB_CREATE | DB_EXCL);
      db.write_info(info);
      return 0;
    }

    // delete a database
    // args: delete <name>
    if (strcasecmp(cmd.c_str(), "delete")==0){
      if (p.pars.size()<2) throw Err() << "database name expected";
      if (p.pars.size()>2) throw Err() << "too many parameters";
      string name = check_name(p.pars[1]); // name should be always checked!
      int res = remove((p.dbpath + "/" + name + ".db").c_str());
      if (res) throw Err() << name <<  ".db: " << strerror(errno);
      return 0;
    }

    // rename a database
    // args: rename <old_name> <new_name>
    if (strcasecmp(cmd.c_str(), "rename")==0){
      if (p.pars.size()<3) throw Err() << "database old and new names expected";
      if (p.pars.size()>3) throw Err() << "too many parameters";
      string name1 = check_name(p.pars[1]);
      string name2 = check_name(p.pars[2]);
      string path1 = p.dbpath + "/" + name1 + ".db";
      string path2 = p.dbpath + "/" + name2 + ".db";
      // check if destination exists
      struct stat buf;
      int res = stat(path2.c_str(), &buf);
      if (res==0) throw Err() << "can't rename database, destination exists: " << name2 << ".db";
      // do rename
      res = rename(path1.c_str(), path2.c_str());
      if (res) throw Err() << "can't rename database: " << strerror(errno);
      return 0;
    }

    // change database description
    // args: set_descr <name> <description>
    if (strcasecmp(cmd.c_str(), "set_descr")==0){
      if (p.pars.size()<3) throw Err() << "database name and new description text expected";
      if (p.pars.size()>3) throw Err() << "too many parameters";
      DBsts db(p.dbpath, p.pars[1], 0);
      DBinfo info = db.read_info();
      info.descr = p.pars[2];
      db.write_info(info);
      return 0;
    }

    // print database info
    // args: info <name>
    if (strcasecmp(cmd.c_str(), "info")==0){
      if (p.pars.size()<2) throw Err() << "database name expected";
      if (p.pars.size()>2) throw Err() << "too many parameters";
      DBsts db(p.dbpath, p.pars[1], DB_RDONLY);
      DBinfo info = db.read_info();
      cout << DBinfo::datafmt2str(info.val);
      if (info.descr!="") cout << '\t' << info.descr;
      cout << "\n";
      return 0;
    }

    // print database list
    // args: list
    if (strcasecmp(cmd.c_str(), "list")==0){
      if (p.pars.size()>1) throw Err() << "too many parameters";
      DIR *dir = opendir(p.dbpath.c_str());
      if (!dir) throw Err() << "can't open database directory: " << strerror(errno);
      struct dirent *ent;
      while ((ent = readdir (dir)) != NULL) {
        string name(ent->d_name);
        size_t p = name.find(".db");
        if (name.size()>3 && p == name.size()-3)
          cout << name.substr(0,p) << "\n";
      }
      closedir(dir);
      return 0;
    }

    // write data
    // args: put <name> <time> <value1> ...
    if (strcasecmp(cmd.c_str(), "put")==0){
      if (p.pars.size()<4) throw Err() << "database name, timstamp and some values expected";
      uint64_t t = str2time(p.pars[2]);
      vector<string> dat;
      for (int i=3; i<p.pars.size(); i++) dat.push_back(string(p.pars[i]));
      // open database and write data
      DBsts db(p.dbpath, p.pars[1], 0);
      db.put(t, dat);
      return 0;
    }

    // get next point after time1
    // args: get_next <name>[:N] [<time1>]
    if (strcasecmp(cmd.c_str(), "get_next")==0){
      if (p.pars.size()<2) throw Err() << "database name expected";
      if (p.pars.size()>3) throw Err() << "too many parameters";
      uint64_t t = p.pars.size()>2? str2time(p.pars[2]): 0;
      DBout dbo(p.dbpath, p.pars[1]);
      DBsts db(p.dbpath, dbo.name, DB_RDONLY);
      db.get_next(t, dbo);
      return 0;
    }

    // get previous point before time2
    // args: get_prev <name>[:N] [<time2>]
    if (strcasecmp(cmd.c_str(), "get_prev")==0){
      if (p.pars.size()<2) throw Err() << "database name expected";
      if (p.pars.size()>3) throw Err() << "too many parameters";
      uint64_t t2 = p.pars.size()>2? str2time(p.pars[2]): -1;
      DBout dbo(p.dbpath, p.pars[1]);
      DBsts db(p.dbpath, dbo.name, DB_RDONLY);
      db.get_prev(t2, dbo);
      return 0;
    }

    // get prefious or interpolated point for the time
    // args: get <name>[:N] <time>
    if (strcasecmp(cmd.c_str(), "get")==0){
      if (p.pars.size()<2) throw Err() << "database name expected";
      if (p.pars.size()>3) throw Err() << "too many parameters";
      uint64_t t2 = p.pars.size()>2? str2time(p.pars[2]): -1;
      DBout dbo(p.dbpath, p.pars[1]);
      DBsts db(p.dbpath, dbo.name, DB_RDONLY);
      db.get(t2, dbo);
      return 0;
    }

    // get data range
    // args: get_range <name>[:N] [<time1>] [<time2>] [<dt>]
    if (strcasecmp(cmd.c_str(), "get_range")==0){
      if (p.pars.size()<2) throw Err() << "database name expected";
      if (p.pars.size()>5) throw Err() << "too many parameters";
      uint64_t t1 = p.pars.size()>2? str2time(p.pars[2]): 0;
      uint64_t t2 = p.pars.size()>3? str2time(p.pars[3]): -1;
      uint64_t dt = p.pars.size()>4? str2time(p.pars[4]): 0;
      DBout dbo(p.dbpath, p.pars[1]);
      DBsts db(p.dbpath, dbo.name, DB_RDONLY);
      db.get_range(t1,t2,dt, dbo);
      return 0;
    }

    // delete one data point
    // args: del <name> <time>
    if (strcasecmp(cmd.c_str(), "del")==0){
      if (p.pars.size()<3) throw Err() << "database name and time expected";
      if (p.pars.size()>3) throw Err() << "too many parameters";
      uint64_t t = str2time(p.pars[2]);
      DBsts db(p.dbpath, p.pars[1], 0);
      db.del(t);
      return 0;
    }

    // delete all points in the data range
    // args: del_range <name> [<time1>] [<time2>]
    if (strcasecmp(cmd.c_str(), "del_range")==0){
      if (p.pars.size()<4) throw Err() << "database name and two times expected";
      if (p.pars.size()>4) throw Err() << "too many parameters";
      uint64_t t1 = str2time(p.pars[2]);
      uint64_t t2 = str2time(p.pars[3]);
      DBsts db(p.dbpath, p.pars[1], 0);
      db.del_range(t1,t2);
      return 0;
    }

    // interactive mode: put/get data using stdin commands
    // args: interactive
/*
    if (strcasecmp(cmd.c_str(), "interactive")==0){
      if (p.pars.size()>1) throw Err() << "too many parameters";
      while (!cin.eof()){
        string line;
        cin.getline(line);
        istringstream lin(line)

        map<string, DBsts> idb, odb;
        // read command
        std::string cmd;
        lin >> cmd;
        if (cmd == "put" || cmd == "PUT"){
        }
        if (cmd == "get_prev" || cmd == "GET_PREV"){
        }

      }
    }
*/

    throw Err() << "Unknown command";

  } catch(Err e){
    if (e.str()!="") cout << "Error: " << e.str() << "\n";
    return 1;
  }
}
