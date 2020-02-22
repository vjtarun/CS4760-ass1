#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <libgen.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include "shared.h"
#include "convert.h"
#include "defines.h"
#include "list.h"

#define OPT_HELP             'h'
#define OPT_MAX_CHILDS       'n'
#define OPT_RUN_CHILDS       's'
#define OPT_SEQUENCE         'b'
#define OPT_INCREMENT        'i'
#define OPT_OUTPUT           'o'

#define OPT_PRM_MAX_CHILDS   "max"
#define OPT_PRM_RUN_CHILDS   "run"
#define OPT_PRM_SEQUENCE     "start"
#define OPT_PRM_INCREMENT    "increment"
#define OPT_PRM_OUTPUT       "logfile"

// Real-time expired timer signal.

#define SIG_TIMER            SIGRTMIN

const unsigned short DEFAULT_MAX_CHILDS = 4;
const unsigned short DEFAULT_RUN_CHILDS = 2;
const int DEFAULT_SEQUENCE              = 2;
const int DEFAULT_INCREMENT             = 1;
const char *DEFAULT_OUTPUT              = "output.log";
const unsigned short MAX_CHILDS         = 20;
const char *CHILD_PATH                  = "./child";
const char *SEP                         = ": ";
const char *LF                          = "\n";
const long MAX_PROCESS_SECS             = 2;

// We can change this to get a slower/faster change.
	
//#ifdef DEBUG
//const time_t TIMER_SECONDS              = 10 * 60;
//const long TIMER_NANOS                  = 0;
//#else
const time_t TIMER_SECONDS              = 0;
const long TIMER_NANOS                  = 100000;
//#endif

// Increment in nanoseconds (fictional timer).
// 1 second      = 1 000 000 000 nanoseconds
// 1 milisecond  = 1 000 000 nanoseconds
// 1 microsecond = 1 000 nanoseconds

const long CLOCK_INCREMENT              = 100000000;

typedef struct option_s
{
	short max_childs;
	short run_childs;
	int   sequence;
	int   increment;
	char  *output;
	char  *program;
} option_t;

// Child process data.

typedef struct child_proc_s
{
	// Child process pid.

	pid_t pid;

	// Command line.

	char  **argv;
} child_proc_t;

// All the program's data.

typedef struct data_s
{
	// How many child processes are done (terminated).

	unsigned short childsdone;

	// How many child processes that have been terminated or are running.

	unsigned short childsspawned;

	// How many child processes currently running simultaneously.

	unsigned short childsrunning;

	// Next number to test for primality.

	int            currsequence;

	// Log file.

	FILE           *log;

	// Each child process spawned will go here.

	child_proc_t   **childs;

	// Id assigned to each child process to be spawned. It starts at zero for convenience.
	// e.g.: it can be used as array index.

	child_id_t     nextid;

	// Real-time timer.

	timer_t        timerid;

	// We need to know if timer was crated OK, so we can release it when we are done.

	int            timerok;

	// How many seconds (real-time) has passed from process start.

	fict_clock_t   rtc;

	// Interrupt from keyboard signal received.

	bool           interrupt;
} data_t;

// Declared here so cleanup function (called from atexit) can clean up options.

option_t options;

data_t data;

void add_time ( long nanos, fict_clock_t *clock )
{
	clock->nanos += nanos;

	if ( clock->nanos >= NANOS_PER_SECOND )
	{
		++clock->seconds;
		clock->nanos = 0;
	}
}

/*
 * signal_handler: handler for signals.
 * 
 * [In]
 * 
 *     signal: the signal to handle.
 */

void signal_handler ( int signal )
{
	fict_clock_t clock;

	if ( signal == SIG_TIMER )
	{
		add_time ( TIMER_NANOS, &data.rtc );

		SH_read_clock ( &clock );

		add_time ( CLOCK_INCREMENT, &clock );

		SH_write_clock ( &clock );
	}

	if ( signal == SIGINT )
	{
		// Force exit.

		data.interrupt = true;
	}
}

/*
 * print_error: prints a formatted error message.
 * 
 * [In]
 * 
 *     program: the name of this program.
 *     fn:      the name of the function that failed.
 */

void print_error ( char *program, char *fn )
{
	char *strp;

	if ( asprintf ( &strp, "%s : Error in function %s() ", program, fn ) == -1 )
	{
		fprintf ( stderr, "%s : Error in function %s() ", program, "print_error" );
		return;
	}

	perror ( strp );

	free ( strp );
}

/*
 * cleanup: free all allocated resources.
 */

void cleanup ()
{
	int i;

	if ( options.output )
	{
		free ( options.output );
	}

	if ( data.log )
	{
		fclose ( data.log );
	}

	if ( data.childs )
	{
		for ( i = 0; i < options.max_childs; i++ )
		{
			if ( data.childs [ i ] )
			{
				free ( data.childs [ i ]->argv [ 0 ] );
				free ( data.childs [ i ]->argv [ 1 ] );
				free ( data.childs [ i ]->argv );
				free ( data.childs [ i ] );
			}
		}

		free ( data.childs );
	}

	if ( data.timerok != -1 )
	{
		timer_delete ( data.timerid );
	}

	SH_clean ( true );
}

/*
 * print_usage: prints how to use this program.
 */

void print_usage ( const char *program )
{
	printf ( "\nUsage: %s [-%c] [-%c <%s> -%c <%s> -%c <%s> -%c <%s> -%c <%s>]\n\n",
		program, OPT_HELP, OPT_MAX_CHILDS, OPT_PRM_MAX_CHILDS, OPT_RUN_CHILDS, OPT_PRM_RUN_CHILDS, OPT_SEQUENCE,
		OPT_PRM_SEQUENCE, OPT_INCREMENT, OPT_PRM_INCREMENT, OPT_OUTPUT, OPT_PRM_OUTPUT );
	printf ( "where:\n\n" );
	printf ( "\t%c:             prints this help.\n", OPT_HELP );
	printf ( "\t%c <%s>:       maximum number of child processes it will be created.\n", OPT_MAX_CHILDS, OPT_PRM_MAX_CHILDS );
	printf ( "\t               If ommited, %u child processes will be created.\n", DEFAULT_MAX_CHILDS );
	printf ( "\t               There can't be more than 20 processes created.\n" );
	printf ( "\t%c <%s>:       number of child processes running at the same time.\n", OPT_RUN_CHILDS, OPT_PRM_RUN_CHILDS );
	printf ( "\t               If ommited, %u child processes will be running at the same time.\n", DEFAULT_RUN_CHILDS );
	printf ( "\t%c <%s>:     number where the sequence will start from.\n", OPT_SEQUENCE, OPT_PRM_SEQUENCE );
	printf ( "\t               If ommited, sequence will start from %i.\n", DEFAULT_SEQUENCE );
	printf ( "\t%c <%s>: increment to add to sequence to get next number.\n", OPT_INCREMENT, OPT_PRM_INCREMENT );
	printf ( "\t               If ommited, increment will be %i.\n", DEFAULT_INCREMENT );
	printf ( "\t%c <%s>:   file name where the log will be saved.\n", OPT_OUTPUT, OPT_PRM_OUTPUT );
	printf ( "\t               If ommited, file name will be '%s'.\n\n", DEFAULT_OUTPUT );
}

/*
 * init_options: initializes options struct.
 *
 * [Out]
 * 
 *     options: the options to be set.
 */

void init_options ( option_t *options )
{
	options->max_childs = DEFAULT_MAX_CHILDS;
	options->run_childs = DEFAULT_RUN_CHILDS;
	options->sequence   = DEFAULT_SEQUENCE;
	options->increment  = DEFAULT_INCREMENT;

	// Alloc memory so we can clean up.

	options->output     = strdup ( DEFAULT_OUTPUT );
	options->program    = NULL;
}

/*
 * parse_args: process the command line to find user's selected options.
 * 
 * [In]
 * 
 *     argc: how many arguments are in the command line.
 *     argv: the command line.
 * 
 * [Out]
 * 
 *     options: parsed user's selected options.
 * 
 * [Returns]
 * 
 *     True if successful, false otherwise.
 */

bool parse_args ( const int argc, char * const argv[], option_t *options )
{
	int opt;
	char opt_string[] = { OPT_HELP, OPT_MAX_CHILDS, ':', OPT_RUN_CHILDS, ':', OPT_SEQUENCE, ':',
		OPT_INCREMENT, ':', OPT_OUTPUT, ':', '\0' };

	atexit( &cleanup );

	// Initialize options

	init_options ( options );

	// Handy. Save program name.
	// Take only filename. Exclude directories.

	options->program = basename ( argv[0] );

	while ( ( opt = getopt ( argc, argv, opt_string ) ) != -1 )
	{
		switch ( opt )
		{
			case OPT_HELP:
				print_usage ( options->program );
				return false;

			case OPT_MAX_CHILDS:

				if ( !CV_str_to_short ( optarg, &options->max_childs ) )
				{
					fprintf ( stderr, "Invalid maximum number of child processes to create.\n" );
					return false;
				}

				break;

			case OPT_RUN_CHILDS:

				if ( !CV_str_to_short ( optarg, &options->run_childs ) )
				{
					fprintf ( stderr, "Invalid number of child processes to run at the same time.\n" );
					return false;
				}

				break;

			case OPT_SEQUENCE:

				if ( !CV_str_to_int ( optarg, &options->sequence ) )
				{
					fprintf ( stderr, "Invalid sequence start value.\n" );
					return false;
				}

				break;

			case OPT_INCREMENT:

				if ( !CV_str_to_int ( optarg, &options->increment ) )
				{
					fprintf ( stderr, "Invalid increment value.\n" );
					return false;
				}

				break;

			case OPT_OUTPUT:
				// Alloc memory so we can clean up.

				options->output = strdup ( optarg );
				break;

			default:
				print_usage ( options->program );
				return false;
		}
	}

	// Check if maximum number of child processes is greater than permited maximum.

	if ( options->max_childs > MAX_CHILDS )
	{
		options->max_childs = MAX_CHILDS;
		printf ( "Maximum number of child processes created changed to %u (max. %u).\n", options->max_childs, MAX_CHILDS );
	}

	// Number of running child processes can't be greater than total child processes.

	if ( options->run_childs > options->max_childs )
	{
		options->run_childs = options->max_childs;
		printf ( "Number of child processes running simultaneously changed to %u (max. %u).\n", options->run_childs,
			options->max_childs );
	}

	return true;
}

/*
 * create_timer: creates and sets a real time timer.
 *
 * [In]
 *
 *     signal:  expired timer generated signal identifier.
 *     seconds: number of seconds between signals.
 *     nanos:   number of nanoseconds between signals.
 *     handler: handler that handles expired timer generated signals.
 *
 * [Out]
 * 
 *     timer:   the new timer to be created.
 *     timerok: indicates if timer was created successfuly.
 *
 * [Return]
 *
 *     True if successful, false otherwise.
 */

bool create_timer ( timer_t *timerid, int *timerok, int signal, time_t seconds, long nanos, void ( *handler ) ( int ) )
{
	struct sigevent se;
	struct sigaction sa;
	struct itimerspec ts;

	sa.sa_flags   = 0;
	sa.sa_handler = handler;
	sigemptyset ( &sa.sa_mask );

	if ( sigaction ( signal, &sa, NULL ) == -1 )
	{
		return false;
	}

	se.sigev_notify          = SIGEV_SIGNAL;
	se.sigev_signo           = signal;
	se.sigev_value.sival_ptr = timerid;

	if ( ( *timerok = timer_create ( CLOCK_REALTIME, &se, timerid ) ) == -1 )
	{
		return false;
	}

	ts.it_value.tv_sec     = seconds;
	ts.it_value.tv_nsec    = nanos;
	ts.it_interval.tv_sec  = ts.it_value.tv_sec;
	ts.it_interval.tv_nsec = ts.it_value.tv_nsec;

	if ( timer_settime ( *timerid, 0, &ts, NULL ) == -1 )
	{
		return false;
	}

	return true;
}

/*
 * init_data: initializes all the date used by the process.
 *
 * [In]
 * 
 *     data:    data used by the process.
 *     options: the options chose by user.
 */

void init_data ( data_t *data, option_t *options )
{
	// Initialize shared memory.

	if ( !SH_init ( options->max_childs, true ) )
	{
		print_error ( options->program, "SH_init" );
		exit ( EXIT_FAILURE );
	}

	// Open output (log) file.

	if ( ( data->log = fopen ( options->output, "w" ) ) == NULL )
	{
		print_error ( options->program, "fopen" );
		exit ( EXIT_FAILURE );
	}

	// Alloc space for child processes.

	if ( ( data->childs = ( child_proc_t** ) malloc ( sizeof ( child_proc_t* ) * options->max_childs ) ) == NULL )
	{
		print_error ( options->program, "malloc" );
		exit ( EXIT_FAILURE );
	}

	// Initialize space.

	memset ( data->childs, 0, sizeof ( child_proc_t* ) * options->max_childs );

	if ( !create_timer ( &data->timerid, &data->timerok, SIG_TIMER, TIMER_SECONDS, TIMER_NANOS, signal_handler ) )
	{
		print_error ( options->program, "create_timer" );
		exit ( EXIT_FAILURE );
	}

	data->childsdone    = 0;
	data->childsspawned = 0;
	data->childsrunning = 0;
	data->currsequence  = options->sequence;
	data->nextid        = 0;
	data->rtc.seconds   = 0;
	data->rtc.nanos     = 0;
	data->interrupt     = false;
}

/*
 * log_time: logs time and child process id.
 *
 * [In]
 * 
 *     childid: id for the child process.
 *     start:   indicates if child process is starting or finishing.
 */

void log_time ( child_id_t childid, bool start, FILE *log )
{
	fict_clock_t clock;
	char         *buf;

	SH_read_clock ( &clock );

	if ( asprintf ( &buf, "Child process with ID %u, %s at %u seconds, %u nanos.\n", childid,
			start ? "started" : "finished", clock.seconds, clock.nanos ) == -1 )
	{
		return;
	}

	fwrite ( buf, sizeof ( char ), strlen ( buf ), log );

	free ( buf );
}

/*
 * log_list: logs a list of numbers into a file.
 *
 * [In]
 * 
 *     title: list title.
 *     list:  list with numbers to log.
 *     log:   log file where list numbers must be written into.
 */

void log_list ( char *title, list_t *list, FILE *log )
{
	char *buf;
	int  *np;

	if ( L_empty ( list ) )
	{
		return;
	}

	fwrite ( title, sizeof ( char ), strlen ( title ), log );
	fwrite ( SEP, sizeof ( char ), strlen ( SEP ), log );

	while ( L_length ( list ) > 0 )
	{
		np = ( int* ) L_remove ( list );
		asprintf ( &buf, "%i%s", *np, L_length ( list ) > 0 ? " " : "" );
		fwrite ( buf, sizeof ( char ), strlen ( buf ), log );
		free ( np );
		free ( buf );
	}

	fwrite ( LF, sizeof ( char ), strlen ( LF ), log );
}

/*
 * log_results: logs the results (primes, no primes, undetermined) into a file.
 *
 * [In]
 * 
 *     count: how many child processes there was.
 *     log:   log file where resuts must be written into.
 */

void log_results ( unsigned int count, FILE *log )
{
	list_t     *lp, *lnp, *lnd;
	int        n, *np;
	slot_t     slot;
	child_id_t id;

	lp  = L_create ();
	lnp = L_create ();
	lnd = L_create ();

	for ( id = 0; id < count; id++ )
	{
		SH_read_slot ( id, &slot );
		np  = malloc ( sizeof ( int ) );
		*np = slot.sequence;

		if ( slot.result == -1 )
		{			
			L_add ( lnd, np );
		}
		else if ( slot.result > 0 )
		{
			L_add ( lp, np );
		}
		else if ( slot.result < 0 )
		{
			L_add ( lnp, np );
		}
	}

	log_list ( "primes", lp, log );
	log_list ( "no primes", lnp, log );
	log_list ( "undetermined", lnd, log );

	L_destroy ( lp );
	L_destroy ( lnp );
	L_destroy ( lnd );
}

/*
 * build_argv: builds an command line for a child process.
 *
 * [In]
 * 
 *     id:       id for the child process.
 *     sequence: number to test for primality.
 *
 * [Return]
 *
 *     The command line for a child process if success, NULL otherwise.
 */

char** build_argv ( child_id_t id, int sequence )
{
	char ** argv;

	// Child process params must terminate with NULL so add extra entry.

	if ( ( argv = ( char** ) malloc ( CHILD_PROCESS_PARAMS * sizeof ( char* ) ) ) == NULL )
	{
		return NULL;
	}

	memset ( argv, 0, CHILD_PROCESS_PARAMS * sizeof ( char* ) );

	if ( asprintf ( &argv [ 0 ], "%s", CHILD_PATH ) != -1 && asprintf ( &argv [ 1 ], "%u", id ) != -1
			&& asprintf ( &argv [ 2 ], "%i", sequence ) != -1 )
	{
		return argv;
	}

	free ( argv [ 0 ] );
	free ( argv [ 1 ] );
	free ( argv [ 2 ] );
	free ( argv );

	return NULL;
}

/*
 * kill_all: kills all child processes.
 *
 * [In]
 * 
 *     count:  how many child processes exists into child processes array.
 *     childs: array with child processes. Some could be NULL.
 *
 * [Out]
 *
 *     childsdone: how many child processes have been terminated.
 */

void kill_all ( unsigned int count, child_proc_t **childs )
{
	int i;

	for ( i = 0; i < count; i++ )
	{
		if ( childs [ i ] )
		{
			kill ( childs [ i ]->pid, SIGKILL );
		}
	}
}

/*
 * new_child_process_data: creates a new child process data structure.
 *
 * [In]
 * 
 *     id:       id for the child process.
 *     sequence: number to test for primality.
 *
 * [Return]
 *
 *     A new child process data structure if success, NULL otherwise.
 */

child_proc_t* new_child_process_data ( child_id_t id, int sequence )
{
	child_proc_t *child;

	if ( ( child = ( child_proc_t* ) malloc ( sizeof ( child_proc_t ) ) ) == NULL )
	{
		return NULL;
	}

	child->pid = 0;

	if ( ( child->argv = build_argv ( id, sequence ) ) == NULL )
	{
		free ( child );
		return NULL;
	}

	return child;
}

/*
 * check_done: checks which child processes had been terminated and log their results.
 *
 * [In]
 * 
 *     count:  how many child processes exists into child processes array.
 *     childs: array with child processes. Some could be NULL.
 */

void check_done ( unsigned int count, data_t *data )
{
	int     status;
	clock_t clock;
	pid_t   pid;
	int     i;

	for ( i = 0; i < count; i++ )
	{
		// Check if child process isn't terminated already (NULL pointer).

		if ( data->childs [ i ] )
		{
			if ( ( pid = waitpid ( data->childs [ i ]->pid, &status, WNOHANG ) ) == -1 )
			{
				return;
			}

			// If pids are equals then there was a state change in child process.
			// Then check if that change is an exit condition.

			if ( pid == data->childs [ i ]->pid && ( WIFEXITED ( status ) || WIFSIGNALED ( status ) ) )
			{
				// Log finished time for i-th child process.

				log_time ( i, false, data->log );

				// Incremente number of child processes that have been terminated.

				++data->childsdone;

				// Decrement number of child processes running simultaneously.

				--data->childsrunning;

				// Free child process data structure.

				free ( data->childs [ i ] );

				// Important: mark it as NULL so program knows child process had terminated.

				data->childs [ i ] = NULL;

#ifdef DEBUG
	printf ( "Child process with PID %i terminated. Remaining child processes running: %u. Child processes done: %u.\n",
		pid, data->childsrunning, data->childsdone );
#endif
			}			
		}
	}
}

/*
 * process: spawn new child processes, update clock, check if done.
 *
 * [In]
 * 
 *     options: the options chose by user.
 */

void process ( option_t *options )
{
	pid_t childpid;
	char  **argv;
	bool  error = false;

	// Receive interrupt from keyboard signal.

	signal ( SIGINT, signal_handler );

	init_data ( &data, options );

#ifdef DEBUG
	printf ( "childsdone: %u\n", data.childsdone );
	printf ( "childsspawned: %u\n", data.childsspawned );
	printf ( "childsrunning: %u\n", data.childsrunning );
	printf ( "currsequence: %i\n", data.currsequence );
	printf ( "nextid: %u\n", data.nextid );
#endif

	while ( 1 )
	{
		// Check if we are done.
		// Two possible scenarios:
		// 1. All child processes have been terminated correctly.
		// 2. Some child processes have been terminated correctly but there was an error so we wait
		//    until all currently running child processes finish.

		if ( data.childsdone == options->max_childs || ( error && data.childsrunning == 0 ) )
		{
			break;
		}

		// If there was an error we need to wait until all processes terminate.
		// Only spawn new child processes if they don't exceed simultaneously running child processes and
		// don't exceed maximum number of child processes ever.

		if ( !error & data.childsrunning < options->run_childs && data.childsspawned < options->max_childs )
		{
			// There is room to spawn a new child process.

			if ( ( data.childs [ data.nextid ] = new_child_process_data ( data.nextid, options->sequence ) ) == NULL )
			{
				print_error ( options->program, "new_child_process_data" );
				kill_all ( options->max_childs, data.childs );
				error = true;
				continue;
			}

			argv     = data.childs [ data.nextid ]->argv;

			childpid = fork ();

			if ( childpid == -1 )
			{
				// Print error, kill all childs, mark error and wait until all child processes terminate.

				print_error ( options->program, "fork" );
				kill_all ( options->max_childs, data.childs );
				error = true;
				continue;
			}
			else if ( childpid == 0 )
			{
				// This is the child process.

#ifdef DEBUG
	int i;

	for ( i = 0; i < CHILD_PROCESS_PARAMS; i++ )
	{
		printf ( "child argv[%i] : %s\n", i, argv [ i ] );
	}
#endif

				execv ( CHILD_PATH, argv );

				// This shouldn't happen.

				print_error ( options->program, "execv" );
				kill_all ( options->max_childs, data.childs );
				error = true;
				continue;
			}

			// Log starting time for this child process.

			log_time ( data.nextid, true, data.log );

			// Add increment to sequence.

			options->sequence += options->increment;

			// Increment how many childs processes have been spawned.

			++data.childsspawned;

			// Increment how many childs processes are running simultanously.

			++data.childsrunning;

			// Store child process pid.

			data.childs [ data.nextid ]->pid = childpid;

			// Prepare next child process id (not pid).

			++data.nextid;
		}

		// Check which child processes have been terminated.

		check_done ( options->max_childs, &data );

//#ifdef DEBUG
//		printf ( "rtc: %ld:%ld\n", data.rtc.seconds, data.rtc.nanos );
//#endif

		// If process takes too long terminate or ctrl-c was received from keyboard.

		if ( data.rtc.seconds > MAX_PROCESS_SECS || data.interrupt )
		{
			kill_all ( options->max_childs, data.childs );
			error = true;
		}
	}

	log_results ( options->max_childs, data.log );
}

int main ( int argc, char *argv[] )
{
	if ( parse_args ( argc, argv, &options ) )
	{

#ifdef DEBUG
	printf ( "Option max childs: %u\n", options.max_childs );
	printf ( "Option run childs: %u\n", options.run_childs );
	printf ( "Option sequence: %u\n", options.sequence );
	printf ( "Option increment: %u\n", options.increment );
	printf ( "Option output: %s\n", options.output );
	printf ( "Option program: %s\n", options.program );
#endif

		process ( &options );
	}

	exit ( EXIT_SUCCESS );
}
