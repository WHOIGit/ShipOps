#!/usr/bin/perl
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
#   20130117       lstolp
#                  add options for udp data sent from dsLog
#
#   20130126       lstolp
#                  - changed looping around.. if not gpgga or gprmc 
#                    don't do anything
#                  - flush only works on writes, put flush statements in
#                    but not really sure they are doing anything
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
#   20130130       lws
#                  -took blocking and sleep statement out
#
#   20170613       lstolp
#		   - updated for use w/ tmpfs and db 
#

use strict;
use warnings;

use DBI;
use IO::Socket::INET;
use Getopt::Long;

my $debug = 0;

# udp port.. 55001  for CNAV - whatever is switched
my $port = "55001";

# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
my $username;
my $password;
my $type;
my $udp_type;

# misc variables to initialize
my $mesg ;
my @strings;
my $field1;
my $field2;
my @data;
my $data;
my %rmc = ();
my ($gps, $ggatime, $lontime, $cogtime, $sogtime, $lattime, $i, $gprmc);
my ($dec_lat, $dec_lon, $lat_dir, $lon_dir);
my (@keys, $key);



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


# moved to outside while loop same as rmb_feeder.pl
my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );

while (1) {
                
        $sock->recv($mesg, 180);
        print STDERR "\n\n Start of loop input message:\n $mesg" if $debug;


# check if message is GPGGA or GPRMC.. if it is not just go to the next string on the
# udp port

     if ($mesg =~ /\$GPGGA|\$GPRMC/)
      {
       @strings = split (" ", $mesg);
       $field1 = $strings[2];
       print "\$strings[4] = $strings[4]\n" if $debug;
       @data = split(",",$strings[4]);
       $gps = shift @data;

          if ($gps =~ /\$GPGGA/)
           {
             $field2 = $strings[4];
             $type = $strings[3];
             $ggatime = localtime;
             print "found $ggatime \$GPGGA\n" if $debug;
# print GPGGA string to  database     

my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");


             $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
             VALUES ('$type', '$field1', '$field2')");

  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();



           }  
           elsif ($gps =~ /\$GPRMC/)
           {
           print "found \$GPRMC\n" if $debug;
           my @gprmc = qw /time active lat latdir lon londir SOG COG date mag chsum/;

           $i = 0;
            foreach $gprmc (@gprmc)
            {
              $rmc{$gprmc} = $data[$i];
              $i++;
           }

# figure out lat and lon in decimal  
# not checking if it is valid or not raw data has proper flags
       $dec_lat= substr($rmc{lat},0,2) + substr($rmc{lat},2)/60.0;
       $dec_lon= substr($rmc{lon},0,3) + substr($rmc{lon},3)/60.0;

        if ($rmc{latdir} eq "S") {$lat_dir = -1.0;}
                        else {$lat_dir = 1.0;}
        if ($rmc{londir} eq "W") {$lon_dir = -1.0;}
                        else {$lon_dir = 1.0;}
        $dec_lat = sprintf("%.6f",$dec_lat * $lat_dir);
        $dec_lon = sprintf("%.6f",$dec_lon * $lon_dir);
        $rmc{Dec_Lat} = $dec_lat;
        $rmc{Dec_Lon} = $dec_lon;
        }
# Step through the keys of the hash to pull out COG, SOC, Dec_Lat, Dec_lon   

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");




   @keys = keys %rmc;
   foreach $key (@keys)
    {
      if ($key eq "COG")
       {     $cogtime = localtime;
             print "$cogtime COG = $rmc{COG}\n" if $debug;
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$rmc{COG}')");
       }

      elsif ($key eq "SOG")
       {      $sogtime = localtime;
              print "$sogtime SOG = $rmc{SOG}\n" if $debug;
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$rmc{SOG}')");

       }

     elsif ($key eq "Dec_Lat")
       {      $lattime = localtime;
              print "$lattime  Dec_Lat = $rmc{Dec_Lat}\n" if $debug;
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$rmc{Dec_Lat}')");

       }
   
     elsif ($key eq "Dec_Lon")
         {    $lontime = localtime;
              print "$lontime Dec_lon = $rmc{Dec_Lon}\n" if $debug;
              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$rmc{Dec_Lon}')");

       } 

     }  # close foreach

  # write immediately to the database
  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();



  } else { print " NOT GPRMC OR GPGGA\n" if $debug };


#  close($sock)
  }   # close infinite while

