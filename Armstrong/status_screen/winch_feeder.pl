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
#  20160319 ams   converted to Armstrong, UDP from datalog, here's the string
#  WINCH 2016/03/19 15:04:29.365 MTN 01RD,2016-03-19T08:09:02.100,00001014,-0000061,000344.5,2837
#   	01RD - fwd hydro (.322)
#	02RD - aft hydro (3/8" hydro)
#	03RD - traction (both wires) ???? Not sure yet
#
#  20160807 ams tweaked because the data was backing up 


use strict;
#use warnings;
use DBI;
use IO::Socket;
use IO::Select;
use Getopt::Long;


my $debug = 0;
my $port = "55801";

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


#my $winch_udp = IO::Socket::INET->new(Proto => 'udp', LocalPort => 55801, Reuse => 1 );
#die "Could not create socket: $!\n" unless $winch_udp;

while (1) {

my $winch_udp = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );
	print STDERR "\nStart of loop\n" if $debug;
       	$winch_udp->recv($msg,1800);
	print STDERR "Input message: $msg\n" if $debug;

	my	@strings = split (/ /, $msg);
		@winch_input = split (/,/, $strings[4]);
		$len  = @winch_input;
		if ($len > 1) {
			print STDERR "winch_input: @winch_input length: $len \n" if $debug;
			if ($winch_input[0]  =~ /01RD/) {
				$winch_input[0] = "Fwd Hydro";
                                print STDERR "winch is $winch_input[2]\n" if $debug;
			}
			elsif ($winch_input[0] =~ /02RD/) {
				$winch_input[0] = "Aft Hydro";
			}
			elsif ($winch_input[0] =~ /03RD/) {
				$winch_input[0] = "Traction";
			}
			$winch_data = sprintf("%s,%d,%d,%d",
#				$winch_input[0],$winch_input[1],$winch_input[3],$winch_input[5]);
				$winch_input[0],$winch_input[2],$winch_input[4],$winch_input[3]);
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

    close($winch_udp);
    sleep 1;
		}
	}
