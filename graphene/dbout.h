/* DBout class: data output: columns, filters, tables */
#ifndef GRAPHENE_DBOUT_H
#define GRAPHENE_DBOUT_H

#include <string>
#include <iostream>

#include "data.h"

/***********************************************************/
// Class for an extended dataset object.
//
// Extended dataset can be just a database name, but it can also
// contain a column, or a filter:
// <name>:<column>
// <name>:<filter>
//
class DBout {
  public:

  bool spp;    // SPP mode (protect # in the beginning of line)

  std::ostream & out;  // stream for output
  int col; // column number, for the main database

  int flt; // filter number 1..MAX_FILTERS-1; <1 for no filtering

  // constructor -- parse the dataset string, create iostream
  DBout(std::ostream & out_ = std::cout):
          col(-1), flt(-1), out(out_), spp(false) {}

  // print_point  -- by default it just prints the line to out,
  // but this function can be overriden.
  virtual void print_point(const std::string & str){
    if (spp) out << graphene_spp_text(str);
    else out << str;
  }

};

// version with string output (for HTTP get interface)
class DBoutS: public DBout {
  std::string mystr;
  public:

  void print_point(const std::string & str){ mystr += str; }
  std::string & get_str() {return mystr;}

};


#endif
