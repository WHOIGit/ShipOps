#! /usr/bin/perl 
#
# 20110304   lstolp  took out -w and debug
#                    too many error messages
#
#
# 


use DBI;
use Getopt::Long;

#$debug = 1;
$db_name = "/var/www/cgi-bin/db_driven_data/status_screen/datascreen.db";

# number of hours fbb can be up before Alarms go off :)
# Change this number to 99999 if you expect to be on FBB for any legnth of time.
# Use iptables_restricted browsing 
# When back in HSN change number back to 1, this 1 is number of hours FBB has been up
# for...

$num = 1;

# next hop options will display on status screen
$shore = "128.128.200.235";
$fbb = "192.168.252.202";
$hsn = "10.200.2.193";

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can no
t connect.";

$table_result = $dbh->do( "CREATE TABLE latest_data (
              header  VARCHAR(10) PRIMARY KEY ,
              udp_time VARCHAR (10),
              data VARCHAR (180),
              timeEnter VARCHAR(120))" );

if ($debug) {
  if (!defined($table_result)) {
        print STDERR "Table already created\n";
  } else {
        print STDERR "Table created\n";
  }
}

$trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");

if ($debug) {
  if (!defined($trigger_result)) {
        print STDERR "Trigger already created\n";
  } else {
        print STDERR "Trigger created\n";
  }
}

while (1) {

 $nexthop = ( `snmpget -v 2c -c sssg 192.168.4.18 RFC1213-MIB::ipRouteNextHop.0.0.0.0  2>&1` );
 $routeage= ( `snmpget -v 2c -c sssg 192.168.4.18  RFC1213-MIB::ipRouteAge.0.0.0.0  2>&1` );

 print STDERR  "SNMPget: $nexthop\n" if $debug;
 print STDERR  "SNMPget: $routeage\n" if $debug;

 @hop_list = split /:/, $nexthop;
 @age_list = split /:/, $routeage;

 my $next = strip($hop_list[3]); 

 if ($next eq $shore )
   { $route = "shore"; }
 if ($next eq $hsn )
   { $route = "SWAP"; }
 if ($next eq $fbb )
   { $route = "FBB limited"; }
 

 
 print STDERR  "\$route = $route\n" if $debug;
 print STDERR  "nexthop: $hop_list[3]\n" if $debug;
 print STDERR  "routeage: $age_list[3]\n" if $debug;

 $hours =  sprintf("%12.3f", $age_list[3]/3600);

 if ($hop_list[0] eq "Timeout: No Response from cisco.") {
        print STDERR  "$hop_list[0]\n" if $debug;
        sleep (15);
 }


$mesg = "$route, $hours";

print STDERR  "Message: $mesg\n" if $debug;

$dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
                VALUES ('#Networkuse', '00:00:00', '$mesg')");



if ($route eq "FBB")
  {
   if ( $hours > $num )
      { 
         system ("echo 1 > /home/sssg/fbb/uptoolong")
      }
   if ( $hours < $num )
      {
	 system ("echo 0 > /home/sssg/fbb/uptoolong")
      }
 } 
else
  {
     system ("echo 0 > /home/sssg/fbb/uptoolong")
  }
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


