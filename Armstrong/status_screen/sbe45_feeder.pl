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
#   20130417       lstolp
#                  add options for udp data sent from dsLog
#
#
#   20130126       asimoneau and lstolp
#                  - general clean up similar to rmc_feeder.pl
#
#
#   20130129       ams and lws
#                  -moved database connect and disconnect around
#                  -now fuser shows the the database as not constantly open
#                  
#   20160627       lstolp
#                  - included sbe45 temperature
#
#   20161010       lstolp
#                  - moved database to tmpfs filesystem
#  version 201606
#



use strict;
use warnings;
use DBI;
use IO::Socket::INET;
use Getopt::Long;

#$| = 1;

my $debug = 0;

# udpport number
my $port = "55501";

# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
my $username;
my $password;
my $type;
my $udp_type;

# misc variables to initialize
my @messages;
my @strings;
my $field1;
my $field2;
my $mesg;
my $sbe45s = "0";
my $sbe45c = "0";
my $sbe45t = "0";

# get options from the command line 
GetOptions('debug' => \$debug, 'port=s' => \$port, 'database=s' => \$db_name,
         'udptype=s' => \$udp_type); 

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


while (1) {
        my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );
        $sock->recv($mesg, 180);
        print STDERR "\n\n Start of Loop Input message: $mesg" if $debug;
        @strings = split (" ", $mesg);
        print "\@strings = @strings\n" if $debug;
        $field1 = $strings[2];
        $sbe45s = $strings[6];
        my $chr = chop($sbe45s);
        $sbe45c = $strings[5];   # conductivity in S/m
        $chr = chop($sbe45c);
        $sbe45t = $strings[4];   # temp
        $chr = chop($sbe45t);

my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");
 
       
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('SBE45S', '$field1', '$sbe45s')");
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('SBE45C', '$field1', '$sbe45c')");
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('SBE45T', '$field1', '$sbe45t')");

  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();

      close($sock)

       }


