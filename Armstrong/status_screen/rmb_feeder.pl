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
#   20130126       lstolp
#                  did a bunch of clean up similar to rmc_feeder.pl
#
#   20130129       ams and lws
#                  -moved database connect and disconnect around
#                  -now fuser shows the the database as not constantly open
#                  
#   20160806	   cts
#		   -switched to take in $ECRMB from openCPN on swim computer
#		   -switched out for now to 0nm and 0knots:
#			hard-coded a distance from station as 0.25 nm, 
#		    at that point db will read 0:00* and 0 nm to station. 
#
#   20161010       lws
#                  -openCPN running on pi
#                  -database writing to tmpfs filesystem instead of disk
#
#
#  version 20160806
#




use strict;
use warnings;

use DBI;
use IO::Socket::INET;
use Getopt::Long;

$| = 1;

my $debug = 0;

# udp port.. this needs to be changed in dslog 
my $port = "55055";

# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
#my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/rmb.db";
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
my %rmb = ();
my ($gps, $i, $ecrmb, $spd, $distance, $ttwp);
my (@keys, $key);



# get options from the command line 
GetOptions('debug' => \$debug, 'port=s' => \$port, 'database=s' => \$db_name,
         'udptype=s' => \$udp_type,);

print "Debug: $debug port: $port \n database: $db_name \n",
        "udptype: $udp_type \n" if $debug;


my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $table_result = $dbh->do( "CREATE TABLE latest_data (
              header  VARCHAR(10) PRIMARY KEY ,
              timeEnter VARCHAR(120))" );


# write immediately to the database
$dbh->do("PRAGMA synchronous = OFF");

# disconnect from the database before starting loop
$dbh->disconnect();


while (1) {
 
        my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );
        $sock->recv($mesg, 180);
        print STDERR "\n\n Start of loop input message:\n $mesg" if $debug;

          if ($mesg =~ /\$ECRMB/)
           {
           print "found \$ECRMB\n" if $debug;
           @strings = split (",", $mesg);
           print "\@strings is @strings\n" if $debug;
           $field1 = time;	##$field1 = $strings[2];
	   print "\$field1 = $field1\n" if $debug;
#           print "\$strings[4] = $strings[4]\n" if $debug;
           @data = @strings;		 #@data = split(",",$strings[4]);
           $gps = shift @data;
           print "\@data = @data\n" if $debug;
           my @ecrmb = qw /status error steer orgid destid dlat latd dlon lond DTWP bearing velocity chsum/;

           $i = 0;
           foreach $ecrmb (@ecrmb)
            {
              $rmb{$ecrmb} = $data[$i];
              $i++;
            }

       # calculate time to waypoint TTWP
       # added if/else loop for when on station or speed is negative, otherwise barfs.
       
       $spd = $rmb{velocity};

       print "\$spd is $spd\n" if $debug;

       $distance = $rmb{DTWP};
 
	
	if ($spd<= 0.25) {
		$rmb{TTWP} = 0;    
	
my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");    

	    @keys = keys %rmb;
            foreach $key (@keys)
            {			
              	if ($key eq "DTWP")
              	{
              	$dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              	VALUES ('$key', '$field1', 'on station')");
              	}
             	if ($key eq "TTWP")
              	{
              	$dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              	VALUES ('$key', '$field1', 'on station')");
              	}
			
	   }

  # write immediately to the database
  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();
     

	}
	else {
       $ttwp = sprintf("%.2f",$distance / $spd);        
       print "\$ttwp = $ttwp\n" if $debug;

       $rmb{TTWP} = $ttwp;
       
      while ( my ($key, $value) = each(%rmb) ) {
        print "$key => $value\n" if $debug;
        }

my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");    


       @keys = keys %rmb;
         foreach $key (@keys)
             {
              if ($key eq "DTWP")
              {       
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$rmb{DTWP}')");
              } 
             if ($key eq "TTWP")
              { 
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$rmb{TTWP}')");
              }    

             } 

	# write immediately to the database
  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();
     
   }


          }  else { print "NOT ECRMB\n" if $debug };
     close($sock)
  }  # close infinite while loop
