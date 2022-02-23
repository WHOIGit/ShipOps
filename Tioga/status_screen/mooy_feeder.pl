#!/usr/bin/perl 
#
#  Grabs information out of datascreen.db and creates true wind for 
#  both port and starboard vaisala.
#                                   lstolp 201302ish
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
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/"."$dsdb";
my $dbh;
my $username;
my $password;
my %data;

my $nitrate;
my $phosphate;
my $sbe45s;
my $sbe48;
my $time;

# get options from the command line 
GetOptions('debug' => \$debug, 'debughash' => \$debughash);




while (1)
{

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can not connect.";

print STDERR "\nStart of while loop Database connected: ", $dbh, "\n" if $debug;

# time variable to test how old the data in the db is.. 
#my $ltime = localtime;
#my ($wkday, $month, $day, $time, $yr) = split(" ",$ltime);
my $epoch_time = time;

my $data = $dbh->selectall_arrayref("SELECT header,data from latest_data");


$dbh->disconnect();
# @$ is a reference to an array
# create a hash of the data from the reference array
foreach my $row (@$data) {
    my ($key, $value) = @$row;
    $data{$key} = $value;
}

#$dbh->disconnect();

# this is mostly a debug statement to see what your hash looks like
while ( my ($key, $value) = each(%data) ) {
        print "$key => $value\n" if ($debughash);
      }

# get time 
my $ltime = localtime;
my ($wkday, $month, $day, $time, $yr) = split(" ",$ltime);




$sbe45s = $data{SBE45S};
$sbe48 = $data{SBE48};

# compute nitrate
$nitrate = 5.988 - (0.162 * $sbe45s);
$nitrate = sprintf("%.3f", $nitrate);

# computer phosphate
$phosphate = 0.524 - (0.0187 * $sbe48);
$phosphate = sprintf("%.3f", $phosphate);


# stuff stbd wind  into the database for display and plotting
  $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Cannot connect.";

   $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('NITRATE', '$time', '$nitrate')");

   $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('PHOSPHATE', '$time', '$phosphate')");

# }

 $dbh->do("PRAGMA synchronous = OFF");
 $dbh->disconnect();


sleep (1);
} # end of infinite while




