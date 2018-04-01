## Graphene -- a simple time-series database with nanosecond precision for scientific applications

Source code: https://github.com/slazav/graphene

E-mail: Vladislav Zavjalov <slazav@altlinux.org>

### Features

- based on BerkleyDB
- store integer, floating point or text values with &lt;seconds&gt;.&lt;nanoseconds&gt; timestamps
- fast access to data, interpolation, downsampling, time ranges
- multi-column numerical values
- command line and socket interfaces for reading/writing data
- http simple json interface for Grafana viewer
- user filters for data processing (calibration tables etc.) -- to be removed?

### Data storage

Data is stored in any folder (default is `.`, but it can be selected via
a command line option -d). The folder contains a BerkleyDB environment
with databases and some other database-specific files. Each dataset is a
separate database, it's name can not contain symbols `.:|+ \t\n/`. By
default a full transaction support is turn on for the database
environment. If needed it can be turned off during compilation. Then you
can have no environment at all (each dataset is a single db file, but
only one program can work with it at a time), or a simple environment
with locking.

### Some BerkleyDB notes

* Documentation can be found here:
 - `https://docs.oracle.com/cd/E17076_05/html/gsg/C/index.html`
 - `https://docs.oracle.com/cd/E17076_05/html/gsg_txn/C/index.html`

* Due to a custom sorting function not all standard BerkleyDB tools can
be used for Graphene databases. For example dumps created by `db_dump` can
be loaded only by graphene `load` command but not by `db_load` utility.

* Do not use environments located on remote filesystems.

* Do not use `db_recover` utility when other program is using a database.


### Data structure

Data are stored as a set of sorted key-value pairs. Key is a timestamp,
one or two 32-bit unsigned integers: a number of seconds
from 1970-01-01 UTC, and optional number of nanoseconds. Duplicated
timestamps are not allowed, but user can choose what to do
with duplicates (see -D option of the graphene program):
 replace -- replace the old record (default),
 skip    -- skip the new record if timestamp already exists,
 error   -- skip the new record and return error message
 sshift  -- increase time by 1 second step until it will be possible
            to put the record (no loss of data)
 nsshift -- same, but with nanosecosd steps.

Value can contain an array of numbers of arbitrary length (data columns)
or some text. The data format can be chosen during the database
creation. Possible variants are: TEXT, INT8, UINT8, INT16, UINT16,
INT32, UINT32, INT64, UINT64, FLOAT, DOUBLE.

Records with 8-bit keys are reserved for database information: data
format, database version, description. Records with 16-bit keys are
reserved for arbitrary user data. These records are not affected by
regular get/put commands.


### Command line interface

The program `graphene` is used to access data from command line.

Usage: `graphene [options] <command> <parameters>`

Options:
- -d <path> -- database directory (default `.`)
- -D <word> -- what to do with duplicated timestamps:
               replace, skip, error, sshift, nsshift (default: replace)
- -h        -- write help message and exit
- -i        -- interactive mode, read commands from stdin
- -s <name> -- socket mode: use unix socket <name> for communications
- -r        -- output relative times (seconds from requested time) instead of absolute timestamps

Interactive mode:

Use -i option to enter the interactive mode. Then commands are read from
stdin, answers are written to stdout. This allows making many requests
without reopening databases. Opening and closing of databases are long,
it can be useful to open the connection once and do many operations.

The program implements a Simple Pipe Protocol (see somewhere in my
`tcl_device` package): When it is started sucsessfully  a prompt message is
printed to stdout started with "#SPP001" and followed by "#OK" line. In
case of an error "#Error: <...>" line is printed and program exits. Then
the program reads commands from stdin and sends ansers to stdout folowed
by "#OK" or "#Error: <...>" lines until the user closes the connection.
If answer contains symbol "#" in the begining of a line, it is protected
with a second "#".

Socket mode is similar to the interactive mode. Graphene program acts as
a server which accepts connections (one at a time) through a unix-domain
socket.


####Commands for manipulating databases:

- `create <name> [<data_fmt>] [<description>]` -- Create a database file.

- `load <name> <file>` -- Create a database and load file in db_dump format
  (note that it is not possible to use db_load utility
  because of non-standard comparison function in graphene databases).

- `delete <name>` -- Delete a database.

- `rename <old_name> <new_name>` -- Rename a database.
   A database can be renamed only if the destination does not exists.

- `set_descr <name> <description>` -- Change database description.

- `info <name>` -- Print database format and description.

- `list` -- List all databases in the data directory.


####Commands for reading and writing data:

- `put <name> <time> <value1> ... <valueN>` -- Write a data point.

- `get_next <extended name> [<time1>]` -- Get first point with t>=time1.

- `get_prev <extended name> [<time2>]` -- Get last point with t<=time2.

- `get <extended name> [<time2>]` -- For integer and text databases
  get is equivalent to get_prev. For double databases it does linear
  interpolation between points, or return the last point if time is
  larger then that of the latest point.

- `get_range <extended name> [<time1>] [<time2>] [<dt>]` -- Get
  points in the time range. If parameter dt>0 then data are filtered,
  only points with distance >dt between them are shown. This works fast
  for any ratio of dt and interpoint distance. For text data only first
  lines are shown.

You can use words "now", "now_s" and "inf" as a timestamp. You can also
add "+" or "-" symbol to numerical value to add 1 ns. This is convenient
if you know a timestamp of some value and want to read next or  previous
one. Default value for time1 is 0, for time2 is "inf".

The "extended name" used in get_* commands have the following format:
`<name>[:<column>][|<filter>]`

`<column>` is a column number (0,1,..), if it exists, then only this
column is shown. If a certain column is requested but data array is not
long enough, a "NaN" value is returned. Columns are ignored for text data.

`<filter>` is a name of filter program, if it exists, the program is run and
data is filtered through it. The program should be located in the
database directory, program name can not contain '.:|+ \t\n/' symbols.
`TODO: remove this feature?`

####Commands for deleting data:

- `del <name> <time>` -- Delete a data point.

- `del_range  <name> [<time1>] [<time2>]` -- Delete all points in the range.

####Command for syncing databases in interactive mode:

- `sync` -- This command flushes any cached information to disk. It is
useful if you keep a connection (for example via ssh) for a long time,
but want to commit your changes to the databases regularly.

- `sync <name>` -- Same, but for one database. If database is not opened
  command does nothing and returns without error.

- `close` -- This command closes all previously opened databases. It can be
used if you want to close unused databases and sync data.

- `close <name>` -- Same, but for one database. If database is not opened
  command does nothing and returns without error.

Information:

- `cmdlist` -- print list of commands.

- `*idn?`   -- print intentifier: "Graphene database <version>".

Examples:

See `examples/*` in the source folder



### HTTP + Simple JSON interface

The simple JSON interface can be used with Grafana frontend to access
data (simple_json plugin is needed). Text databases can be viewed as
annotations, and numerical as metrics. Columns can be specified after
database name: <name>:<column>, default column is 0.

Usage: `graphene_http [options]`
Options:
```
 -p <port>  -- tcp port for connections (default 8081)
 -d <path>  -- database path (default /var/lib/graphene/)
 -v <level> -- be verbose
                0 - write nothing.
                1 - write some information on start
                2 - write info about connections
                3 - write input data
                4 - write output data
 -l <file>  -- log file, use '-' for stdout
               (default /var/log/graphene.log in daemon mode, '-' in)"
 -f         -- do fork and run as a daemon;
 -h         -- write help message and exit;
```

###  Matlab/octave interface

Nothing is ready yet. You can use something like this to get data using the
graphene program:

  [r, out] = system('graphene get_range my_dataset 1464260400 now 60000');
  [t val1 val2] = strread(out, '%f %f %f');

### Known problems

- Linebreaks in text databases. You can put linebreaks in text database using
the command line interface. In interactive mode it is not possible, because
line breaks always mean starting of a new command.

###  Performance

NOTE: this was written for a Graphene database without BerkleyBD
environment.

Consider a situation: we want to measure some parameter for a year with
10 s period and put 8-byte double values into the database. This means 3.15
million points.

let's create a database DB and put all these points at once:
```
$ graphene -d . create DB
$ for t in $(seq 3153600); do printf "put DB $t $RANDOM\n"; done |
    ./graphene -d . interactive &>/dev/null
```

This takes about 1 minute at my computer. Note that I use interactive
mode to write all points without reopening the database. If I do writing
with command-line interface it takes about 10 hours:
```
$ for t in $(seq 3153600); do ./graphene -d . put DB "$t" "$RANDOM"; done
```

Size of the database is 73.6 Mb, 24.5 bytes/point. Gzip can make in
smaller (17.3 Mb, 5.74 bytes/point), xz even smaller (10.3 Mb, 3.41
bytes/point).

If you use non-integer seconds for timestamps the size will increase by
4 bytes per point.

If you have a few parameters which do not need separate timestamps, you
can put them into a multi-column data points (put DB $t $par1 $par2 ...)
saving some space. Database size grows by 8 bytes per additional column.


You can also configure the database to store 4-byte float values instead
of 8-byte doubles. It will save 4 bytes per data column.

Reading of all the points takes 23s:

```
$ time graphene -d . get_range DB | wc
23.18user 0.10system 0:23.27elapsed 100%CPU (0avgtext+0avgdata 5168maxresident)k
0inputs+0outputs (0major+289minor)pagefaults 0swaps
3153600 6307200 73507308
```

Reading of last 3000 points, first 3000 points, every 1000th point takes
almost nothing:

```
$ time graphene -d . get_range DB 3150601 3153600 | wc
0.01user 0.00system 0:00.01elapsed 94%CPU (0avgtext+0avgdata 5024maxresident)k
0inputs+0outputs (0major+257minor)pagefaults 0swaps
   3000    6000   40965
```

```
$ time graphene -d . get_range DB 0 3000 | wc
0.01user 0.00system 0:00.01elapsed 94%CPU (0avgtext+0avgdata 4896maxresident)k
0inputs+0outputs (0major+249minor)pagefaults 0swaps
   3000    6000   30873
```

```
$ time graphene -d . get_range DB 0 -1 1000 | wc
0.02user 0.00system 0:00.03elapsed 96%CPU (0avgtext+0avgdata 5204maxresident)k
0inputs+0outputs (0major+292minor)pagefaults 0swaps
   3154    6308   42011
```

###  Database versions

In v1 time was stored in milliseconds in a 64-bit integer.
Time input and output in the command line interface used integer numbers
of milliseconds:
```
$ graphene put DB 1479463946000 1.0
$ graphene get_range DB
1479463946000 1.0
```

In v2 time is stored as two 32-bit numbers: number of seconds
and number of nanoseconds. On input and output a decimal dot is used:
```
$ graphene put DB 1479463946 1.0
$ graphene put DB 1479463946.123456789 2.0
$ graphene get_range DB
1479463946.000000000 1.0
1479463946.123456789 2.0
```
v1 databases are supported in v2.
