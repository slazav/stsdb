#include <iostream>
#include <vector>
#include <string>

#include "err/err.h"
#include "err/assert_err.h"

#include "dbout.h"
#include "gr_db.h"

/*
std::string
proc_point_str(const std::string & input, const DBinfo & info){

  // split input
  std::istringstream in(input);
  std::vector<std::string> args;
  std::string key;
  in >> key;
  while (1) {
    std::string a;
    in >> a;
    if (!in) break;
    args.push_back(a);
  }

  std::ostringstream out;
  DBout dbo("", "dbname", out);

  // pack input
  std::string kp = graphene_time_parse(key, info.ttype);
  std::string vp = graphene_data_parse(args, info.dtype);
  DBT k = GrapheneDB::mk_dbt(kp);
  DBT v = GrapheneDB::mk_dbt(vp);

  dbo.proc_point(&k,&v, info);
  return out.str();
}
*/

using namespace std;
int main() {
  try{
/***************************************************************/
    // FMT object, data and time formats

/*
    {  // check names and extract column numbers
      check_name("abc");
      std::string e1("symbols '.:+| \\n\\t/' are not allowed in the database name: ");
      assert_err(check_name("abc/def"), e1+"abc/def");
      assert_err(check_name("./abc/def" ), e1+"./abc/def" );
      assert_err(check_name("/abc/def:1"), e1+"/abc/def:1");
      assert_err(check_name("/abc/def+1"), e1+"/abc/def+1");
      assert_err(check_name("/abc/def 1"), e1+"/abc/def 1");
      assert_err(check_name("/abc/def\t"), e1+"/abc/def\t");
      assert_err(check_name("/abc/def\n"), e1+"/abc/def\n");

      {DBout dbn("abc",cout);    assert_eq(dbn.name, "abc");
                            assert_eq(dbn.col, -1);}
      {DBout dbn("abc_",cout);   assert_eq(dbn.name, "abc_");}

      {DBout dbn("abc:1",cout);
         assert_eq(dbn.name, "abc"); assert_eq(dbn.col, 1);}
    }

    {
      DBinfo hh1; // default constructor
      DBinfo hh2(DATA_INT16);

      assert_eq(hh1.ttype, TIME_V2);
      assert_eq(hh1.dtype, DATA_DOUBLE);
      assert_eq(hh2.ttype, TIME_V2);
      assert_eq(hh2.dtype, DATA_INT16);
    }


    {
      DBinfo hh1(DATA_DOUBLE);

      /// version 1 timestamps
      hh1.version = 1;
      hh1.ttype = TIME_V1;

      assert_eq(proc_point_str("1234567890.123000000 0.1 0.2 0.3", hh1),
                std::string("1234567890.123000000 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("0.000000000 0.1 0.2 0.3", hh1),
                "0.000000000 0.1 0.2 0.3\n");

      // largest possible value
      assert_eq(proc_point_str("18446744073709551.615 0.1 0.2 0.3", hh1),
                "18446744073709551.615000000 0.1 0.2 0.3\n");

      // same as inf
      assert_eq(proc_point_str("inf 0.1 0.2 0.3", hh1),
                "18446744073709551.615000000 0.1 0.2 0.3\n");

      // overfull
      assert_err(proc_point_str("18446744073709551.616 0.1 0.2 0.3", hh1),
                "Bad V1 timestamp: too large value: 18446744073709551.616");

      // version 1 has 1ms precision
      assert_eq(proc_point_str("1234567890.123123000 0.1 0.2 0.3", hh1),
                std::string("1234567890.123000000 0.1 0.2 0.3\n"));

      // and no rounding, extra digits are skipped
      assert_eq(proc_point_str("1234567890.123923000 0.1 0.2 0.3", hh1),
                std::string("1234567890.123000000 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("1 0.1 0.2 0.3", hh1),
                std::string("1.000000000 0.1 0.2 0.3\n"));

      assert_err(proc_point_str("-1a 0.1 0.2 0.3", hh1),
                "Bad V1 timestamp: positive value expected: -1a");

      assert_err(proc_point_str("1a 0.1 0.2 0.3", hh1),
                "Bad V1 timestamp: can't read decimal dot: 1a");

      assert_err(proc_point_str("1.2a 0.1 0.2 0.3", hh1),
                "Bad V1 timestamp: can't read milliseconds: 1.2a");


      /// version 2 timestamps
      hh1.version = 2;
      hh1.ttype = TIME_V2;

      assert_eq(proc_point_str("1234567890.000000000 0.1 0.2 0.3", hh1),
                std::string("1234567890.000000000 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("1234567890.123456789 0.1 0.2 0.3", hh1),
                std::string("1234567890.123456789 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("0.0 0.1 0.2 0.3", hh1),
                std::string("0.000000000 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("1 0.1 0.2 0.3", hh1),
                std::string("1.000000000 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("1. 0.1 0.2 0.3", hh1),
                std::string("1.000000000 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("1.0 0.1 0.2 0.3", hh1),
                std::string("1.000000000 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("1.1 0.1 0.2 0.3", hh1),
                std::string("1.100000000 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("1.001 0.1 0.2 0.3", hh1),
                std::string("1.001000000 0.1 0.2 0.3\n"));

      // ns precision, extra digits are skipped
      assert_eq(proc_point_str("1234567890.12345678999 0.1 0.2 0.3", hh1),
                std::string("1234567890.123456789 0.1 0.2 0.3\n"));

      // max value
      assert_eq(proc_point_str("4294967295.999999999 0.1 0.2 0.3", hh1),
                std::string("4294967295.999999999 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("inf 0.1 0.2 0.3", hh1),
                std::string("4294967295.999999999 0.1 0.2 0.3\n"));

      // error on overfull:
      assert_err(proc_point_str("4294967296.000000000 0.1 0.2 0.3", hh1),
                "Bad timestamp: can't read seconds: 4294967296.000000000");

      /// +/- suffixes
      assert_eq(proc_point_str("123456789.12345678999+ 0.1 0.2 0.3", hh1),
                std::string("123456789.123456790 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("123456789.12345678999- 0.1 0.2 0.3", hh1),
                std::string("123456789.123456788 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("0+ 0.1 0.2 0.3", hh1),
                std::string("0.000000001 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("0- 0.1 0.2 0.3", hh1),
                std::string("4294967295.999999999 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("0.- 0.1 0.2 0.3", hh1),
                std::string("4294967295.999999999 0.1 0.2 0.3\n"));

      assert_eq(proc_point_str("0.0- 0.1 0.2 0.3", hh1),
                std::string("4294967295.999999999 0.1 0.2 0.3\n"));

      // inf+/inf- are not supported yet!
//      assert_eq(proc_point_str("inf- 0.1 0.2 0.3", hh1),
//                std::string("4294967295.999999998 0.1 0.2 0.3\n"));

//      assert_eq(proc_point_str("inf+ 0.1 0.2 0.3", hh1),
//                std::string("0.000000000 0.1 0.2 0.3\n"));

    }

*/

/***************************************************************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
    return 1;
  }
  return 0;
}
