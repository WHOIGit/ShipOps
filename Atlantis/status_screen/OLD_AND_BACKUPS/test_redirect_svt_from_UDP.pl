#!/usr/bin/perl
#  
#starting with write_svt.pl & simplelisten.pl - read a line of data from a UDP port
#and then redirect it to a serial port
#
#this is for redirecting the UDP 55504 string (SST & SSV) to em122 serial feed

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
my $udport = 55504;

# serial port setup
my $PORT = "/dev/ttyS4"; # port to watch

#getoptions from the command line 
GetOptions('debug' => \$debug); 


# set all the serial port information and then write the sound velocity

my $port = Device::SerialPort->new($PORT) || die "Can't Open $PORT: $!";
$port->baudrate(9600) || die "failed setting baudrate";
$port->parity("none") || die "failed setting parity";
$port->databits(8) || die "failed setting databits";
$port->handshake("none") || die "failed setting handshake";
$port->write_settings || die "no settings";

 
###################
#open udp port and read a line

#from simplelisten - open port 55504
use IO::Socket;

my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $udport, Reuse=>1);

die "Could not create socket: $!\n" unless $sock;

#this is where the redirect to serial must happen
while (1)
{
 $sock->recv($msg,180);
      $port->write("$msg");
}
#end udp reading
################

#not sure about closing the serial port
#does it need to be opened and closed each time
#the while lopp is executed???

$port->close     || die "close port failed";

undef $port;

sleep (1);
