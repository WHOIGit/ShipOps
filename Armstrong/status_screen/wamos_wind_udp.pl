#!/usr/bin/perl
#  
#  asimoneau and lstolp                         20130119
#  
#  Program to grab ssv out of datascreen.db and then send it out the serial port
#  -- the output of this file can be seen using the serial sniffer
#
#
#
#

use strict;
use DBI;
use Getopt::Long;
use Time::gmtime;
use IO::Socket::INET;
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
my $outstring;
my @ssv;
my $sst;
my $knots;
my $wkey;
my $twindspeed;

# sqlite3 parameters
my $dsdb = "datascreen.db";
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/"."$dsdb";
my $dbh;
my $username;
my $password;


# UDP  port setup
my $udpport = "55410";
#my $peeradd = "scibc.science.knorr.whoi.edu";
#my $peeradd = "172.16.60.17";
#my $peeradd = "192.147.41.25"; #turn on if old wamos machine
my $peeradd = "10.100.100.49"; #wave radar server
#my $peeradd = "10.100.100.50";	#wave radar virtual machine
#my $peeradd = "192.147.41.2"; #test

my $Debug=0;

#getoptions from the command line 
GetOptions('debug' => \$debug); 


while (1)
{
### OPEN data base and get data
## check data is within the last minute??

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || 
die "Can not connect.";

print STDERR "Database connected: ", $db_name, "\n" if $debug;

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



# this is mostly a debug statement to see what your hash looks like
while ( my ($key, $value) = each(%data) ) {
        print "$key => $value\n" if ($Debug);
    }

                my $knots = ($data{WXTS_TS} * 1.9438445);
                $knots=sprintf("%.2f", $knots);



$dbh->disconnect();
$outstring="\$ARMWD,$data{WXTS_TD},T,$data{WXTS_TD},M,$knots,N,$data{WXTS_TS},M\n";

my $MySocket=new IO::Socket::INET->new(PeerPort=>$udpport,
                                         Proto=>'udp',
                                         PeerAddr => $peeradd,
                                         Broadcast=>1,
                                         Reuse=>1, )
                                         or die "Can't bind: $@\n";

     # send input out udp port
     my $chars_sent = $MySocket->send($outstring);
     print "Characters sent: $chars_sent \n" if ($Debug);
     sleep(1);
}
 




