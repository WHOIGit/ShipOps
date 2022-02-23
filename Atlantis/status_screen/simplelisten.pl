#!/usr/bin/perl -w

#       6/25/01 Initial version, jim akens, whoi        
#       12/28/01 jim akens, whoi, added knudsen, cmail athena data file
#       6/20/08 jpa Stripped for use as a test program
#       5/5/11 lstolp stipped it down even more.. very simple 


use IO::Socket;


$sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $ARGV[0], Reuse=>1);
die "Could not create socket: $!\n" unless $sock;

while (1)
{
 $sock->recv($msg,3600);
 print "$msg";
}
