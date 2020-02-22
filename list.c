#include <stdlib.h>
#include "list.h"

void L_free_node ( list_node_t *n )
{
	free ( n->data );
	free ( n );
}

list_t* L_create ()
{
	list_t *l;

	if ( ( l = ( list_t* ) malloc ( sizeof ( list_t ) ) ) == NULL )
	{
		return NULL;
	}

	l->first = NULL;
	l->last  = NULL;
	l->count = 0;

	return l;
}

void L_destroy ( list_t *l )
{
	list_node_t *n = l->first;
	list_node_t *m;

	while ( n != NULL )
	{
		m = n;
		n = n->next;
		L_free_node ( m );
	}

	free ( l );
}

list_t* L_add ( list_t *l, void *d )
{
	list_node_t *n;

	if ( ( n = ( list_node_t* ) malloc ( sizeof ( list_node_t ) ) ) == NULL )
	{
		return NULL;
	}

	n->data = d;
	n->next = NULL;

	if ( l->first == NULL )
	{
		l->first = n;
	}
	else
	{
		l->last->next = n;
	}

	l->last = n;

	++l->count;

	return l;
}

void* L_remove ( list_t *l )
{
	list_node_t *n;
	void *d;

	if ( l->first == NULL )
	{
		return NULL;
	}

	n = l->first->next;

	d = l->first->data;

	free ( l->first );

	l->first = n;

	if ( l->first == NULL )
	{
		l->last = NULL;
	}

	--l->count;

	return d;
}

int L_length ( list_t *l )
{
	return l->count;
}

bool L_empty ( list_t *l )
{
	return L_length ( l ) == 0;
}
