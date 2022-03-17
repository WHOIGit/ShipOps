#!/usr/bin/perl
#  
#  asimoneau and lstolp                         20130119
#  
#  Program to grab SBE45S and SBE48T from database, make SSV and put it back into database
#
#
#
#  20130126  lstolp   put use strict and warnings back in.. cleaned up script accordingly 
#  20130127  lstolp   added --debughash and --debug  cleaned up debugging
#                     changed time to be only time and not entire string
#                     -- changed name to ssv_feeder.pl for consistency   
#
#
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
use Getopt::Long;
use Time::gmtime;
use Physics::Water::SoundSpeed;


# misc variables
my $debug = 0;
my $debughash = 0;  # if turn this on will show entire hash
my $nowdate;
my $nowtime;
my %data;
my $msg;
my $tens;
my $decimal;
my $ssv;
my @ssv;
my $ssvf;
my $dbh;

# sqlite3 parameters
my $dsdb = "datascreen.db";
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/squlite_database/"."$dsdb";
my $username;
my $password;

#getoptions from the command line 
GetOptions('debug' => \$debug, 'debughash' => \$debughash, 'database=s' => \$db_name); 


while (1)
{
### OPEN data base and get data
## check data is within the last minute??

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can not connec
t.";

# time variable to test how old the data in the db is.. 
my $ltime = localtime;
my ($wkday, $month, $day, $time, $yr) = split(" ",$ltime);

print "\n\nThe start of the while loop \$time = $time\n" if $debug;

my $data = $dbh->selectall_arrayref("SELECT header,data from latest_data");

# @$ is a reference to an array
# create a hash of the data from the reference array
foreach my $row (@$data) {
    my ($key, $value) = @$row;
    $data{$key} = $value;
}

# write immediately to the database
$dbh->do("PRAGMA synchronous = OFF");

# disconnect from the database before starting loop
$dbh->disconnect();

# this is mostly a debug statement to see what your hash looks like
while ( my ($key, $value) = each(%data) ) {
        print "$key => $value\n" if $debughash;
      }



my $obj = new Physics::Water::SoundSpeed();
my $ssv = $obj->sound_speed_sea_tps( $data{SBE48}, 1, $data{SBE45S} );
 $ssvf = sprintf("%.2f", $ssv);
 print "\$ssvf = $ssvf \n" if $debug;


@ssv = split (/\./, $ssvf);
my $dec = substr ($ssv[1], 0, 1 );
$ssv = "S"."$ssv[0]"."$dec";


print "\$ssv = $ssv\n" if $debug; 

# stuff SV into the database for display and plotting
$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Cannot connect.";

   $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('SSV', '$time', '$ssvf')");

# write immediately to the database
$dbh->do("PRAGMA synchronous = OFF");

# disconnect from the database before starting loop
$dbh->disconnect();

sleep (1);  #need to sleep for a second or it sends the data too fast


}
 
