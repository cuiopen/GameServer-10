

#include "uv.h"
#include "timer_list.h"
#include <stdio.h>




struct timer
{
	void(*on_timer)(void* udata);
	uv_timer_t t;
	int count;
	void* udata;
};



void m_timer_cb(uv_timer_t* handle)
{
	//handle->data->count;
	struct timer* t = (struct timer*)handle->data;
	if (t->count>0)
	{
		t->on_timer(t->udata);
		t->count--;
		if (t->count == 0)
		{
			uv_timer_stop(&t->t);
			printf("timer stop\n");
			free(t);
		}
	}
	//printf("timer\n");
	//printf("%d\n", time);
}

struct timer* schedule(void(*on_timer)(void* udata), void* udata, int after_sec, int repeat)
{
	struct timer* handle = (struct timer*)malloc(sizeof(struct timer));
	handle->count = repeat;
	handle->on_timer = on_timer;
	handle->udata = udata;
	handle->t.data = handle;
	uv_timer_init(uv_default_loop(), &handle->t);
	uv_timer_start(&handle->t, m_timer_cb, after_sec, after_sec);
	return handle;
}

//static int schedule(void(*on_timer)(void* udata), void* udata, int after_sec, int repeat)
//{
//	timer* handle = new timer[sizeof(timer)];
//	handle->count = repeat;
//	handle->on_timer = on_timer;
//	handle->udata = udata;
//	handle->t.data = handle;
//	uv_timer_init(uv_default_loop(), &handle->t);
//	uv_timer_start(&handle->t, m_timer_cb, after_sec, after_sec);
//	return 1;
//}

 void cancel_timer(struct timer * t)
{
	if (t->count != 0)
	{
		uv_timer_stop(&t->t);
		printf("timer stop\n");
		delete(t);
	}
	else
	{
		return;
	}
}