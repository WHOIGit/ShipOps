#!/usr/bin/perl
#
#   20110516       javery
#                  non whoi ships port messages are raw: they don't have the format of
#                  the calliope messages, with |header time message| separated by white space.
#                  So they need to be "cooked" to get them from raw to calliope form.
#
#   20110527       javery
#                  added --multi
#                       this assumes two or more strings "seperated by "/n" aka newline
#                       use join or split to join the line of split it
#                        --multi join
#                        --multi split
#   20130117       lstolp
#                  add options for udp data sent from dsLog
#
#   20130126       lstolp
#                  - changed looping around.. if not gpgga or gprmc 
#                    don't do anything
#                  - flush only works on writes, put flush statements in
#                    but not really sure they are doing anything
#                  - puts my $sock inside while loop so it creates a new one
#                    at the top of each while, then closes it at the bottom
#                    wait a nano of a second before grabbing message
#                  - mesg should be closer in real time
#                  - cleaned up debug
#
#   20130129       ams and lws
#                  -moved database connect and disconnect around
#                  -now fuser shows the the database as not constantly open
#                  
#   20130130       lws
#                  -took blocking and sleep statement out
#
#   20170613       lstolp
#		   - updated for use w/ tmpfs and db 
#

use strict;
use warnings;

use DBI;
use IO::Socket::INET;
use Getopt::Long;

my $debug = 0;

# udp port.. 55001  for CNAV - whatever is switched
my $port = "55001";

# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/mvp.db";
my $username;
my $password;
my $type;
my $udp_type;

# misc variables to initialize
my $mesg ;
my @strings;
my $field1;
my $field2;
my @data;
my $data;
my %rmc = ();
my ($gps, $ggatime, $lontime, $cogtime, $sogtime, $lattime, $i, $gprmc);
my ($dec_lat, $dec_lon, $lat_dir, $lon_dir);
my (@keys, $key);



# get options from the command line 
GetOptions('debug' => \$debug, 'port=s' => \$port, 'database=s' => \$db_name,
         'udptype=s' => \$udp_type,);

my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $table_result = $dbh->do( "CREATE TABLE latest_data (
              header  VARCHAR(10) PRIMARY KEY ,
              udp_time VARCHAR (10),
              data VARCHAR (180),
              timeEnter VARCHAR(120))" );

# write immediately to the database
$dbh->do("PRAGMA synchronous = OFF");

# disconnect from the database before starting loop
$dbh->disconnect();


if ($debug) {
  if (!defined($table_result)) {
        print STDERR "Table already created\n";
  } else {
        print STDERR "Table created\n";
  }
}


# moved to outside while loop same as rmb_feeder.pl
my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );

while (1) {
                
        $sock->recv($mesg, 180);
        print STDERR "\n\n Start of loop input message:\n $mesg" if $debug;


# check if message is GPGGA or GPRMC.. if it is not just go to the next string on the
# udp port

     if ($mesg =~ /\$GPGGA/)
      {
       @strings = split (" ", $mesg);
       $field1 = $strings[2];
       print "\$strings[4] = $strings[4]\n" if $debug;
       $gpgga = split(",",$strings[4]);


my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");


             $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
             VALUES ('$type', '$field1', '$field2')");

  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();


     }  # close foreach

  # write immediately to the database
  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();



  } if ($mesg =~ /\$GPVTG/) 

     {
       @strings = split (" ", $mesg);
       $field1 = $strings[2];
       print "\$strings[4] = $strings[4]\n" if $debug;
       $field2 = split(",",$strings[4]);


my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");


             $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
             VALUES ('$type', '$field1', '$field2')");

  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();


     }  # close foreach

  # write immediately to the database
  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();








{ print " NOT GPRMC OR GPGGA\n" if $debug };


#  close($sock)
  }   # close infinite while

