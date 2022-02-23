#!/usr/bin/perl
#
# split out the RAD data for status screens
#
# port: 55406
# string: MET 2016/03/25 13:00:09.668 RAD 
#   $WIR22,16/03/25,13:01:20, 176, -175.1, 381.71, 23.33, 22.49, 355.08, 27.9, 10.6
#    ID    DATE      TIME       #   PIR    LW      TCASE  TDOME     SW   T-AVR BATT
# 
# want LW Long Wave?   
# want SW Short Wave?
#
#
#
#  


use strict;
use warnings;

use DBI;
use IO::Socket::INET;
use Getopt::Long;

$| = 1;

my $debug = 0;

# udp port to listen on
my $port = "55406";

# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
my $username;
my $password;
my $type;
my $udp_type;

# misc variables to initialize
my $mesg;
my @strings;
my $field1;
my $field2;
my @data;
my $data;
my ($sw, $lw);


# get options from the command line 
GetOptions('debug' => \$debug, 'port=s' => \$port, 'database=s' => \$db_name,
         'udptype=s' => \$udp_type,);

print "Debug: $debug port: $port \n database: $db_name \n",
        "udptype: $udp_type \n" if $debug;


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


while (1) {

        my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );
        $sock->recv($mesg, 180);
        print STDERR "\n\n Start of loop input message:\n $mesg" if $debug;

        @strings = split (" ", $mesg);
                $type = $strings[3];
                $field1 = $strings[2];
                $field2 = $strings[4];
                $strings[7]=~ s/,$//;
                $strings[10]=~ s/,$//;
                $lw = $strings[7];
                $sw = $strings[10];



# put data into database

            $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

         my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
         BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
         END");

         print "Table insert: type: $type field1: $field1 field2: $field2\n" if $debug;

         $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
               VALUES ('lw', '$field1', '$lw')");

         $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
               VALUES ('sw', '$field1', '$sw')");

  # write immediately to the database
  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();
}

