/* ----------------------------------------------------------------------

tcp thread for carl camera status

Modification History:
DATE        AUTHOR COMMENT
17 oct 2006  jch    create and write

---------------------------------------------------------------------- */



/* standard ansi C header files */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdarg.h>

/* posix header files */
#define  POSIX_SOURCE 1

#include <pthread.h>
#include <termios.h>
#include <unistd.h>

#include "skel.h"
#include "tcp_utilities.h"

void error( int status, int err, const char *fmt, ... )
{
	va_list ap;

	va_start( ap, fmt );
	fprintf( stderr, "tcp client: " );
	vfprintf( stderr, fmt, ap );
	va_end( ap );
	if ( err )
		fprintf( stderr, ": %s (%d)\n", strerror( err ), err );
	if ( status )
		EXIT( status );
}



/* set_address - fill in a sockaddr_in structure */
 void set_address( const char *hname, int  sname,struct sockaddr_in *sap )
{
	
	struct hostent *hp;
	printf(" host  %s port %d\n",hname,sname);
	
   // make this tcp specific!
	bzero( sap, sizeof( *sap ) );
	sap->sin_family = AF_INET;
	if(!strcmp(hname, "INADDR_ANY")){
	   printf("setting address to any...\n");
		sap->sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else if ( hname != NULL )
	{
		if ( !inet_aton( hname, &sap->sin_addr ) )
		{
			hp = gethostbyname( hname );
			if ( !hp )
				error( 1, 0, "unknown host: %s\n", hname );
			sap->sin_addr = *( struct in_addr * )hp->h_addr;
		}
	}
	sap->sin_port = htons( (unsigned short)sname );
	
	//sap->sin_port = (unsigned short)sname ;
	printf(" the sin port is  %d %d %d\n",sname,(unsigned short)sname,sap->sin_port);
	
}


