#ifdef DEBUG
#include <stdio.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include "shared.h"
#include "convert.h"
#include "defines.h"

void check_primality ( child_id_t id, fict_clock_t *startclock, slot_t *slot )
{
	fict_clock_t nowclock;

	if ( slot->sequence <= 1 )
	{
		slot->result = -slot->sequence;
		return;
	}
	else if ( slot->sequence <= 3 )
	{
		slot->result = slot->sequence;
		return;
	}
	else if ( slot->sequence % 2 == 0 || slot->sequence % 3 == 0 )
	{
		slot->result = -slot->sequence;
		return;
	}

	int i = 5;

//#ifdef DEBUG
//	while ( 1 )
//#else
	while ( i * i <= slot->sequence )
//#endif
	{
//#ifdef DEBUG
//#else
		if ( slot->sequence % i == 0 || slot->sequence % ( i + 2 ) == 0 )
		{
			slot->result = -slot->sequence;
			return;
		}

		i = i + 6;
//#endif

		SH_read_clock ( &nowclock );

		if ( abs ( nowclock.seconds - startclock->seconds ) != 0
				|| abs ( nowclock.nanos - startclock->nanos ) > NANOS_PER_MILISECOND )
		{
			slot->result = -1;
			return;
		}
	}

	slot->result = slot->sequence;
}

void cleanup ()
{
	SH_clean ( false );
}

void init ( int argc, char *argv[], child_id_t *id, int *sequence, fict_clock_t *clock )
{
	// CHILD_PROCESS_PARAMS are aware about NULL terminator. But inside child process NULL terminator
	// doesn't exists.

	if ( argc != CHILD_PROCESS_PARAMS - 1 )
	{
		exit ( EXIT_FAILURE );
	}

	if ( !CV_str_to_int ( argv [ 1 ], id ) )
	{
		exit ( EXIT_FAILURE );
	}

	if ( !CV_str_to_int ( argv [ 2 ], sequence ) )
	{
		exit ( EXIT_FAILURE );
	}

	if ( !SH_init ( 0, false ) )
	{
		exit ( EXIT_FAILURE );
	}

	SH_read_clock ( clock );
}

int main ( int argc, char *argv[] )
{
	child_id_t   id;
	int          sequence;
	fict_clock_t clock;
	slot_t slot;

//#ifdef DEBUG
//	int i = 0;
//	for (; i < argc; i++ )
//	{
//		printf ( "argv[%i]=%s\n", i, argv [ i ] );
//	}
//#endif

	init ( argc, argv, &id, &sequence, &clock );

//#ifdef DEBUG
//	while ( 1 )
//	{
		// Give a chance to this child process to be killed.

//		sleep ( 1 );
//	}
//#endif

//#ifdef DEBUG
//	printf ( "id=%i\n", id );
//	printf ( "sequence=%i\n", sequence );
//	printf ( "clock.seconds=%i\n", clock.seconds );
//	printf ( "clock.nanos=%i\n", clock.nanos );
//#endif

	slot.sequence = sequence;

	check_primality ( id, &clock, &slot );

#ifdef DEBUG
	printf ( "slot.sequence=%i\n", slot.sequence );
	printf ( "slot.result=%i\n", slot.result );
#endif

	SH_write_slot ( id, &slot );

	exit ( EXIT_SUCCESS );
}
