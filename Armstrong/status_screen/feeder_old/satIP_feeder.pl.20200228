#! /usr/bin/perl 
#
# 20110304   lstolp  took out -w and debug
#                    too many error messages
#
# 20200228   lstolp   using command line wget to  wtfismyip
#                     this query should go out the weightd balance 
#                     peplin rule   
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

my $route;
my $debug = 0;


$db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
  
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
my $route = LWP::Simple::get("https://wtfismyip.com/text") or warn "0.0.0.0";
chomp $route;





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


