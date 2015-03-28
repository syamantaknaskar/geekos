#include <stdio.h>
struct int_event{
int _time;
int _pid;
struct int_event *next;
};

struct eventQueue {
struct int_event *head;
};
struct eventQueue  *curr_interrupts;
//int insert_event(struct eventQueue *queue,struct int_event *_intevent);
int insert_event(struct eventQueue *queue,struct int_event *_intevent)
{
    struct int_event *it=queue->head;
    //struct int_event *_nit=queue->head;
    if(it==NULL)
    {
        it=_intevent;
    }
    else
    {
        if(_intevent->_time < it->_time)
        {
            queue->head = _intevent;
            _intevent->next=it;
        }
        for(;it!=NULL;it=it->next)
        {
            if(it->next==NULL)
            {
                it->next=_intevent;
                return 1;
            }

            else if((_intevent->next)->_time < (it->next)->_time)
            {
                struct int_event *temp;
                temp=it->next;
                it->next=_intevent;
                _intevent->next=temp;
                return 1;
            }
            
        }

    }
    return 0;
}
int delete_event(struct eventQueue *queue,struct int_event *_intevent);
void update_head(struct eventQueue *queue);
