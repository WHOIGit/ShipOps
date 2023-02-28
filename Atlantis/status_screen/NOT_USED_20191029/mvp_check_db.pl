#!/usr/bin/perl 
#  check_db.pl
#
#  check database for timing of inputs        lstolp 20130126
#
#  20130201  lstolp  checks the database for age of data
#            prints out: 
#            current computer time, key, time from UDP (dslog), value, lapsetime
#            lapsetime should be current epoch time - epoch time from database tridder value
#  20170613  lstolp updated to use tmpfs for sqlite database
#
#
use strict;
use DBI;

my $debug = 1;

# sqlite3 parameters
my $dsdb = "mvp.db";
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/"."$dsdb";
my $dbh;
my $username;
my $password;
my %data;


while (1)
{

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can not connect.";

print STDERR "Database connected: \n" if $debug;

my $data = $dbh->selectall_arrayref("SELECT header,udp_time,data,timeEnter from latest_data");

# @$ is a reference to an array
# create a hash of the data from the reference array
foreach my $row (@$data) {
    my ($key, $dbtime, $value, $timeEnter) = @$row;

    my $epoch_time = time;
    my $time = localtime;
    my @now = split(" ",$time);

   my $lapsetime = $epoch_time - $timeEnter;




    print "$now[3] $key  $dbtime $value  $lapsetime\n" if $debug;
}

$dbh->disconnect();

exit;

}





