#!/bin/perl
#
#   20110516       javery
#                  non whoi ships port messages are raw: they don't have the format of
#                  the calliope messages, with |header time message| separated by white space.
#                  So they need to be "cooked" to get them from raw to calliope form.
#
#   20110527       javery
#                  added --multi
#                       this assumes two or more strings "seperated by "/n" aka newline
#                       use join or split to join the line of split it
#                        --multi join
#                        --multi split
#   20130417       lstolp
#                  add options for udp data sent from dsLog
#
#
#  20130124  move autoflush down to make_entry
#  20130125  close of foreach loop in wrong place, moved above entry into database
#  20130126  lstolp
#                  - puts my $sock inside while loop so it creates a new one
#                    at the top of each while, then closes it at the bottom
#                    wait a nano of a second before grabbing message
#                  - mesg should be closer in real time
#                  - cleaned up debug
#
#   20130129       ams and lws
#                  -moved database connect and disconnect around
#                  -now fuser shows the the database as not constantly open
#                  
# 
#  version 20130129
#




use strict;
use warnings;
use DBI;
use IO::Socket::INET;
use Getopt::Long;

#$| = 1;


my $debug = 0;
# udp port changes depending on which vaisala is used
my $port = "";


# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/datascreen.db";
my $username;
my $password;
my $type;
my $udp_type = "wxt";


# misc variables
my $mesg;
my @strings;
my $field1;
my $field2;
my @data;
my $data;
my %wxt = ();
my $baro;
my ($ggatime, $wx, $var, $vaisala, @comp, $nextime);
my $wkey;

# get options from the command line 
GetOptions('debug' => \$debug, 'port=s' => \$port, 'database=s' => \$db_name,
         'udptype=s' => \$udp_type,);


my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $table_result = $dbh->do( "CREATE TABLE latest_data (
              header  VARCHAR(10) PRIMARY KEY ,
              udp_time VARCHAR (10),
              data VARCHAR (180),
              timeEnter VARCHAR(120))" );

# write immediately to the database
$dbh->do("PRAGMA synchronous = OFF");

# disconnect from the database before starting loop
$dbh->disconnect();



if ($debug) {
  if (!defined($table_result)) {
        print STDERR "Table already created\n";
  } else {
        print STDERR "Table created\n";
  }
}

while (1) {
        my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );
        $sock->recv($mesg, 180);
        $ggatime = localtime;
        print "\n\n This is the start of the loop \$mesg = $ggatime $mesg" if $debug;

		@strings = split (" ", $mesg);
                $field1 = $strings[2];
                @data = split(",",$strings[4]);
                $wx = shift @data;
               print "\$wx = $wx\n" if $debug;

 # figure out which vaisala the string is coming from to add the WXTP/S to the
 # variable
        $var = substr($wx, 0, 1);
             print "\$var = $var\n" if $debug;
           if ($var eq "P")
             {
              $vaisala = "WXTP";
           print "\$vaisala = $vaisala\n" if $debug;
             }
           if ($var eq "S")
             {
              $vaisala = "WXTS";
           print "\$vaisala = $vaisala\n" if $debug;
             }

        foreach $data (@data)
             {
               @comp = split("=",$data);
               $wkey = "$vaisala"."_"."$comp[0]";
             if ($wkey =~ /Pa/)
	       {
		my $chr = chop($comp[1]);
		my $baro = $comp[1] + (.1185 * 15.5);
	        $baro=sprintf("%.1f", $baro);
		$wxt{$wkey} = $baro;

		print "\$rawbaro = $comp[1]\n" if $debug;
		print "\$baro = $baro\n" if $debug;
		 }
	   else
 	     {
               my $chr = chop($comp[1]);
               $wxt{$wkey} = $comp[1];
             }
          }


$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");


 
	    while ( my ($key, $value) = each(%wxt) ) {
              $nextime = localtime;
              print "\$key = $nextime $value\n" if $debug;

              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$value')");

             }


  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();



          #  $sock->close();
}
