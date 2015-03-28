#include <libc/event.h>
#include <string.h>
#include <process.h>
#include <conio.h>
#include <stddef.h>
#include <geekos/ktypes.h>
#include <geekos/syscall.h>



int delete_event(struct eventQueue *queue,struct int_event *_intevent)
{
    struct int_event *it=queue->head;
    if(it==NULL)
    {
        return 0;
    }
    else
    {
        if((_intevent->_time == it->_time) && (_intevent->_pid== it->_pid))
        {
            queue->head=it->next;
            return 1;
        }
        for(;it!=NULL;it=it->next)
        {

            if((_intevent->_time == (it->next)->_time) && (_intevent->_pid== (it->next)->_pid))
            {
                struct int_event * tmp;
                tmp=it->next;
                it->next=tmp->next;
                //free(tmp);
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
void update_head(struct eventQueue *queue)
{
    struct int_event * it;
    it=queue->head;
    queue->head=it->next;
   // free(it);
}

