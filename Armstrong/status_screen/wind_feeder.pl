#!/usr/bin/perl 
#  Program to figure out True Wind and Direction from Port Vaisala
#
#
#  20161010  lstolp
#         - rewote samos cprogram to compute wind
#         - changed database to tmpfs filesystem 
#
#
#

use strict;
use warnings;
use Math::Trig ':pi';
use DBI;
use Getopt::Long;



my $debug = 0;
my $debughash = 0;

# sqlite3 parameters
my $dsdb = "datascreen.db";
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/"."$dsdb";
my $dbh;
my $username;
my $password;
my %data;

# wind parameters
my $cog;      #course over ground crse in truewind.c
my $mcog;             #corrected cog.. mcrse in truewind.c
my $sog_inknots;       #speed over ground  cspd in truewind.c
my $hdt;        #heading from gyro  hd in truewind.c
my $zlr = 0;          # Zero line reference -- angle between bow and
                                 # zero line on anemometer.  Direction is clockwise
                                 # from the bow.  (Use bow=0 degrees as default 
                                 #  when reference not known.) 
my $wdir;     # wind direction measured by anemometer, referenced to the ship
my $wspd;      # wind speed measured by anemometer, referenced to the vessels
                               # frame of reference
my @wind;
my $wind;
my $svalue;
my $dvalue;


my $mtdir;           # correction for angle of true wind
my $dtor =  pi/180;  #degrees to radians conversion 


# get options from the command line 
GetOptions('debug' => \$debug, 'debughash' => \$debughash);




while (1)
{

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can not connect.";

print STDERR "\nStart of while loop Database connected: ", $dbh, "\n" if $debug;

# time variable to test how old the data in the db is.. 
my $time = localtime;
#my ($wkday, $month, $day, $time, $yr) = split(" ",$ltime);
my $epoch_time = time;

my $data = $dbh->selectall_arrayref("SELECT header,data,timeEnter from latest_data");


# @$ is a reference to an array
# create a hash of the data from the reference array
foreach my $row (@$data) {
   my ($key, $value, $timeEnter) = @$row;
    my $lapsetime = $epoch_time - $timeEnter;

   # figure out how old the data is
    print "The \$key is $key
                   \$value = $value
                   \$epoch_time = $epoch_time 
                   \$timeEnter = $timeEnter 
                   \$lapsetime =  $lapsetime\n\n" if $debug;

# if greater than 45 use nan
    if ($lapsetime > 45 )
     {
      $value = "nan";
     }
    $data{$key} = $value;
}


$dbh->disconnect();

# this is mostly a debug statement to see what your hash looks like
while ( my ($key, $value) = each(%data) ) {
        print "$key => $value\n" if ($debughash);
      }

$sog_inknots = $data{SOG};
$cog = $data{COG};
$hdt = $data{GYRO};

@wind = ("port", "stbd");
#@wind = ("stbd");

foreach $wind (@wind)
 { 
   if ($wind eq "port")
      {      
       print "Using Port Vaisala\n" if $debug;
       $wdir = $data{WXTP_Dm};
       $wspd = $data{WXTP_Sm};
       $dvalue = "WXTP_TD";
       $svalue = "WXTP_TS";
       print "\$wdir = $data{WXTP_Dm}\n" if $debug;
       print "\$wspd = $data{WXTP_Sm}\n" if $debug;
       break_wind();


      }
    elsif ($wind eq "stbd")
      {
       print "\nUsing STBD Vaisala\n" if $debug;
       $wdir = $data{WXTS_Dm};
       $wspd = $data{WXTS_Sm};
       $dvalue = "WXTS_TD";
       $svalue = "WXTS_TS";
       print "\$wdir = $data{WXTS_Dm}\n" if $debug;
       print "\$wspd = $data{WXTS_Sm}\n" if $debug;
       break_wind()
        }



sub break_wind
{
#       convert sog into meters per second using 1 Knot = 0.514 Meters per Second 
        my $sog = $sog_inknots * .514;

         
#       convert from navigational coordinates to angles commonly used 
#         in mathematics 
         $mcog = 90.0 - $cog;   

#       keep the value between 0 and 360 degrees  
         if( $mcog <= 0.0 ) {$mcog = $mcog + 360.0;}

#	 check zlr for valid value.  If not valid, set equal to zero.
	 if( ($zlr < 0.0) || ($zlr > 360.0) ) {$zlr = 0.0;}
	 
#	 calculate apparent wind direction  
	 my $adir = $hdt + $wdir + $zlr;
	 
#	 keep adir between 0 and 360 degrees  
	 if ( $adir >= 360.0 ) { $adir = $adir - 360.0;	 }

#       convert from meteorological coordinates to angles commonly used   
#         in mathematics  
         my $mwdir = 270.0 - $adir;

#       keep the value between 0 and 360 degrees 
         if( $mwdir <= 0.0 ) 
             {
              $mwdir = $mwdir + 360.0; 
         } elsif( $mwdir > 360.0 ) 
             {$mwdir = $mwdir - 360.0;
             }

#       determined the East-West vector component and the North-South
#         vector component of the true wind 
         my $x = ($wspd * cos($mwdir * $dtor)) + ($sog * cos($mcog * $dtor));
         my $y = ($wspd * sin($mwdir * $dtor)) + ($sog * sin($mcog * $dtor));

#       use the two vector components to calculate the true wind speed  */
         my $tspd = sqrt(($x * $x) + ($y * $y));
         my $calm_flag = 1;

#       determine the angle for the true wind  */
         if(abs($x) > 0.00001) 
             { 
              $mtdir = (atan2($y,$x)) / $dtor;
             } else {
               if(abs($y) > 0.00001) 
                 {
                  $mtdir = 180.0 - (90.0 * $y) / abs($y);
                 } else {
#            the true wind speed is essentially zero: winds are calm
#              and direction is not well defined    
               $mtdir = 270.0;
               $calm_flag = 0;
              }
           }

#       convert from the common mathematical angle coordinate to the 
#         meteorological wind direction  
         my $tdir = 270.0 - $mtdir;

#       make sure that the true wind angle is between 0 and 360 degrees 
         if ($tdir < 0.0)  { $tdir = ($tdir + 360.0) * $calm_flag;}
         if ($tdir > 360.0) {$tdir = ($tdir - 360.0) * $calm_flag;}

      my $ltime = localtime;
      my ($wkday, $month, $day, $time, $yr) = split(" ",$ltime);

         $tspd = sprintf("%.2f", $tspd);
         $tdir = sprintf("%.2f", $tdir);
#	 print "testing \$tdir = $tdir\n" if ($debug);

	
        if ($tdir == 0.00)
           {
             $tdir = "nan";

        } else {

	$tdir = $tdir;	

	}
         
         print "$time \$tspd = $tspd\n" if ($debug);
         print "$time \$tdir = $tdir\n" if ($debug);

# stuff stbd wind  into the database for display and plotting
  $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Cannot connect.";

   $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$svalue', '$time', '$tspd')");

   $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$dvalue', '$time', '$tdir')");

 }

 $dbh->do("PRAGMA synchronous = OFF");
 $dbh->disconnect();


 } #end of foreach
sleep (1);
} # end of infinite while




