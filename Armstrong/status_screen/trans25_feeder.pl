#!/usr/bin/perl
#
#   lstolp  transmissometer version.. working through the math
#
#

use strict;
use warnings;
use DBI;
use IO::Socket::INET;
use Getopt::Long;

#$| = 1;

my $debug = 0;

# udpport number
my $port = "55508";

# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
my $username;
my $password;
my $type;
my $udp_type;

# misc variables to initialize
my @messages;
my @strings;
my $field1;
my $field2;
my $mesg;
my $signal;

# transmissometer variables
my $Vsig;
my $VdarkS=0;
my $Vref=15451;   # calibration sheet
my $Vdark=0;
my $Vair=15809;
my $VairS=15420;  # light/air ship calibration Signal 
my $Tr;
my $bac;
my $z=.25;

# get options from the command line 
GetOptions('debug' => \$debug, 'port=s' => \$port, 'database=s' => \$db_name,
         'udptype=s' => \$udp_type); 

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


while (1) {
        my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );
        $sock->recv($mesg, 180);
        print STDERR "\n\n Start of Loop Input message: $mesg" if $debug;
        @strings = split (" ", $mesg);
        print "\@strings = @strings\n" if $debug;
        next if ($strings[6] == 0);
       	$field1 = $strings[2];
	$signal = $strings[6];

# Transmittance Ratio (Tr) = [(Vsig – VdarkS)/(Vref - Vd)]*[(Vair – Vd)/(VairS - VdarkS)]
     $Tr = (($signal - $VdarkS)/($Vref - $Vdark))*(($Vair - $Vdark)/($VairS - $VdarkS));
my $logtry = log($Tr);

# Calculate Beam Attenuation
     $bac = -1/$z * $logtry;



print STDERR "\$logtry=$logtry\n" if $debug;
print STDERR "\$signal = $signal\n" if $debug;
print STDERR "\$Tr = $Tr\n" if $debug;
print STDERR "\$bac = $bac\n" if $debug;


my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");
 
       
              
	       	print "field1 = $field1;\n" if $debug;
		print "string = $strings[6]\n" if $debug;
		$dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              	VALUES ('VSIG25', '$field1', '$Tr')");
		


  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();

      close($sock)

       }


