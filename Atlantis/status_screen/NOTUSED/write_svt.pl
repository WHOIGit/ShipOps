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
my $ssv;
my @ssv;
my $sst;

# sqlite3 parameters
my $dsdb = "ssw.db";
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/"."$dsdb";
my $dbh;
my $username;
my $password;

# serial port setup
my $PORT = "/dev/ttyS7"; # port to watch

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


$dbh->disconnect();

# this is mostly a debug statement to see what your hash looks like
while ( my ($key, $value) = each(%data) ) {
        print "$key => $value\n" if ($debug);
      }


$ssv=sprintf("%6.1f",$data{SSV});
$sst=sprintf("%6.2f",$data{SBE48});

print "\$ssv = $ssv\n" if $debug; 
print "\$sst = $sst\n" if $debug; 


# set all the serial port information and then write the sound velocity

my $port = Device::SerialPort->new($PORT) || die "Can't Open $PORT: $!";
$port->baudrate(9600) || die "failed setting baudrate";
$port->parity("none") || die "failed setting parity";
$port->databits(8) || die "failed setting databits";
# did not need this one
#$port->stty_icrnl(1) || die "failed setting convert cr to new line";
$port->handshake("none") || die "failed setting handshake";
$port->write_settings || die "no settings";


$msg=$port->write("$sst $ssv\n\r");

sleep (.1);

$port->close     || die "close port failed";

undef $port;

sleep (1);
}
 




