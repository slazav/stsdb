/*  Command-line interface for the Graphene time series database.
*/

#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <dirent.h>
#include <sys/stat.h>

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
  string dpolicy;      /* what to do with duplicated timestamps*/
  vector<string> pars; /* non-option parameters */
  DBpool pool;         /* database storage */

  // defaults
  Pars(){
    dbpath  = "/var/lib/graphene/";
    dpolicy = "replace";
  }

  // print help message and exit
  void print_help(){
    Pars p; // default parameters
    cout << "graphene -- command line interface to Graphene time series database\n"
            "Usage: graphene [options] <command> <parameters>\n"
            "Options:\n"
            "  -d <path> -- database directory (default " << p.dbpath << "\n"
            "  -D <word> -- what to do with duplicated timestamps:\n"
            "               replace, skip, error, sshift, nsshift (default: " << p.dpolicy << ")\n"
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
            "  del_range <name> <time1> <time2>\n"
            "      -- delete all points in the time range\n"
            "  interactive\n"
            "      -- interactive mode, commands are read from stdin\n"
            "  sync -- close all opened databases in interactive mode\n"
    ;
    throw Err();
  }

  // get options and parameters from argc/argv
  void parse_cmdline_options(const int argc, char **argv){
    /* parse  options */
    int c;
    while((c = getopt(argc, argv, "+d:D:h"))!=-1){
      switch (c){
        case '?':
        case ':': throw Err(); /* error msg is printed by getopt*/
        case 'd': dbpath = optarg; break;
        case 'D': dpolicy = optarg; break;
        case 'h': print_help();
      }
    }
    pars = vector<string>(argv+optind, argv+argc);
    if (pars.size() < 1) print_help();
  }

  // get parameters from a string (for interactive mode)
  void parse_command_string(const string & str){
    istringstream in(str);
    pars.clear();
    while (1) {
      string a;
      in >> a;
      if (!in) break;
      pars.push_back(a);
    }
  }

  // Run command, using parameters
  // For read/write commands time is transferred as a string
  // to db.put, db.get_* functions without change.
  // "now", "now_s" and "inf" strings can be used.
  void run_command(){
    if (pars.size() < 1) return;
    string cmd = pars[0];

    // create new database
    // args: create <name> [<data_fmt>] [<description>]
    if (strcasecmp(cmd.c_str(), "create")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>4) throw Err() << "too many parameters";
      DBinfo info(
         pars.size()<3 ? DEFAULT_DATAFMT : DBinfo::str2datafmt(pars[2]),
         pars.size()<4 ? "" : pars[3] );
      // todo: create folders if needed
      DBgr db(dbpath, pars[1], DB_CREATE | DB_EXCL);
      db.write_info(info);
      return;
    }

    // delete a database
    // args: delete <name>
    if (strcasecmp(cmd.c_str(), "delete")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>2) throw Err() << "too many parameters";
      string name = check_name(pars[1]); // name should be always checked!
      pool.clear();
      int res = remove((dbpath + "/" + name + ".db").c_str());
      if (res) throw Err() << name <<  ".db: " << strerror(errno);
      return;
    }

    // rename a database
    // args: rename <old_name> <new_name>
    if (strcasecmp(cmd.c_str(), "rename")==0){
      if (pars.size()<3) throw Err() << "database old and new names expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string name1 = check_name(pars[1]);
      string name2 = check_name(pars[2]);
      string path1 = dbpath + "/" + name1 + ".db";
      string path2 = dbpath + "/" + name2 + ".db";
      // check if destination exists
      struct stat buf;
      int res = stat(path2.c_str(), &buf);
      if (res==0) throw Err() << "can't rename database, destination exists: " << name2 << ".db";
      // do rename
      pool.clear();
      res = rename(path1.c_str(), path2.c_str());
      if (res) throw Err() << "can't rename database: " << strerror(errno);
      return;
    }

    // change database description
    // args: set_descr <name> <description>
    if (strcasecmp(cmd.c_str(), "set_descr")==0){
      if (pars.size()<3) throw Err() << "database name and new description text expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      DBgr db = pool.get(dbpath, pars[1]);
      DBinfo info = db.read_info();
      info.descr = pars[2];
      db.write_info(info);
      return;
    }

    // print database info
    // args: info <name>
    if (strcasecmp(cmd.c_str(), "info")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>2) throw Err() << "too many parameters";
      DBgr db = pool.get(dbpath, pars[1], DB_RDONLY);
      DBinfo info = db.read_info();
      cout << DBinfo::datafmt2str(info.val);
      if (info.descr!="") cout << '\t' << info.descr;
      cout << "\n";
      return;
    }

    // print database list
    // args: list
    if (strcasecmp(cmd.c_str(), "list")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      DIR *dir = opendir(dbpath.c_str());
      if (!dir) throw Err() << "can't open database directory: " << strerror(errno);
      struct dirent *ent;
      while ((ent = readdir (dir)) != NULL) {
        string name(ent->d_name);
        size_t p = name.find(".db");
        if (name.size()>3 && p == name.size()-3)
          cout << name.substr(0,p) << "\n";
      }
      closedir(dir);
      return;
    }

    // write data
    // args: put <name> <time> <value1> ...
    if (strcasecmp(cmd.c_str(), "put")==0){
      if (pars.size()<4) throw Err() << "database name, timstamp and some values expected";
      vector<string> dat;
      for (int i=3; i<pars.size(); i++) dat.push_back(string(pars[i]));
      // open database and write data
      DBgr db = pool.get(dbpath, pars[1]);
      db.put(pars[2], dat, dpolicy);
      return;
    }

    // get next point after time1
    // args: get_next <name>[:N] [<time1>]
    if (strcasecmp(cmd.c_str(), "get_next")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t1 = pars.size()>2? pars[2]: "0";
      DBout dbo(dbpath, pars[1]);
      DBgr db = pool.get(dbpath, dbo.name, DB_RDONLY);
      db.get_next(t1, dbo);
      return;
    }

    // get previous point before time2
    // args: get_prev <name>[:N] [<time2>]
    if (strcasecmp(cmd.c_str(), "get_prev")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t2 = pars.size()>2? pars[2]: "inf";
      DBout dbo(dbpath, pars[1]);
      DBgr db = pool.get(dbpath, dbo.name, DB_RDONLY);
      db.get_prev(t2, dbo);
      return;
    }

    // get previous or interpolated point for the time
    // args: get <name>[:N] <time>
    if (strcasecmp(cmd.c_str(), "get")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t2 = pars.size()>2? pars[2]: "inf";
      DBout dbo(dbpath, pars[1]);
      DBgr db = pool.get(dbpath, dbo.name, DB_RDONLY);
      db.get(t2, dbo);
      return;
    }

    // get data range
    // args: get_range <name>[:N] [<time1>] [<time2>] [<dt>]
    if (strcasecmp(cmd.c_str(), "get_range")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>5) throw Err() << "too many parameters";
      string t1 = pars.size()>2? pars[2]: "0";
      string t2 = pars.size()>3? pars[3]: "inf";
      string dt = pars.size()>4? pars[4]: "0";
      DBout dbo(dbpath, pars[1]);
      DBgr db = pool.get(dbpath, dbo.name, DB_RDONLY);
      db.get_range(t1,t2,dt, dbo);
      return;
    }

    // delete one data point
    // args: del <name> <time>
    if (strcasecmp(cmd.c_str(), "del")==0){
      if (pars.size()<3) throw Err() << "database name and time expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      DBgr db = pool.get(dbpath, pars[1]);
      db.del(pars[2]);
      return;
    }

    // delete all points in the data range
    // args: del_range <name> <time1> <time2>
    if (strcasecmp(cmd.c_str(), "del_range")==0){
      if (pars.size()<4) throw Err() << "database name and two times expected";
      if (pars.size()>4) throw Err() << "too many parameters";
      DBgr db = pool.get(dbpath, pars[1]);
      db.del_range(pars[2],pars[3]);
      return;
    }

    // interactive mode: put/get data using stdin commands
    // args: interactive
    if (strcasecmp(cmd.c_str(), "interactive")==0){
      if (pars.size()>1) throw Err() << "too many parameters";

      string line;
      while (getline(cin, line)){
        try {
          parse_command_string(line);
          if (pars.size()>0 &&
              strcasecmp(pars[0].c_str(), "interactive")==0)
            throw Err() << "Command can not be run in interactive mode";
          run_command();
          cout << "OK\n";
        }
        catch(Err e){
          if (e.str()!="") cout << "Error: " << e.str() << "\n";
        }
      }
      return;
    }

    // close all opened databases in interactive mode
    // args: sync
    if (strcasecmp(cmd.c_str(), "sync")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      pool.clear();
      return;
    }

    // unknown command
    throw Err() << "Unknown command: " << cmd;
  }
};


/**********************************************************/
int
main(int argc, char **argv) {

  try {
    Pars p;  /* program parameters */
    p.parse_cmdline_options(argc, argv);
    p.run_command();

  } catch(Err e){
    if (e.str()!="") cout << "Error: " << e.str() << "\n";
    return 1;
  }
}