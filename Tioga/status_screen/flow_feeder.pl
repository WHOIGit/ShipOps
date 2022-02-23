#!/usr/bin/perl
#
#   2017042        cts
#                  - adapted from gravity feeder to indicate flowmeter status
#		   - moved out of udp_db_feeder to indicate 0 flow as RED on screen
#
#   20190530       lstolp
#                  - changed UDP port for Tioga
#                  - commented out the 'turning' red, for now. want to see it work
#  version 201905
#

use strict;
use warnings;
use DBI;
use IO::Socket::INET;
use Getopt::Long;

#$| = 1;

my $debug = 0;

# udpport number
my $port = "55507";

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
my $flowstatus;

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
        $strings[7]=~s/,$//;				#remove comma from ml/s
	$field1 = $strings[2];
	$flowstatus = $strings[7]*60/1000; 		#convert from ml/s to Lpm

my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");
 
       
              
#	if($flowstatus > 0.5 ) {		#avoid ship roll causing false zero
		print "flow status = $flowstatus\n" if $debug; 
		print "field1 = $field1;\n" if $debug;
		print "string = $strings[7]\n" if $debug;
		$dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              	VALUES ('FLOW', '$field1', '$flowstatus')");
	#		}


  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();

      close($sock)

       }


