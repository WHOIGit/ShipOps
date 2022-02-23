#!/bin/sh

datadir=/usr/lib/cgi-bin/db_driven_data/status_screen
process=winch_feeder.pl
daem=winchd

# See how we were called.
case "$1" in
  start)
        if [ -f /var/run/$daem ] ; then
           pid=`cat /var/run/$daem`
           if [ -d /proc/$pid ] ; then
             echo "$daem already running"
             echo failure
             exit 1
           fi
        fi
        echo -n "Starting $daem: "
        $datadir/$process &> /dev/null 2>&1
        pid=`pidof -o $$ -o $PPID -o %PPID -x $process`
        echo $pid > /var/run/$daem
        RETVAL=$?
        if [ $RETVAL -eq 0  ] ; then
          touch /var/lock/$daem
          echo success
        else
          echo failure
        fi
        echo
        ;;
  stop)
     echo ""
      if [ -f /var/run/$daem ] ; then
        echo -n "Stoping $daem: "
        /bin/kill `cat /var/run/$daem`
        rm -f /var/lock/$daem
        rm -f /var/run/$daem
        echo success
        echo
     else
        echo -n "No $daem running?"
        echo failure
        echo
     fi

        ;;
  restart|reload)
        $0 stop
        sleep 1
        $0 start
        ;;
  *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac

exit 0

