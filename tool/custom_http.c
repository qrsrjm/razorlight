#include <stdio.h>
#include "cnhttp.h"
#include "http_bsd.h"
#include <string.h>
/*
int issue_command(char * buffer, int retsize, char *pusrdata, unsigned short len)
{
	char * buffend = buffer;
	pusrdata[len] = 0;

	switch( pusrdata[0] )
	{
	case 'e': case 'E':
		if( retsize > len )
		{
			memcpy( buffend, pusrdata, len );
			return len;
		}
		else
		{
			return -1;
		}
	}
	return -1;
}
*/

static void huge()
{
	uint8_t i = 0;

	DataStartPacket();
	do
	{
		PushByte( 0 );
		PushByte( i );
	} while( ++i ); //Tricky:  this will roll-over to 0, and thus only execute 256 times.

	EndTCPWrite( curhttp->socket );
}


static void echo()
{
	char mydat[128];
	int len = URLDecode( mydat, 128, curhttp->pathbuffer+8 );

	DataStartPacket();
	PushBlob( mydat, len );
	EndTCPWrite( curhttp->socket );

	curhttp->state = HTTP_WAIT_CLOSE;
}

static void issue()
{
	uint8_t  __attribute__ ((aligned (32))) buf[1300];
	int len = URLDecode( buf, 1300, curhttp->pathbuffer+9 );
	printf( "%d\n", len );
	int r = issue_command(buf, 1300, buf, len );
	if( r > 0 )
	{
		printf( "BD: %d\n", r );
		DataStartPacket();
		PushBlob( buf, r );
		EndTCPWrite( curhttp->socket );
	}
	curhttp->state = HTTP_WAIT_CLOSE;
}


void HTTPCustomStart( )
{
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/huge", 7 ) == 0 )
	{
		curhttp->rcb = (void(*)())&huge;
		curhttp->bytesleft = 0xffffffff;
	}
	else
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/echo?", 8 ) == 0 )
	{
		curhttp->rcb = (void(*)())&echo;
		curhttp->bytesleft = 0xfffffffe;
	}
	else
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/issue?", 9 ) == 0 )
	{
		curhttp->rcb = (void(*)())&issue;
		curhttp->bytesleft = 0xfffffffe;
	}
	else
	{
		curhttp->rcb = 0;
		curhttp->bytesleft = 0;
	}
	curhttp->isfirst = 1;
	HTTPHandleInternalCallback();
}



void HTTPCustomCallback( )
{
	if( curhttp->rcb )
		((void(*)())curhttp->rcb)();
	else
		curhttp->isdone = 1;
}




static void WSEchoData(  int len )
{
	char cbo[len];
	int i;
	for( i = 0; i < len; i++ )
	{
		cbo[i] = WSPOPMASK();
	}
	WebSocketSend( cbo, len );
}



static void WSCommandData(  int len )
{
	uint8_t  __attribute__ ((aligned (32))) buf[1300];
	int i;

	for( i = 0; i < len; i++ )
	{
		buf[i] = WSPOPMASK();
	}

	i = issue_command(buf, 1300, buf, len );
	if( i < 0 ) i = 0;

	WebSocketSend( buf, i );
}



void NewWebSocket()
{
	if( strcmp( (const char*)curhttp->pathbuffer, "/d/ws/echo" ) == 0 )
	{
		curhttp->rcb = 0;
		curhttp->rcbDat = (void*)&WSEchoData;
	}
	else if( strncmp( (const char*)curhttp->pathbuffer, "/d/ws/issue", 11 ) == 0 )
	{
		curhttp->rcb = 0;
		curhttp->rcbDat = (void*)&WSCommandData;
	}
	else
	{
		curhttp->is404 = 1;
	}
}




void WebSocketTick()
{
	if( curhttp->rcb )
	{
		((void(*)())curhttp->rcb)();
	}
}

void WebSocketData( int len )
{	
	if( curhttp->rcbDat )
	{
		((void(*)( int ))curhttp->rcbDat)(  len ); 
	}
}
/*
int main()
{
	RunHTTP( 8888 );

	while(1)
	{
		TickHTTP();
		usleep( 30000 );
	}
}
*/
