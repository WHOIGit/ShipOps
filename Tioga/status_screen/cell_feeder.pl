#! /usr/bin/perl 
#
# 20110304   lstolp  took out -w and debug
#                    too many error messages
#
#
# 


use DBI;
use Getopt::Long;
use strict;
use LWP::Simple;

my $db_name;
my $username;
my $password; 
my $dell;
my $dbh;
my $fbb;

my $route;
my $cell;
my $shore;
my $debug = 0;


$db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";

# next hop options will display on status screen
$shore = "128.128.200.206";
$cell = "166.143.120.129";
$fbb = "216.86.245.186";

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can no
t connect.";

my $table_result = $dbh->do( "CREATE TABLE latest_data (
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

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
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
my $content = LWP::Simple::get("https://wtfismyip.com/text") or warn "0.0.0.0";
chomp $content;
       
if ($content eq $shore )
   { $route = "shore"; }
 if ($content  eq $cell )
   { $route = "cell"; }
 if ($content eq $fbb )
   { $route = "FBB limited"; }
 

 

$dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
                VALUES ('Networkuse', '00:00:00', '$route')");

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


