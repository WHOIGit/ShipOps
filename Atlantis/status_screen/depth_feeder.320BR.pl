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
#   20130126       asimoneau and lstolp 
#                  did a bunch of clean up similar to rmc_feeder.pl
#
#   20130129       ams and lws
#                  -moved database connect and disconnect around
#                  -now fuser shows the the database as not constantly open
#                  
#  20170613        lstolp
#                  - updated to use tmpfs for db
# 
#

use strict;
use warnings;
use DBI;
use IO::Socket::INET;
use Getopt::Long;

my $debug = 0;

# udp port number
my $port = "55603";

# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
my $username;
my $password;
my $type;
my $udp_type;

# misc variables to initialize
my $mesg;
my @strings;
my $field1;
my $field2;
my @data;
my $data;
my %depth = ();
my ($i, $pkel);
my $dep;
my (@keys, $key);

# get options from the command line 
GetOptions('debug' => \$debug, 'port=s' => \$port, 'database=s' => \$db_name,
         'udptype=s' => \$udp_type,);

print "Debug: $debug port: $port \n database: $db_name \n" if $debug;

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

while (1) {
        my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );
#        sleep (.1);
        $sock->recv($mesg, 180);
        print STDERR "\n\n Start of input message: $mesg" if $debug;
       
        @strings = split (" ", $mesg);
        $field1 = $strings[2];
        print "\$strings[4] = $strings[4]\n" if $debug;
        @data = split(",",$strings[4]);
        my $dep = shift @data;

          if ($dep =~ /\$PKEL99/)
           {
           print "found \$PKEL99\n" if $debug;
           print "\@data = @data\n" if $debug;
           my @pkel = qw /Depth12 12valid draft12 Depth35 35valid draft12 sspeed position/;
 
           $i = 0;
            foreach $pkel (@pkel)
            {
              $depth{$pkel} = $data[$i];
              $i++;
           }


     # chop off the ending .
       my $chr = chop $depth{Depth12};

       
     while ( my ($key, $value) = each(%depth) ) {
            print "$key => $value\n" if $debug;
          }  


my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");    

   @keys = keys %depth;
   foreach $key (@keys)
    {
      if ($key eq "Depth12")
       { 
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$depth{Depth12}')");
       }
      if ($key eq "Depth35")
       { 
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$depth{Depth35}')");
       }

       }

  # write immediately to the database
  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();
     
    }   else { print "NOT PKEL99\n" if $debug };

#    close($sock);

 }  # close infinite while loop
