#ifndef __MY_TIMER_LIST_H__
#define __MY_TIMER_LIST_H__

#ifdef __cplusplus
extern "C"
{
#endif

	struct timer;

	struct timer* schedule(void(*on_timer)(void* udata), void* udata, int after_sec, int repeat);
	void cancel_timer(struct timer* t);
#ifdef __cplusplus
}
#endif



#endif // !M_TIMER_H




