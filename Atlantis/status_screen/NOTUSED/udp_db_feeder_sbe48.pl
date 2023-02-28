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
#   20130129       ams and lws
#                  -moved database connect and disconnect around
#                  -now fuser shows the the database as not constantly open
#                  
# 
#  version 2015FEB06  - select which SSV to use from udp 55500
#  

use strict;
use warnings;
use DBI;
use IO::Socket::INET;
use Getopt::Long;

#my $debug = 0;
my $debug = 1;
my $port = "";
my $db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
my $username;
my $password;
my $type = "";
my $udp_type = "";

# misc variables to initialize
my $mesg = "";
my $host;
my @messages;
my @strings;
my $field1 = "";
my $field2 = "";
my $i;
my @data;
my $data;


GetOptions('debug' => \$debug, 'port=s' => \$port, 'database=s' => \$db_name,
        'udptype=s' => \$udp_type,);


print STDERR "\$udp_type = $udp_type\n" if ($debug);
#print "\$udp_type = $udp_type\n" if ($debug);
#print "\$udp_type = $udp_type\n" if ($debug);

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



#my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );


while (1) {

        my $sock = IO::Socket::INET->new(Proto => 'udp', LocalPort => $port, Reuse => 1 );
        $sock->recv($mesg, 180);
        print STDERR "\n\n Start of input message: $mesg\n" if $debug;


                make_entry();
}

sub make_entry {

print STDERR "at start of loop \$udp_type = $udp_type\n" if ($debug);

             if ($udp_type eq "gps") {
                @strings = split (" ", $mesg);
                $type = $strings[3];
                $field1 = $strings[2];
                if ($strings[4] =~ /\$GPGGA/)
                {
                $field2 = $strings[4];
                }       

             }
             elsif ($udp_type eq "gyro") {
                @strings = split (" ", $mesg);
                $type = $strings[3];
                $field1 = $strings[2];
                @data = split (",", $strings[4]);
                $field2 = $data[1];
                sleep(1);
             }

          elsif ($udp_type eq "spd") {
                @strings = split (" ", $mesg);
                $type = $strings[3];
                $field1 = $strings[2];
 # if string4 is not defined, don't do anything
                   if (!defined($strings[4])) { 
                      print "string4 not defined\n" if $debug;
                       }
#                   elsif ($strings[4] =~ /\$VDVBW/)
                   elsif ($strings[4] =~ /VDVBW/)
                      {
                      @data = split (",", $strings[4]);
                      $field2 = $data[1];
	              }
                   else {return;} 
                      
            }
            elsif ($udp_type eq "met") {
                @strings = split (" ", $mesg);
                $type = $strings[3];
                $field1 = $strings[2];
                $field2 = $strings[4];
             }

# take the value of the swr string
            elsif ($udp_type eq "shw") {
                @strings = split (" ", $mesg);
                $type = $strings[3];
                $field1 = $strings[2];
                $field2 = $strings[4];

             }

# take the last value of the lwr string
            elsif ($udp_type eq "lwr") {
                @strings = split (" ", $mesg);
                $type = $strings[3];
                $field1 = $strings[2];
                $field2 = $strings[7];

             }


# take the value of the depth from the em122 string 
#the string looks like $KIDPT,x.x,x.x.x.x*hh<CR><LF>
#I think the first x.x is the one we want
            elsif ($udp_type eq "em122") {
                @strings = split (" ", $mesg);
                $type = $strings[3];
                $field1 = $strings[2];
                      print "string4 $strings[4]\n" if $debug;
                @data = split (",", $strings[4]);
                $field2 = $data[1] + $data[2];
#                $field2 = $data[1] + 5.27;
             }

# drop the * and + from the front of the flr string
            elsif ($udp_type eq "flr") {
                @strings = split (" ", $mesg);
                $type = $strings[3];
                $field1 = $strings[2];
                $field2 = substr($strings[4], 2);
             }

            elsif ($udp_type eq "SBE48") {
                @strings = split (" ", $mesg);
                $type = $strings[3];
print STDERR "SBE48 should be the type, the type is $type  \n" if $debug;
#print STDERR "strings4 is $strings[4]  \n" if $debug;
                if (defined $strings[4] && $strings[4] eq "#")
                {
                  $field1 = $strings[2];
                  $field2 = $strings[5];
                  my $chr = chop($field2);
print STDERR "SBE48 values are $type $field1 $field2 \n" if $debug;
                } 
             } else {
                   print STDERR "$udp_type not defined SBE48\n" if $debug; 
                   print STDERR "the string is $mesg \n" if $debug;
                   @strings = split (" ", $mesg);
                   print STDERR "variable type is *$strings[3]*  \n" if $debug;

                   print STDERR "is there a pound sign *$strings[4]*  \n" if $debug;
                   print STDERR "data $strings[5]  \n" if $debug;
                    }
                    

# put data into database

            $dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password,
                { PrintError => 0} ) || die "Cannot connect.";

         my $trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
         BEGIN
            UPDATE latest_data SET timeEnter = strftime('%s', 'now')
                        WHERE rowid = new.rowid;
         END");

         print "Table insert: type: $type field1: $field1 field2: $field2\n" if $debug;
          
         $dbh->do( "INSERT OR REPLACE INTO latest_data (header, udp_time, data)
               VALUES ('$type', '$field1', '$field2')");


  # write immediately to the database
  $dbh->do("PRAGMA synchronous = OFF");
  $dbh->disconnect();

}


