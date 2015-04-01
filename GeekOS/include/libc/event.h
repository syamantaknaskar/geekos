#include <stdio.h>
#include <stdlib.h>
#include <geekos/malloc.h>
struct int_event{
int _time;
int _pid;
struct int_event *next;
};

/*********************
***** Event Queue ****
*********************/

struct eventQueue {
struct int_event *head;
};


struct eventQueue  curr_interrupts;

/***************************
****Fun Declaration ********
***************************/
//extern void Free(void *buf);
int insert_event(struct eventQueue *queue,struct int_event *_intevent)
{
    if(queue->head==NULL)
    {
        queue->head=_intevent;
    }
    else
    {
        if((queue->head)->_time > _intevent->_time)
        {
			_intevent->next=queue->head;
            queue->head = _intevent;
            
            return 1;
        }
        else
        {
			struct int_event *it=queue->head;
			for(;it!=NULL;it=it->next)
			{
				if(it->next==NULL)
				{
					it->next=_intevent;
					return 1;
				}

				else if(_intevent->_time < (it->next)->_time)
				{
					struct int_event *temp;
					temp=it->next;
					it->next=_intevent;
					_intevent->next=temp;
					return 1;
				}
				
			}
		}

    }
    return 0;
}


void update_head(struct eventQueue *queue)
{
    struct int_event * it;
    it=queue->head;
    queue->head=it->next;
   // Free(it);
}

int delete_event(struct eventQueue *queue,struct int_event *_intevent)
{
    if(queue->head==NULL)
    {
        return 0;
    }
    else
    {
		struct int_event *it=queue->head;
        if((_intevent->_time == it->_time) && (_intevent->_pid== it->_pid))
        {
			
            queue->head=it->next;
            //Free(it);
            //Free(_intevent);
            return 1;
        }
        for(;it!=NULL;it=it->next)
        {

            if((_intevent->_time == (it->next)->_time) && (_intevent->_pid== (it->next)->_pid))
            {
                struct int_event * tmp;
                tmp=it->next;
                it->next=tmp->next;
                //Free(tmp);
                //Free(_intevent);
                return 1;
            }
            else if(it->next==NULL)
            {
                return 0;
            }
        }

    }
    return 0;
}