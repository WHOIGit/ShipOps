#!/usr/bin/perl
###############################################################################
#
# test_tcp_read - sl 5/99
#
# Description: reads UDP packets from tcp port and print outs to stdout
#
# Usage: test_tcp_read <-p port>
#
# History:
#       Date       Who      Description
#       ----       ---      ---------------------------------------------------
#       5/99       SL       Create
###############################################################################
my $debug = 0;
#local listen port
my $opt_l = "55904";
#udp port to write to
my $opt_p = "55908";
#address to send to
my $opt_H ="199.92.161.3";

use IO::Socket;

$sock1 = IO::Socket::INET->new(Proto => 'udp',
                              LocalPort => $opt_l);

$sock2 = IO::Socket::INET->new(Proto => 'udp',
                              PeerPort => "$opt_p",
                              PeerAddr => "$opt_H",
                              Type => SOCK_DGRAM);

while (1)
    {$msg="empty";
     $sock1->recv($msg,180);
#     print "$msg\n";
     $sock2->send($msg);
#print STDERR "\n I just read in the msg: $msg" if $debug;
     }
