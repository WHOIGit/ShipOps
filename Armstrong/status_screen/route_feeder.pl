#! /usr/bin/perl 
#
# 20110304   lstolp  took out -w and debug
#                    too many error messages
#
#
# 
#   20130129       ams and lws
#                  -moved database connect and disconnect around
#                  -now fuser shows the the database as not constantly open
#                  
#   20130210       lstolp
#                  -updated for use with port and stbd fbbs
#
#
#  version 20130129
#


use DBI;
use Getopt::Long;
use strict;
use warnings;




my $debug = 0;
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/datascreen.db";
my $username;
my $password;

# misc variables to intialize
my ($hours, $mesg, $route);
my ($table_result, $nexthop, $routeage, @hop_list, @age_list);


# number of hours fbb can be up before Alarms go off :)
# Change this number to 99999 if you expect to be on FBB for any legnth of time.
# Use iptables_restricted browsing 
# When back in HSN change number back to 1, this 1 is number of hours FBB has been up
# for...

my $num = 1;

# next hop options will display on status screen
my $shore = "128.128.200.236";
my $hsn_fbb = "192.168.252.210";
my $hsn = "128.128.252.210";
my $fbb = "192.168.0.1";
my $ka = "192.168.200.1";

# get options from the command line 
GetOptions('debug' => \$debug, 'database=s' => \$db_name );


my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can not connect.";

$table_result = $dbh->do( "CREATE TABLE latest_data (
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

 $nexthop = ( `snmpget -v 2c -c sssg 128.128.252.17 RFC1213-MIB::ipRouteNextHop.0.0.0.0  2>&1` );
 $routeage= ( `snmpget -v 2c -c sssg 128.128.252.17 RFC1213-MIB::ipRouteAge.0.0.0.0  2>&1` );

 print STDERR  "SNMPget: $nexthop\n" if $debug;
 print STDERR  "SNMPget: $routeage\n" if $debug;

 @hop_list = split /:/, $nexthop;
 @age_list = split /:/, $routeage;

 my $next = strip($hop_list[3]);

 if ($next eq $shore )
   { $route = "shore"; }
 if ($next eq $hsn )
   { $route = "HSN"; }
 if ($next eq $hsn_fbb )
   { $route = "STBD FBB limited"; }
 if ($next eq $fbb )
   { $route = "T&T FBB"; }
 if ($next eq $ka)
   { $route = "Ka Band"; }



 print STDERR  "\$route = $route\n" if $debug;
 print STDERR  "nexthop: $hop_list[3]\n" if $debug;
 print STDERR  "routeage: $age_list[3]\n" if $debug;

 
 $hours =  sprintf("%12.3f", $age_list[3]/3600);
 $hours = strip($hours);

 if ($hop_list[0] eq "Timeout: No Response from cisco.") {
        print STDERR  "$hop_list[0]\n" if $debug;
        sleep (15);
 }

  my $ltime = localtime;
  my ($wkday, $month, $day, $time, $yr) = split(" ",$ltime);




$mesg = "$route, $hours";

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");



$dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
                VALUES ('Networkuse', '$time', '$mesg')");


$dbh->do("PRAGMA synchronous = OFF");
$dbh->disconnect();

sleep 5;
}


## remove whitespace
sub strip{
    my @out = @_;
    for (@out) {
        s/^\s+//;
        s/\s+$//;
    }
    return wantarray ? @out : $out[0];
  }


