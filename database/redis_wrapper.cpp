#ifdef WIN32
#include "hiredis.h"
#define NO_QFORKIMPL //这一行必须加才能正常使用
#include "Win32_Interop/win32fixes.h"
#endif // WIN32

#include "redis_wrapper.h"
#include "uv.h"


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define my_malloc malloc
#define my_free free


struct redis_context
{
	void* pConn;
	uv_mutex_t lock;
	int is_closing;
};

struct connect_req
{
	char* ip;
	int port;
	void(*open_cb)(const char* err, void* context, void* udata);

	char* err;
	void* context;
	void* udata;

};

struct query_req
{
	void* context;
	char* cmd;
	void(*query_cb)(redisReply* replay, void* udata);
	redisReply* res;
	void* udata;
};


extern "C"
{
	static void
	connect_work(uv_work_t* req)
	{
		connect_req* r = (connect_req*)req->data;
		struct timeval timeout = { 5,0 };
		redisContext* rc = redisConnectWithTimeout(r->ip, r->port, timeout);
		if (rc->err)
		{
			r->err = strdup(rc->errstr);
			r->context = NULL;
			redisFree(rc);
		}
		else
		{
			redis_context* c = (redis_context*)my_malloc(sizeof(redis_context));
			memset(c, 0, sizeof(redis_context));
			c->pConn = rc;
			uv_mutex_init(&c->lock);
			r->context = (void*)c;
		}
	}

	static void
	on_work_complete(uv_work_t* req, int status)
	{
		connect_req* r = (connect_req*)req->data;
		r->open_cb(r->err, r->context, r->udata);
		if (r->ip)
		{
			free(r->ip);
		}
		if (r->err)
		{
			free(r->err);
		}
		free(r);
		free(req);
	}

	static void
	close_work(uv_work_t* req)
	{
		redis_context* r = (redis_context*)req->data;
		//MYSQL* pConn = (MYSQL*)c->pConn;
		redisContext* c = (redisContext*)r->pConn;
		uv_mutex_lock(&r->lock);
		redisFree(c);
		//mysql_close(pConn); 
		uv_mutex_unlock(&r->lock);
	}

	static void
	on_close_complete(uv_work_t* req, int status)
	{
		redis_context* c = (redis_context*)req->data;
		my_free(c);
		my_free(req);
	}

	static void
	query_work(uv_work_t* req)
	{
		query_req* r = (query_req*)req->data;
		redis_context* c = (redis_context*)r->context;
		redisContext* rc = (redisContext*)c->pConn;
		uv_mutex_lock(&c->lock);
		redisReply* res =(redisReply*)redisCommand(rc, r->cmd);
		if (res)
		{
			r->res = res;
		}

		uv_mutex_unlock(&c->lock);
	}

	static void
	on_query_complete(uv_work_t* req, int status)
	{
		query_req* r = (query_req*)req->data;
		r->query_cb(r->res,r->udata);
		if (r->cmd)
		{
			free(r->cmd);
		}
		if (r->res)
		{
			freeReplyObject(r->res);
		}
		free(r);
		free(req);
	}

}





void 
RedisWrapper::Connect(char* ip, int port,void(*open_cb)(const char* err, void* context, void* udata), void* udata)
{
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));
	connect_req* r = (connect_req*)my_malloc(sizeof(connect_req));
	memset(r, 0, sizeof(struct connect_req));

	r->ip = strdup(ip);
	r->port = port;
	r->open_cb = open_cb;
	r->udata = udata;

	w->data = (void*)r;
	uv_queue_work(uv_default_loop(), w, connect_work, on_work_complete);

}

void RedisWrapper::Close(void* context)
{
	
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));
	w->data = (context);
	//uv_mutex_lock(&((redis_context*)context)->lock);
	//c->is_closing = 1;
	uv_queue_work(uv_default_loop(), w, close_work, on_close_complete);
}

void 
RedisWrapper::Query(void* context, char* cmd, void(*query_cb)(redisReply* replay, void* udata), void* udata)
{
	//redisContext* c = (redisContext*)context;
	//if (c->is_closing)
	//{
	//	return;
	//}
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));
	query_req* r = (query_req*)my_malloc(sizeof(query_req));
	memset(r, 0, sizeof(query_req));
	r->context = context;
	r->query_cb = query_cb;
	r->cmd = strdup(cmd);
	w->data = (void*)r;
	r->udata = udata;
	
	uv_queue_work(uv_default_loop(), w, query_work, on_query_complete);

}
