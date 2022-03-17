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

# sqlite3 parameters
my $dsdb = "datascreen.db";
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/"."$dsdb";
my $dbh;
my $username;
my $password;

# serial port setup
my $PORT = "/dev/ttyS3"; # port to watch



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


@ssv = split (/\./, $data{SSV});
my $dec = substr ($ssv[1], 0, 1 );
$ssv = "S"."$ssv[0]"."$dec";


print "\$ssv = $ssv\n" if $debug; 

# set all the serial port information and then write the sound velocity

my $port = Device::SerialPort->new($PORT) || die "Can't Open $PORT: $!";
$port->baudrate(4800) || die "failed setting baudrate";
$port->parity("none") || die "failed setting parity";
$port->databits(8) || die "failed setting databits";
# did not need this one
#$port->stty_icrnl(1) || die "failed setting convert cr to new line";
$port->handshake("none") || die "failed setting handshake";
$port->write_settings || die "no settings";


$msg=$port->write("$ssv\n");

sleep (.1);

$port->close     || die "close port failed";

undef $port;

sleep (1);
}
 
