#include <fcntl.h>
#include <stddef.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string.h>
#include "shared.h"

// Shared memory segment id.

int   shmid;

// Shared memory.

void  *shmem;

// Semaphore for process synchronization.

sem_t *sem;

/*
 * SH_init: initializes semaphore and shared memory.
 *
 * [In]
 *
 *     childs: number of child processes.
 *     parent: indicates if this function is invoked by parent or child process.
 *
 * [Return]
 *
 *     True if all initialization succeded, false otherwise.
 */

bool SH_init ( unsigned int childs, bool parent )
{
	key_t        key;
	unsigned int size;
	int          shmflg;

	if ( parent )
	{
		size = sizeof ( clock_t ) + childs * sizeof ( slot_t );
	}
	else
	{
		size = 0;
	}

	if ( ( sem = sem_open ( SHARED_MEM_SEMAPHORE_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1 ) ) == SEM_FAILED )
	{
		return false;
	}

	// This is a key needed to get shared memory.

	if ( ( key = ftok ( ".", 'Y') ) == -1 )
	{
		return false;
	}

	// Alloc shared memory.

	if ( parent )
	{
		shmflg = IPC_CREAT | 0666;
	}
	else
	{
		shmflg = 0666;
	}

	if ( ( shmid = shmget ( key, size, shmflg ) ) == -1 )
	{
		return false;
	}

	// Attach shared memory.

	if ( ( shmem = shmat ( shmid, NULL, 0 ) ) == ( void* ) -1 )
	{
		return false;
	}

	// Initialize shared memory to all zeros.

	if ( parent )
	{
		memset ( shmem, 0, size );
	}

	return true;
}

/*
 *  SH_clean: cleans up semaphore and shared memory.
 */

void SH_clean ( bool parent )
{
	// Detach shared memory segment.

	if ( shmem != ( void* ) -1 )
	{
		shmdt ( shmem );
	}

	// Mark the segment to be destroyed.

	if ( shmid != -1 )
	{
		shmctl ( shmid, IPC_RMID, NULL );
	}

	if ( sem != SEM_FAILED )
	{
		sem_close ( sem );

		if ( parent )
		{
			sem_unlink ( SHARED_MEM_SEMAPHORE_NAME );
		}
	}
}

void SH_read_clock ( fict_clock_t *clock )
{
	sem_wait ( sem );

	clock->seconds = ( ( fict_clock_t* ) shmem )->seconds;
	clock->nanos   = ( ( fict_clock_t* ) shmem )->nanos;

	sem_post ( sem );
}

void SH_write_clock ( fict_clock_t *clock )
{
	sem_wait ( sem );

	( ( fict_clock_t* ) shmem )->seconds = clock->seconds;
	( ( fict_clock_t* ) shmem )->nanos   = clock->nanos;

	sem_post ( sem );
}

slot_t* SH_read_slot ( child_id_t childid, slot_t *slot )
{
	sem_wait ( sem );

	slot->sequence = ( ( slot_t* ) ( shmem + sizeof ( fict_clock_t ) + childid * sizeof ( slot_t ) ) )->sequence;
	slot->result   = ( ( slot_t* ) ( shmem + sizeof ( fict_clock_t ) + childid * sizeof ( slot_t ) ) )->result;

	sem_post ( sem );

	return slot;
}

void SH_write_slot ( child_id_t childid, slot_t *slot )
{
	sem_wait ( sem );

	( ( slot_t* ) ( shmem + sizeof ( fict_clock_t ) + childid * sizeof ( slot_t ) ) )->sequence = slot->sequence;
	( ( slot_t* ) ( shmem + sizeof ( fict_clock_t ) + childid * sizeof ( slot_t ) ) )->result   = slot->result;

	sem_post ( sem );
}
