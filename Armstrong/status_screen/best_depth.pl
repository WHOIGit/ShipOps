#!/usr/bin/perl
# best_depth.pl
#
#  check depth feeders, put out according to priority				cseaton 20180404
#
#  20180404  cts  checks the database for depth data, generates one depth feeder
#            prints out: 
#            DPT string for the most recently acquired data
#            dpt string is:
#                sentence identifier, total depth, transducer depth, source identifier
#                $ARDPT, 357.8, 0.0, 12.0
#                source identifier: 12.0 - Knudsen
#                                   80.0 - EK80
#                                  122.0 - EM122
#                                  710.0 - EM710
#                                   
#
#
use strict;
#use warnings;
use DBI;
use IO::Socket::INET;
use IO::Handle;
use Getopt::Long;

my $debug = 0;

# sqlite3 parameters
my $dsdb = "datascreen.db";
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/"."$dsdb";
my $dbh;
my $username;
my $password;
my %data;
my %time;
my ($source, $dpt);
my ($a, $b);

my $port = "55607";
my $addr = "192.147.41.25";


while (1) {

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can not connect.";
print STDERR "Database connected: \n" if $debug;

my $data = $dbh->selectall_arrayref("SELECT header,udp_time,data,timeEnter from latest_data");

foreach my $row (@$data) {
	my ($key, $dbtime, $value, $timeEnter) = @$row;
	#my @splittime = split(":",$dbtime);
	#$dbtime = $splittime[0]*60*60+$splittime[1]*60+$splittime[2];
	
	if ($key =~ /Depth12/) {
  	   $dpt = "\$ARDPT,$value,0.0,kn12\n";
	   my $epoch_time = time;
	   my $time = localtime;
	   my @now = split(" ",$time);
	   $time{$dpt} = $epoch_time - $timeEnter;			
	   $source = "knudsen";
	   print "knudsen found $time{$dpt} sec ago\n" if $debug;
	   }
	if ($key =~ /EM122/) {
	   $dpt = "\$ARDPT,$value,0.0,em122\n";
	   my $epoch_time = time;
           my $time = localtime;
           my @now = split(" ",$time);
           $time{$dpt} = $epoch_time - $timeEnter;  
	   $source = "em122";
	   print "em122 found $time{$dpt} sec ago\n" if $debug;
	   }
	if ($key =~ /EM710/) {
	   $dpt = "\$ARDPT,$value,0.0,em710\n";
	   my $epoch_time = time;
           my $time = localtime;
           my @now = split(" ",$time);
           $time{$dpt} = $epoch_time - $timeEnter; 
	   $source = "em710";
	   print "em710 found $time{$dpt} sec ago\n" if $debug;
	   }
	if ($key =~ /ek80/) {
	   $dpt = "\$ARDPT,$value,0.0,ek80\n";
	   my $epoch_time = time;
           my $time = localtime;
           my @now = split(" ",$time);
           $time{$dpt} = $epoch_time - $timeEnter;
	   $source = "ek80";
	   print "ek80 found $time{$dpt} sec ago\n" if $debug;
	   }
}	   

print "most recent string is $dpt---- Elapsed time: $time{$dpt}\n" if $debug;

$dbh->disconnect();

if ($time{$dpt} < 10) {

    my $sock = new IO::Socket::INET->new(PeerPort=> $port,
                                 Proto => 'udp', 
				 PeerAddr  => $addr,
				 Broadcast => 1,
				 Reuse     => 1, )
    or die "Can't bind: $@\n";
    print STDERR "Port $port connected: \n" if $debug;

    my $output = $sock->send($dpt);
    print "Characters sent: $output \n" if $debug;

}
sleep(1);
}

