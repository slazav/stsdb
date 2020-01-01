/* DBinfo class: information about databases and function for
   packing/unpacking/interpolating raw database records.
*/

#ifndef GRAPHENE_DBINFO_H
#define GRAPHENE_DBINFO_H

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>
#include "err/err.h"

// current database version
#define DBVERSION 2

// bercleydb:
//  http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
//  https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/reftoc.html

/***********************************************************/
// Enum for the data format
enum DataFMT { DATA_TEXT,
         DATA_INT8, DATA_UINT8, DATA_INT16, DATA_UINT16,
         DATA_INT32, DATA_UINT32, DATA_INT64, DATA_UINT64,
         DATA_FLOAT, DATA_DOUBLE};

/// default value
const DataFMT DEFAULT_DATAFMT = DATA_DOUBLE;

// last values -- for loops and array dimensions
const DataFMT LAST_DATAFMT = DATA_DOUBLE;

// string names of data formats
const std::string data_fmt_names[LAST_DATAFMT+1] =
  {"TEXT", "INT8", "UINT8", "INT16", "UINT16",
   "INT32", "UINT32", "INT64", "UINT64", "FLOAT", "DOUBLE"};

// sizes of data elements, bytes
const static size_t data_fmt_sizes[LAST_DATAFMT+1] =
      {1,1,1,2,2,4,4,8,8,4,8}; // bytes

/************************************/
// Check database or filter name
// All names (not only for reading/writing, but
// also for moving or deleting should be checked).
void check_name(const std::string & name);

/***********************************************************/
// Class for the database information.
class DBinfo {
  public:
  DataFMT val;
  uint8_t  version;
  std::string descr;

  DBinfo(const DataFMT v = DATA_DOUBLE,
         const std::string &d = std::string())
            : val(v),descr(d),version(DBVERSION) {}

  // return size and name of the data format
  size_t dsize() const { return data_fmt_sizes[val]; }
  std::string dname() const { return data_fmt_names[val]; }

  // convert string into enum member
  static DataFMT str2datafmt(const std::string & s){
    for (int i = 0; i<=LAST_DATAFMT; i++)
      if (strcasecmp(s.c_str(),data_fmt_names[i].c_str())==0)
        return static_cast<DataFMT>(i);
    throw Err() << "Unknown data format: " << s;
  }
  // ...and back
  static std::string datafmt2str(const DataFMT s){
    return data_fmt_names[s]; }

  bool operator==(const DBinfo &o) const{
    return o.val==val && o.descr==descr && o.version==version; }

  // We keep time in a std::string storage, which
  // can be easily converted into Berkleydb data.
  // It is not a c-string!


  // Parse timestamp from a string
  std::string parse_time(const std::string & ts) const;

  // Print timestamp
  std::string print_time(const std::string & s) const;

  // Compare two packed time values, return +1,0,-1 if s1>s2,s1=s2,s1<s2
  int cmp_time(const std::string & s1, const std::string & s2) const;

  // Is time equals zero?
  bool is_zero_time(const std::string & s1) const;

  // Add two packed time values, return packed string
  std::string add_time(const std::string & s1, const std::string & s2) const;

  // Subtract two packed time values, return number of seconds as a double value
  double time_diff(const std::string & s1, const std::string & s2) const;

  // Pack data according with data format
  // std::string is used as a convenient data storage, which
  // can be easily converted into Berkleydb data.
  // Output string is not a c-string!
  std::string parse_data(const std::vector<std::string> & strs) const;

  // Print data
  std::string print_data(const std::string & s, const int col=-1) const;

  // interpolate data (for FLOAT and DOUBLE values)
  // k0,k1,k2,v1,v2 are packed strings!
  std::string interpolate(
        const std::string & k0,
        const std::string & k1, const std::string & k2,
        const std::string & v1, const std::string & v2);

  // Version-specific functions - use only inside object or in tests
//  private:
  // Pack/Unpack integer timestamp
  std::string pack_time_v1(const uint64_t t) const;
  uint64_t unpack_time_v1(const std::string & s) const;

  std::string parse_time_v1(const std::string & ts) const;
  std::string print_time_v1(const std::string & s) const;
  int cmp_time_v1(const std::string & s1, const std::string & s2) const;
  bool is_zero_time_v1(const std::string & s1) const;
  std::string add_time_v1(const std::string & s1, const std::string & s2) const;
  double time_diff_v1(const std::string & s1, const std::string & s2) const;
  std::string interpolate_v1(
        const std::string & k0,
        const std::string & k1, const std::string & k2,
        const std::string & v1, const std::string & v2);

  // Pack/Unpack timestamp
  std::string pack_time_v2(const uint64_t t) const;
  uint64_t unpack_time_v2(const std::string & s) const;

  std::string parse_time_v2(const std::string & ts) const;
  std::string print_time_v2(const std::string & s) const;
  int cmp_time_v2(const std::string & s1, const std::string & s2) const;
  bool is_zero_time_v2(const std::string & s1) const;
  std::string add_time_v2(const std::string & s1, const std::string & s2) const;
  double time_diff_v2(const std::string & s1, const std::string & s2) const;
  std::string interpolate_v2(
        const std::string & k0,
        const std::string & k1, const std::string & k2,
        const std::string & v1, const std::string & v2);

};


#endif
