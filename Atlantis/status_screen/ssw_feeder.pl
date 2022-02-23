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
#   20130417       lstolp
#                  add options for udp data sent from dsLog
#
#
#   20130126       asimoneau and lstolp
#                  - general clean up similar to rmc_feeder.pl
#
#
#   20130129       ams and lws
#                  -moved database connect and disconnect around
#                  -now fuser shows the the database as not constantly open
#                  
#   20130504       lws
#                  - grabbing sal and sst to create ssv for multibeam
#
#  version 201300504
#



use strict;
use warnings;
use DBI;
use IO::Socket::INET;
use Getopt::Long;


my $debug = 0;

# udpport number
my $port = "55904";

# sqlite db setup
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/ssw.db";
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
my $chr;
my $sbe45;
my $sbe48;
my %ssw = ();
my @keys;


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
        $type = $strings[3];

          if ($type eq "SBE48")
          {
          if (defined $strings[4] && $strings[4] eq "#")
                {
                  print "GOT  SBE48 \n " if ($debug);
                  $field1 = $strings[2];
                  $sbe48 = $strings[5];
                  chop($sbe48);
                  $ssw{$type}=$sbe48;
               }
         }
           elsif ($type eq "SBE45")
            { 
              $field1 = $strings[2]; 
              $sbe45 = $strings[6];
              chop($sbe45);
              $ssw{$type}=$sbe45;
             }

            else { print "no matches\n" if ($debug);}

my $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_d
ata
        BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
        END");
 
      
    @keys = keys %ssw;
      foreach my $key (@keys)
    {

              $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
              VALUES ('$key', '$field1', '$ssw{$key}')");
     }

  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();

      close($sock)

       }


