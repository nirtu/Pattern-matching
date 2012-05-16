/*
 * pattern_matching.c
 *
 *  Created on: Apr 27, 2012
 *      Author: nirtu
 */

#include "pattern_matching.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Costs
static const int FAILURE = -1;
static const int SUCCESS = 0;

// new decelerations
int pm_state_init(pm_state_t*,int, int , pm_state_t*);
int pm_match_init(pm_match_t* ,int, int, char* ,pm_state_t*, int);


//*********************************************************************************************************************
/* Initializes the fsm parameters (the fsm itself should be allocated).  Returns 0 on success, -1 on failure.
*  this function should init zero state
*/
int pm_init(pm_t * pm)
{
	pm_state_t *zero_state = (pm_state_t*)malloc(sizeof(pm_state_t));
	if(NULL == zero_state)
		return FAILURE;

	int res = pm_state_init(zero_state, 0,0,NULL);
	if(-1 == res)
	{
		free (zero_state);
		return -FAILURE;
	}

	pm->zerostate = zero_state;
	pm->newstate =1;

	return SUCCESS;
}

//*********************************************************************************************************************
/* Adds a new string to the fsm, given that the string is of length n.
   Returns 0 on success, -1 on failure.*/
int pm_addstring(pm_t * pm ,unsigned char * string, size_t len)
{
	if(NULL == pm || NULL == string)
		return FAILURE;
	if(len < 1)
		return SUCCESS;

	pm_state_t* some_state = pm->zerostate;
	pm_state_t* ptr = NULL;
	int i =0;

	//this loop make sure that if two strings have an equal start it will combine them to the same branch in the FSM
	while ((i < len) && (NULL != (ptr = pm_goto_get(some_state, string[i]))))
	{
		some_state = ptr;
		i++;
	}

	//this loop create a new branch for the leftover of the string from the position of 'some_state' (current state)
	while(i < len)
	{
		pm_state_t* new_state = (pm_state_t*)malloc(sizeof(pm_state_t));
		if(NULL == new_state)
			return FAILURE;
		else
		{
			int res = pm_state_init(new_state,pm->newstate, (some_state->depth+1), NULL);
			if(FAILURE == res)
			{
				free(new_state);
				return FAILURE;
			}
			pm->newstate++;
		}

		int res = pm_goto_set(some_state, string[i], new_state);
		if (FAILURE == res)
			return FAILURE;

		printf("Allocating state %d \n", new_state->id);
		printf("%d -> %c -> %d \n", some_state->id , string[i], new_state->id);

		some_state = new_state;
		i++;
	}

	if(slist_size(some_state->output) == 0)
	{
		char* str = (char*)malloc(sizeof(char)*strlen((char*) string) + 1);
		memcpy(str, string, strlen((char*) string));
		str += '\0';

		int result = slist_append(some_state->output, str);
		if(FAILURE == result )
			return FAILURE;
	}

	return SUCCESS;
}
//*********************************************************************************************************************
/* Finalizes construction by setting up the failure transitions, as
   well as the goto transitions of the zerostate.
   Returns 0 on success, -1 on failure.*/
int pm_makeFSM(pm_t * pm)
{
	slist_t *queue = (slist_t*)malloc(sizeof(slist_t));
	if(NULL == queue)
		return FAILURE;

	//first i will add all the state at depth one to the queue
	int i;
	for(i=0; i<PM_CHARACTERS; i++)
	{
		pm_state_t *depth_one_state = pm_goto_get(pm->zerostate, i);
		if(NULL != depth_one_state)
		{
			depth_one_state->fail = pm->zerostate;
			printf("Setting f(%d) == %d\n", depth_one_state->id, depth_one_state->fail->id);
			int res = slist_append(queue,depth_one_state);
			if(FAILURE == res)
				return FAILURE;
		}
	}

	pm_state_t *prev_state;
	pm_state_t *next_state;

	while (slist_size(queue) > 0)
	{
		pm_state_t *curr_state = (pm_state_t*)slist_pop_first(queue);

		int j;
		for(j=0; j<PM_CHARACTERS;j++)
		{
			int f=1;
			next_state = pm_goto_get(curr_state,j);
			if(NULL != next_state)
			{
				int res = slist_append(queue,next_state);
				if(FAILURE == res)
					return -1;
				prev_state = curr_state->fail;
				while(pm_goto_get(prev_state,j) == NULL)
				{
					if(prev_state->id == 0)
					{
						next_state->fail = prev_state;
						printf("Setting f(%d) == %d\n", next_state->id, next_state->fail->id);
						f=0;
						break;
					}
					else
						prev_state = prev_state->fail;
				}

				if(f==0)
					continue;

				next_state->fail = pm_goto_get(prev_state,j);
				printf("Setting f(%d) == %d\n", next_state->id, next_state->fail->id);

				if(-1 == slist_append_list(next_state->output, next_state->fail->output))
					return FAILURE;
			}
		}
	}

	slist_destroy(queue, SLIST_LEAVE_DATA);
	return SUCCESS;
}
//*********************************************************************************************************************
/* Set a transition arrow from this from_state, via a symbol, to a
   to_state. will be used in the pm_addstring and pm_makeFSM functions.
   Returns 0 on success, -1 on failure.*/
int pm_goto_set(pm_state_t *from_state, unsigned char symbol, pm_state_t *to_state)
{
	if(NULL == from_state || NULL == to_state)
		return FAILURE;

	pm_labeled_edge_t* new_edge = (pm_labeled_edge_t*)malloc(sizeof(pm_labeled_edge_t));
	if(NULL == new_edge)
		return FAILURE;
	else
	{
		new_edge->label = symbol;
		new_edge->state = to_state;
	}

	int res = slist_append(from_state->_transitions, new_edge);
	if(-1 == res)
	{
		free(new_edge);
		return FAILURE;
	}
	return SUCCESS;

}
//*********************************************************************************************************************
/* Returns the transition state.  If no such state exists, returns NULL.
   will be used in pm_addstring, pm_makeFSM, pm_fsm_search, pm_destroy functions. */
pm_state_t* pm_goto_get(pm_state_t *state, unsigned char symbol)
{
	slist_node_t *node = NULL;
	if(NULL == state)
		return NULL;

	if(NULL == state->_transitions)
		return NULL;
	else
		node = slist_head(state->_transitions);

	while(NULL != node)
	{
		if(((pm_labeled_edge_t*)node->data)->label == symbol)
			return ((pm_labeled_edge_t*)node->data)->state;
		node = slist_next(node);
	}

	return NULL;
}
//*********************************************************************************************************************
/* Search for matches in a string of size n in the FSM.
   if there are no matches return empty list */
slist_t* pm_fsm_search(pm_state_t* state ,unsigned char* string ,size_t len)
{
	slist_t* matched_list = (slist_t*)malloc(sizeof(slist_t));
	if(NULL == matched_list)
		return NULL;
	slist_init(matched_list);

	if(state == NULL)
		return NULL;

	pm_state_t* tmp = state;
	int i;
	for(i=0; i<len; i++)
	{
		while ((pm_goto_get(tmp,(string[i])) == NULL) && (i < len))
		{
			if(tmp->id != 0 )
				tmp = tmp->fail;
			else
				i++;
		}
		tmp = pm_goto_get(tmp, string[i]);
		if (NULL == tmp)
			continue;
		if(slist_size(tmp->output)>0) //means we hit receive state
		{

			slist_node_t* node = tmp->output->head;
			while (NULL != node)
			{
				int size_of_pattern = strlen((char*)node->data);
				pm_match_t* match = (pm_match_t*)malloc(sizeof(pm_match_t));
				if(NULL == match)
				{
					slist_destroy(matched_list, SLIST_FREE_DATA);
					free(matched_list);
					return NULL;
				}
				pm_match_init(match,i - size_of_pattern + 1, i ,(char*)node->data,tmp,size_of_pattern);
				slist_append(matched_list, match);
				node = slist_next(node);

			}
		}
	}
	return matched_list;
}
//*********************************************************************************************************************
/* Destroys the fsm, deallocating memory. */
void pm_destroy(pm_t* pm)
{
	slist_t *des_queue = (slist_t*)malloc(sizeof(slist_t));
	if(NULL == des_queue)
		return;
	slist_init(des_queue);

	int i;
	for(i=0; i<PM_CHARACTERS; i++)
	{
		pm_state_t *depth_one_state = pm_goto_get(pm->zerostate, i);
		if(NULL != depth_one_state)
		{
			int res = slist_append(des_queue,depth_one_state);
			if(-1 == res)
			{
				free(des_queue);
				return;
			}
		}
	}

	while(slist_size(des_queue) > 0)
	{
		pm_state_t *curr_state = (pm_state_t*)slist_pop_first(des_queue);
		if(NULL == curr_state)
			return;
		int j;
		for(j=0; j < PM_CHARACTERS; j++)
		{
			pm_state_t* child_state = pm_goto_get(curr_state,j);
			if(NULL != child_state)
			{
				int res = slist_append(des_queue,child_state);
				if(-1 == res)
				{
					free(des_queue);
					return;
				}
			}
		}
		slist_destroy((curr_state->_transitions) ,SLIST_FREE_DATA);
		slist_destroy((curr_state->output) ,SLIST_FREE_DATA);
		free(curr_state);
	}

	slist_destroy(pm->zerostate->_transitions ,SLIST_FREE_DATA);
	slist_destroy(pm->zerostate->output ,SLIST_FREE_DATA);
	free(pm->zerostate);

	slist_destroy(des_queue ,SLIST_LEAVE_DATA);
	free(des_queue);
}
//*********************************************************************************************************************
int pm_state_init(pm_state_t* state,int id, int depth, pm_state_t* fail_state)
{
	slist_t *output = (slist_t*)malloc(sizeof(slist_t));
	if(NULL == output)
		return FAILURE;

	slist_t *transitions = (slist_t*)malloc(sizeof(slist_t));
	if(NULL == transitions)
	{
		slist_destroy(transitions,SLIST_LEAVE_DATA);
		free(transitions);
		return FAILURE;
	}

	slist_init(output);
	slist_init(transitions);

	state->id = id;
	state->depth = depth;
	state->fail = fail_state;
	state->_transitions = transitions;
	state->output = output;

	return SUCCESS;
}

//*********************************************************************************************************************
int pm_match_init(pm_match_t* match ,int start_pos, int end_pos, char* pattern ,pm_state_t* fstate, int size_of_pattern)
{
	match->start_pos = start_pos;
	match->end_pos = end_pos;
	match->fstate = fstate;

	char* matched_pattern = (char*)malloc(sizeof(char) * size_of_pattern + 1);
	if(NULL == matched_pattern)
		return FAILURE;

	strcpy(matched_pattern,pattern);
	matched_pattern += '\0';

	match->pattern = matched_pattern;

	return SUCCESS;
}
