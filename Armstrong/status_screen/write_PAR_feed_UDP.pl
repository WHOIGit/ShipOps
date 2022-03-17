#!/usr/bin/perl
#  
#  cgraver and asimoneau 12 March 2013 
#  
#  adapted from:
#  asimoneau and lstolp       write_cbeam_ssv.pl                  20130119
#  sl			      test_udp_write			  5/99
#  
#	Program to grab PAR value out of the database, timestamp it, and send out as a udp broadcast to a specified ip address.
#
#
#
#

use strict;
use DBI;
use Getopt::Long;
use Device::SerialPort;
use Time::gmtime;
use IO::Handle;
use Time::HiRes qw(gettimeofday sleep);         #need to wait for response

# misc variables
my $debug = 0;
my $nowdate;
my $nowtime;
my %data;
my $msg;
my $tens;
my $decimal;
my @ues;
my $ues;
my $par;
my $sock;
my $timestamp;

# sqlite3 parameters
my $dsdb = "datascreen.db";
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/"."$dsdb";
my $dbh;
my $username;
my $password;


while (1)
{
### OPEN data base and get data
## check data is within the last minute??

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can not connec
t.";

print STDERR "Database connected: ", $dbh, "\n" if $debug;

# time variable to test how old the data in the db is.. 
my $time = localtime;
my $epoch_time = time;

my $data = $dbh->selectall_arrayref("SELECT header,data from latest_data");

# @$ is a reference to an array
# create a hash of the data from the reference array
foreach my $row (@$data) {
    my ($key, $value) = @$row;
    $data{$key} = $value;
}


$dbh->disconnect();

# this is mostly a debug statement to see what your hash looks like
while ( my ($key, $value) = each(%data) ) {
        print "$key => $value\n" if ($debug);
      }

#@ues = split (" ", $data{PAR});
#$par = $ues[3];
$par = $data{PAR};

print "\$par = $par\n" if $debug; 

# now send out UDP

use IO::Socket;

$sock = IO::Socket::INET->new(Proto => 'udp',
                              PeerPort => "55406",
                              PeerAddr => "192.168.11.30",
                              Type => SOCK_DGRAM);

$timestamp = `date`; chomp($timestamp);
$msg= "$timestamp PAR $par\n";
$sock->send($msg);
sleep(1);
     }
