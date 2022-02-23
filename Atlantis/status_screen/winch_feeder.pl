#! /usr/bin/perl 
#
#  20110323 javery   converted to Atlantis:
#			moxa is 192.92.161.191
#			udp is 55007
#			winch table is
#				1 = For
#				2 = Aft
#				3 = Trawl
#				4 = Fiber
#
#  20110304 lstolp   took out -w 
#                    too many error messages
#
#  20120327 lstolp   converted to Knorr w/ 3PS system
#                    WINCH NAME:      WINCH NUMBER:      WINCH "DESIGNATOR" NUMBER:
#----------------------------------------------------------------
#  Forward Hydro     #1                01
#  UNUSED            #2                02
#  Stbd Hydro        #3                03
#  UNUSED            #4                04
#  Trawl Winch       #5                05
#  UNUSED            #6                05
#  UNUSED            #7                06
#  UNUSED            #8                07
#
#  20120815 lstolp   reworked for 3PS udp port (of course they had to change everything 
#                    in the udp string)
# 
#
#
#   20130129       ams and lws
#                  -moved database connect and disconnect around
#                  -now fuser shows the the database as not constantly open
#                  
# 
#  version 20130129
#
# 20130716 changed the winch designator for winch 2 to Fiber

use strict;
#use warnings;
use DBI;
use IO::Socket;
use IO::Select;
use Getopt::Long;


my $debug = 0;

# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
my $username;
my $password;
#misc 
my ($msg, $host, $len);

my @winch_input=();
my $winch_data;

# get options from the command line
GetOptions('debug' => \$debug, 'database=s' => \$db_name);

my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
	{ PrintError => 0} ) || die "Can not connect.";

$dbh->do( "CREATE TABLE latest_data (
              header  VARCHAR(10) PRIMARY KEY ,
	      udp_time VARCHAR (10),
              data VARCHAR (180),
              timeEnter VARCHAR(120))" );

# write immediately to the database
$dbh->do("PRAGMA synchronous = OFF");

# disconnect from the database before starting loop
$dbh->disconnect();


my $winch_udp = IO::Socket::INET->new(Proto => 'udp', LocalPort => 55801, Reuse => 1 );
die "Could not create socket: $!\n" unless $winch_udp;

while (1) {
	print STDERR "\nStart of loop\n" if $debug;
       	$winch_udp->recv($msg,180);
	print STDERR "Input message: $msg\n" if $debug;

		@winch_input = split (/,/, $msg);
		$len  = @winch_input;
		if ($len > 1) {
			print STDERR "winch_input: @winch_input length: $len \n" if $debug;
#			$winch_input[0] = ($winch_input[0] == 2) ? "Desh-5" : "Com-15";
# on Atlantis, 1 is For; 2 is Aft; 3 is Trawl; 4 is Fiber
#
			if ($winch_input[2]  =~ /01/) {
				$winch_input[2] = "Forward Hydro";
                                print STDERR "winch is $winch_input[2]\n" if $debug;
			}
			elsif ($winch_input[2] =~ /02/) {
				$winch_input[2] = "Fiber";
			}
			elsif ($winch_input[2] =~ /03/) {
				$winch_input[2] = "Aft Hydro";
			}
                        elsif ($winch_input[2] =~ /04/) {
                                $winch_input[2] = "Trawl";
                        }


			$winch_data = sprintf("%s,%d,%d,%d",
			$winch_input[2],$winch_input[3],$winch_input[5],$winch_input[7]);
			print STDERR "winch_data: $winch_data \n" if $debug;

my $ltime = localtime;
my ($wkday, $month, $day, $time, $yr) = split(" ",$ltime);


$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");

		
	$dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
					VALUES ('Winch', '$time', '$winch_data')");

 $dbh->do("PRAGMA synchronous = OFF");
 $dbh->disconnect();


 }
} 
