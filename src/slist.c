/*
 * slist.c
 *
 *  Created on: Mar 24, 2012
 *      Author: nir turjeman
 *      ID    : 039622543
 */
#include "slist.h"
#include <stdio.h>
#include <stdlib.h>

//********************************************************************************
void slist_init(slist_t * slist)
{
	slist_head(slist) = NULL;
	slist_tail(slist) = NULL;
	slist_size(slist) = 0;
}
//********************************************************************************

void slist_destroy(slist_t *slist,slist_destroy_t flag)
{
	int i;
	slist_node_t *ptr;
	if (NULL == slist)
		return;

	if (flag == SLIST_FREE_DATA)
	{
		for (i=0; i<slist_size(slist); i++ )
		{
			ptr= slist_next(slist_head(slist));
			free (slist_data(slist_head(slist)));
			//free (slist->head->data);
			free (slist_head(slist));
			slist_head(slist) = ptr;
		}
	}
	slist_head(slist) = NULL;
	slist_tail(slist) = NULL;
	slist_size(slist) = 0;
}

//********************************************************************************

void *slist_pop_first(slist_t * slist)
{
	if (NULL == slist)
		return NULL;

	void *data;
	slist_node_t *ptr = slist_head(slist);

	if(slist_size(slist) == 0)
		return NULL;
	else
	{
		data = slist_data(slist_head(slist));
		slist_head(slist) = slist_next(slist_head(slist));
		free(ptr);
		if (1 == slist_size(slist))
			slist_tail(slist) = NULL;
		slist_size(slist)--;
		return data;
	}

}

//********************************************************************************
/** Append data to list (add as last node of the list)
	\param list - a pointer to a list
	\param data - the data to place in the list
	\return 0 on success, or -1 on failure */

int slist_append(slist_t *slist ,void *data)
{
	if(NULL == slist)
		return -1;

	slist_node_t *newNode = (slist_node_t*)malloc(sizeof(slist_node_t));
	if(NULL == newNode)
		exit (-1) ;

	else
	{
		slist_data(newNode) = data;
		slist_next(newNode) = NULL;
	}

	if (slist_head(slist) == NULL)
		slist_head(slist) = newNode;
	else
		slist_next(slist_tail(slist)) = newNode;

	slist_tail(slist) = newNode;
	slist_size(slist)++;

	return 0;
}
//********************************************************************************
int slist_prepend(slist_t * slist ,void * data)
{
	if(NULL == slist )
		return -1;

	slist_node_t *tmp;
	slist_node_t *newNode = (slist_node_t*)malloc(sizeof(slist_node_t));
	if(NULL == newNode)
		exit (-1) ;
	else
	{
		slist_data(newNode) = data;
		slist_next(newNode) = NULL;
	}

	if(0 != slist_size(slist))
	{
		tmp = slist_head(slist);
		slist_head(slist) = newNode;
		slist_next(newNode) = tmp;
	}
	else
	{
		slist_head(slist) = newNode;
		slist_tail(slist) = newNode;
	}

	slist_size(slist)++;

	return 0;
}
//********************************************************************************
int slist_append_list(slist_t *destSlist, slist_t* srcSlist)
{
	int appendRes =0 ;
	if(NULL == destSlist)
		return -1;
	if(NULL == srcSlist)
		return 0;
	slist_node_t *ptr = slist_head(srcSlist);
	slist_node_t *destSlistTail = slist_tail(destSlist);
	while(NULL != ptr && destSlistTail != ptr)
	{
		appendRes = slist_append(destSlist, slist_data(ptr));
		if(-1 ==appendRes)
			return -1;
		ptr = slist_next(ptr);
	}

	return 0;
}
//********************************************************************************
