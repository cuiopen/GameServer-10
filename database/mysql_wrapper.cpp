#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uv.h"
#ifdef WIN32
#include <winsock.h>
#endif
#include "mysql.h"
#include "mysql_wrapper.h"

#define my_malloc malloc
#define my_free free


struct mysql_context
{
	void* pConn;
	uv_mutex_t lock;
	int is_closing;
};

struct connect_req
{
	char* ip;
	int port;
	char* db_name;
	char* uname;
	char* upwd;
	void(*open_cb)(const char* err, void* context, void* udata);

	char* err;
	void* context;

	void* udata;
};

struct query_req
{
	void* context;
	char* sql;
	char* error;
	void(*query_cb)(const char* err, MYSQL_RES* res,void* udata);
	MYSQL_RES* res;
	void * udata;
};


extern "C"
{
	static void
	connect_work(uv_work_t* req)
	{
		connect_req* r = (connect_req*)req->data;
		MYSQL* pConn = mysql_init(NULL);
		if (mysql_real_connect(pConn, r->ip, r->uname, r->upwd, r->db_name, r->port, NULL, 0))
		{
			mysql_context* c = (mysql_context*)my_malloc(sizeof(mysql_context));
			memset(c, 0, sizeof(mysql_context));

			c->pConn = pConn;
			
			uv_mutex_init(&c->lock);
			r->context = c;
			r->err = NULL;
		}
		else
		{
			r->context = NULL;
			r->err =strdup(mysql_error(pConn));
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
		if (r->db_name)
		{
			free(r->db_name);
		}
		if (r->err)
		{
			free(r->err);
		}
		if (r->uname)
		{
			free(r->uname);
		}
		if (r->upwd)
		{
			free(r->upwd);
		}
		free(r);
		free(req);
	}

	static void
	close_work(uv_work_t* req)
	{
		mysql_context* c = (mysql_context*)req->data;
		uv_mutex_lock(&c->lock);
		MYSQL* pConn = (MYSQL*)c->pConn;
		mysql_close(pConn); 
		uv_mutex_unlock(&c->lock);
	}

	static void
	on_close_complete(uv_work_t* req, int status)
	{
		mysql_context* c = (mysql_context*)req->data;
		my_free(c);
		my_free(req);
	}

	static void
	query_work(uv_work_t* req)
	{
		query_req* r = (query_req*)req->data;
		mysql_context* c = (mysql_context*)r->context;
		MYSQL* pConn = (MYSQL*)c->pConn; 
		uv_mutex_lock(&c->lock);
		int ret = mysql_query(pConn, r->sql);
		if (ret!=0)
		{
			r->error = strdup(mysql_error(pConn));
			r->res = NULL;
		}
		r->error = NULL;
		MYSQL_RES* res = mysql_store_result(pConn);
		//if (!res)
		//{
		//	r->res = NULL;
		//	uv_mutex_unlock(&c->lock);
		//	return;
		//}
		r->res = res;/*new std::vector<std::vector<std::string>>;
		int filed_num = mysql_num_fields(res);
		std::vector<std::string> empty;
		MYSQL_ROW row;
		std::vector<std::vector<std::string>>::iterator end_elem;
		while (row=mysql_fetch_row(res))
		{
			r->res->push_back(empty);
			end_elem = r->res->end()-1;
			for (int i = 0; i < filed_num; i++)
			{
				end_elem->push_back(row[i]);
			}
		}*/
		//mysql_free_result(res);
		uv_mutex_unlock(&c->lock);
	}

	static void
	on_query_complete(uv_work_t* req, int status)
	{
		query_req* r = (query_req*)req->data;
		r->query_cb(r->error, r->res,r->udata);
		
		if (r->sql)
		{
			free(r->sql);
		}
		if (r->error)
		{
			free(r->error);
		}
		if (r->res)
		{
			mysql_free_result(r->res);
		}
		free(r);
		free(req);
	}

}





void 
MysqlWrapper::Connect(char* ip, int port, char* db_name, char* uname, char* pwd,						    void(*open_cb)(const char* err, void* context, void* udata), void* udata)
{
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));
	connect_req* r = (connect_req*)my_malloc(sizeof(connect_req));
	memset(r, 0, sizeof(struct connect_req));

	r->ip = strdup(ip);
	r->port = port;
	r->db_name = strdup(db_name);
	r->uname = strdup(uname);
	r->upwd = strdup(pwd);
	r->open_cb = open_cb;
	r->udata = udata;

	w->data = (void*)r;
	uv_queue_work(uv_default_loop(), w, connect_work, on_work_complete);

}

void MysqlWrapper::Close(void* context)
{
	mysql_context* c = (mysql_context*)context;
	//if (c->is_closing)
	//{
	//	return;
	//}
	uv_work_t* w = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	memset(w, 0, sizeof(uv_work_t));
	w->data = (context);

	
	//c->is_closing = 1;
	uv_queue_work(uv_default_loop(), w, close_work, on_close_complete);
}

void MysqlWrapper::Query(void* context, char* sql, void(*query_cb)(const char* err,						      MYSQL_RES* result, void* udata), void* udata)
{
	mysql_context* c = (mysql_context*)context;
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
	r->sql = strdup(sql);
	r->udata = udata;
	w->data = (void*)r;
	uv_queue_work(uv_default_loop(), w, query_work, on_query_complete);

}
