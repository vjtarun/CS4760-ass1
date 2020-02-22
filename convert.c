#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "convert.h"

/*
 * CV_str_to_long: converts a string to a long value.
 * 
 * [In]
 * 
 *     s:     the string to convert.
 *     value: value converted.
 *
 * [Return]
 *
 *     True if successful, false otherwise.
 */

bool CV_str_to_long ( char *s, long *value )
{
	char *endptr;

	errno = 0;

	*value = strtol ( s, &endptr, 10 );

	if ( ( errno == ERANGE && ( *value == LONG_MAX || *value == LONG_MIN ) ) || ( errno != 0 && *value == 0 ) ) {
		return false;
	}

	if ( endptr == s ) {
		return false;
	}

	return true;
}

/*
 * CV_str_to_short: converts a string to a short value.
 * 
 * [In]
 * 
 *     s:     the string to convert.
 *     value: value converted.
 *
 * [Return]
 *
 *     True if successful, false otherwise.
 */

bool CV_str_to_short ( char *s, short *value )
{
	long l;

	if ( CV_str_to_long ( s, &l ) )
	{
		*value = ( short ) l;
		return true;
	}

	return false;
}

/*
 * CV_str_to_int: converts a string to a int value.
 * 
 * [In]
 * 
 *     s:     the string to convert.
 *     value: value converted.
 *
 * [Return]
 *
 *     True if successful, false otherwise.
 */

bool CV_str_to_int ( char *s, int *value )
{
	long l;

	if ( CV_str_to_long ( s, &l ) )
	{
		*value = ( int ) l;
		return true;
	}

	return false;
}

