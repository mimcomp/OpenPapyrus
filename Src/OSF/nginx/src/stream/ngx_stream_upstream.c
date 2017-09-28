/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
//#include <ngx_stream.h>

static ngx_int_t ngx_stream_upstream_add_variables(ngx_conf_t * cf);
static ngx_int_t ngx_stream_upstream_addr_variable(ngx_stream_session_t * s, ngx_stream_variable_value_t * v, uintptr_t data);
static ngx_int_t ngx_stream_upstream_response_time_variable(ngx_stream_session_t * s, ngx_stream_variable_value_t * v, uintptr_t data);
static ngx_int_t ngx_stream_upstream_bytes_variable(ngx_stream_session_t * s, ngx_stream_variable_value_t * v, uintptr_t data);
static const char * ngx_stream_upstream(ngx_conf_t * cf, const ngx_command_t * cmd, void * dummy); // F_SetHandler
static const char * ngx_stream_upstream_server(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
static void * ngx_stream_upstream_create_main_conf(ngx_conf_t * cf);
static char * ngx_stream_upstream_init_main_conf(ngx_conf_t * cf, void * conf);

static ngx_command_t ngx_stream_upstream_commands[] = {
	{ ngx_string("upstream"), NGX_STREAM_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_TAKE1,
	  ngx_stream_upstream, 0, 0, NULL },
	{ ngx_string("server"), NGX_STREAM_UPS_CONF|NGX_CONF_1MORE,
	  ngx_stream_upstream_server, NGX_STREAM_SRV_CONF_OFFSET, 0, NULL },
	ngx_null_command
};

static ngx_stream_module_t ngx_stream_upstream_module_ctx = {
	ngx_stream_upstream_add_variables, /* preconfiguration */
	NULL,                              /* postconfiguration */
	ngx_stream_upstream_create_main_conf, /* create main configuration */
	ngx_stream_upstream_init_main_conf, /* init main configuration */
	NULL,                              /* create server configuration */
	NULL                               /* merge server configuration */
};

ngx_module_t ngx_stream_upstream_module = {
	NGX_MODULE_V1,
	&ngx_stream_upstream_module_ctx,   /* module context */
	ngx_stream_upstream_commands,      /* module directives */
	NGX_STREAM_MODULE,                 /* module type */
	NULL,                              /* init master */
	NULL,                              /* init module */
	NULL,                              /* init process */
	NULL,                              /* init thread */
	NULL,                              /* exit thread */
	NULL,                              /* exit process */
	NULL,                              /* exit master */
	NGX_MODULE_V1_PADDING
};

static ngx_stream_variable_t ngx_stream_upstream_vars[] = {
	{ ngx_string("upstream_addr"), NULL, ngx_stream_upstream_addr_variable, 0, NGX_STREAM_VAR_NOCACHEABLE, 0 },
	{ ngx_string("upstream_bytes_sent"), NULL, ngx_stream_upstream_bytes_variable, 0, NGX_STREAM_VAR_NOCACHEABLE, 0 },
	{ ngx_string("upstream_connect_time"), NULL, ngx_stream_upstream_response_time_variable, 2, NGX_STREAM_VAR_NOCACHEABLE, 0 },
	{ ngx_string("upstream_first_byte_time"), NULL, ngx_stream_upstream_response_time_variable, 1, NGX_STREAM_VAR_NOCACHEABLE, 0 },
	{ ngx_string("upstream_session_time"), NULL, ngx_stream_upstream_response_time_variable, 0, NGX_STREAM_VAR_NOCACHEABLE, 0 },
	{ ngx_string("upstream_bytes_received"), NULL, ngx_stream_upstream_bytes_variable, 1, NGX_STREAM_VAR_NOCACHEABLE, 0 },
	{ ngx_null_string, NULL, NULL, 0, 0, 0 }
};

static ngx_int_t ngx_stream_upstream_add_variables(ngx_conf_t * cf)
{
	for(ngx_stream_variable_t * v = ngx_stream_upstream_vars; v->name.len; v++) {
		ngx_stream_variable_t * var = ngx_stream_add_variable(cf, &v->name, v->flags);
		if(var == NULL) {
			return NGX_ERROR;
		}
		var->get_handler = v->get_handler;
		var->data = v->data;
	}
	return NGX_OK;
}

static ngx_int_t ngx_stream_upstream_addr_variable(ngx_stream_session_t * s, ngx_stream_variable_value_t * v, uintptr_t data)
{
	u_char   * p;
	ngx_uint_t i;
	v->valid = 1;
	v->no_cacheable = 0;
	v->not_found = 0;
	if(!s->upstream_states || !s->upstream_states->nelts)
		v->not_found = 1;
	else {
		size_t len = 0;
		ngx_stream_upstream_state_t  * state = (ngx_stream_upstream_state_t *)s->upstream_states->elts;
		for(i = 0; i < s->upstream_states->nelts; i++) {
			if(state[i].peer) {
				len += state[i].peer->len;
			}
			len += 2;
		}
		p = (u_char*)ngx_pnalloc(s->connection->pool, len);
		if(!p) {
			return NGX_ERROR;
		}
		v->data = p;
		i = 0;
		for(;; ) {
			if(state[i].peer) {
				p = ngx_cpymem(p, state[i].peer->data, state[i].peer->len);
			}
			if(++i == s->upstream_states->nelts) {
				break;
			}
			*p++ = ',';
			*p++ = ' ';
		}
		v->len = p - v->data;
	}
	return NGX_OK;
}

static ngx_int_t ngx_stream_upstream_bytes_variable(ngx_stream_session_t * s, ngx_stream_variable_value_t * v, uintptr_t data)
{
	u_char * p;
	size_t len;
	ngx_uint_t i;
	ngx_stream_upstream_state_t  * state;
	v->valid = 1;
	v->no_cacheable = 0;
	v->not_found = 0;
	if(s->upstream_states == NULL || s->upstream_states->nelts == 0) {
		v->not_found = 1;
		return NGX_OK;
	}
	len = s->upstream_states->nelts * (NGX_OFF_T_LEN + 2);
	p = (u_char*)ngx_pnalloc(s->connection->pool, len);
	if(!p) {
		return NGX_ERROR;
	}
	v->data = p;
	i = 0;
	state = (ngx_stream_upstream_state_t *)s->upstream_states->elts;
	for(;; ) {
		if(data == 1) {
			p = ngx_sprintf(p, "%O", state[i].bytes_received);
		}
		else {
			p = ngx_sprintf(p, "%O", state[i].bytes_sent);
		}
		if(++i == s->upstream_states->nelts) {
			break;
		}
		*p++ = ',';
		*p++ = ' ';
	}
	v->len = p - v->data;
	return NGX_OK;
}

static ngx_int_t ngx_stream_upstream_response_time_variable(ngx_stream_session_t * s, ngx_stream_variable_value_t * v, uintptr_t data)
{
	u_char   * p;
	size_t len;
	ngx_uint_t i;
	ngx_msec_int_t ms;
	ngx_stream_upstream_state_t  * state;
	v->valid = 1;
	v->no_cacheable = 0;
	v->not_found = 0;
	if(s->upstream_states == NULL || s->upstream_states->nelts == 0) {
		v->not_found = 1;
		return NGX_OK;
	}
	len = s->upstream_states->nelts * (NGX_TIME_T_LEN + 4 + 2);
	p = (u_char*)ngx_pnalloc(s->connection->pool, len);
	if(!p) {
		return NGX_ERROR;
	}
	v->data = p;
	i = 0;
	state = (ngx_stream_upstream_state_t *)s->upstream_states->elts;
	for(;; ) {
		if(data == 1) {
			if(state[i].first_byte_time == (ngx_msec_t)-1) {
				*p++ = '-';
				goto next;
			}
			ms = state[i].first_byte_time;
		}
		else if(data == 2 && state[i].connect_time != (ngx_msec_t)-1) {
			ms = state[i].connect_time;
		}
		else {
			ms = state[i].response_time;
		}
		ms = MAX(ms, 0);
		p = ngx_sprintf(p, "%T.%03M", (time_t)ms / 1000, ms % 1000);
next:
		if(++i == s->upstream_states->nelts) {
			break;
		}
		*p++ = ',';
		*p++ = ' ';
	}
	v->len = p - v->data;
	return NGX_OK;
}

static const char * ngx_stream_upstream(ngx_conf_t * cf, const ngx_command_t * cmd, void * dummy) // F_SetHandler
{
	char   * rv;
	void   * mconf;
	ngx_str_t   * value;
	ngx_url_t u;
	ngx_uint_t m;
	ngx_conf_t pcf;
	ngx_stream_module_t   * module;
	ngx_stream_conf_ctx_t * ctx, * stream_ctx;
	ngx_stream_upstream_srv_conf_t  * uscf;
	memzero(&u, sizeof(ngx_url_t));
	value = (ngx_str_t*)cf->args->elts;
	u.host = value[1];
	u.no_resolve = 1;
	u.no_port = 1;
	uscf = ngx_stream_upstream_add(cf, &u, NGX_STREAM_UPSTREAM_CREATE|NGX_STREAM_UPSTREAM_WEIGHT|
		NGX_STREAM_UPSTREAM_MAX_CONNS|NGX_STREAM_UPSTREAM_MAX_FAILS|NGX_STREAM_UPSTREAM_FAIL_TIMEOUT|NGX_STREAM_UPSTREAM_DOWN|NGX_STREAM_UPSTREAM_BACKUP);
	if(uscf == NULL) {
		return NGX_CONF_ERROR;
	}
	ctx = (ngx_stream_conf_ctx_t*)ngx_pcalloc(cf->pool, sizeof(ngx_stream_conf_ctx_t));
	if(!ctx) {
		return NGX_CONF_ERROR;
	}
	stream_ctx = (ngx_stream_conf_ctx_t*)cf->ctx;
	ctx->main_conf = stream_ctx->main_conf;
	/* the upstream{}'s srv_conf */
	ctx->srv_conf = (void**)ngx_pcalloc(cf->pool, sizeof(void *) * ngx_stream_max_module);
	if(ctx->srv_conf == NULL) {
		return NGX_CONF_ERROR;
	}
	ctx->srv_conf[ngx_stream_upstream_module.ctx_index] = uscf;
	uscf->srv_conf = ctx->srv_conf;
	for(m = 0; cf->cycle->modules[m]; m++) {
		if(cf->cycle->modules[m]->type != NGX_STREAM_MODULE) {
			continue;
		}
		module = (ngx_stream_module_t*)cf->cycle->modules[m]->ctx;
		if(module->create_srv_conf) {
			mconf = module->create_srv_conf(cf);
			if(mconf == NULL) {
				return NGX_CONF_ERROR;
			}
			ctx->srv_conf[cf->cycle->modules[m]->ctx_index] = mconf;
		}
	}
	uscf->servers = ngx_array_create(cf->pool, 4, sizeof(ngx_stream_upstream_server_t));
	if(uscf->servers == NULL) {
		return NGX_CONF_ERROR;
	}
	/* parse inside upstream{} */
	pcf = *cf;
	cf->ctx = ctx;
	cf->cmd_type = NGX_STREAM_UPS_CONF;
	rv = ngx_conf_parse(cf, NULL);
	*cf = pcf;
	if(rv != NGX_CONF_OK) {
		return rv;
	}
	if(uscf->servers->nelts == 0) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "no servers are inside upstream");
		return NGX_CONF_ERROR;
	}
	return rv;
}

static const char * ngx_stream_upstream_server(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf) // F_SetHandler
{
	ngx_stream_upstream_srv_conf_t  * uscf = (ngx_stream_upstream_srv_conf_t *)conf;
	time_t fail_timeout;
	ngx_str_t * value, s;
	ngx_url_t u;
	ngx_int_t weight, max_conns, max_fails;
	ngx_uint_t i;
	ngx_stream_upstream_server_t  * us = (ngx_stream_upstream_server_t *)ngx_array_push(uscf->servers);
	if(us == NULL) {
		return NGX_CONF_ERROR;
	}
	memzero(us, sizeof(ngx_stream_upstream_server_t));
	value = (ngx_str_t*)cf->args->elts;
	weight = 1;
	max_conns = 0;
	max_fails = 1;
	fail_timeout = 10;
	for(i = 2; i < cf->args->nelts; i++) {
		if(ngx_strncmp(value[i].data, "weight=", 7) == 0) {
			if(!(uscf->flags & NGX_STREAM_UPSTREAM_WEIGHT)) {
				goto not_supported;
			}
			weight = ngx_atoi(&value[i].data[7], value[i].len - 7);
			if(weight == NGX_ERROR || weight == 0) {
				goto invalid;
			}
			continue;
		}
		if(ngx_strncmp(value[i].data, "max_conns=", 10) == 0) {
			if(!(uscf->flags & NGX_STREAM_UPSTREAM_MAX_CONNS)) {
				goto not_supported;
			}
			max_conns = ngx_atoi(&value[i].data[10], value[i].len - 10);
			if(max_conns == NGX_ERROR) {
				goto invalid;
			}
			continue;
		}
		if(ngx_strncmp(value[i].data, "max_fails=", 10) == 0) {
			if(!(uscf->flags & NGX_STREAM_UPSTREAM_MAX_FAILS)) {
				goto not_supported;
			}
			max_fails = ngx_atoi(&value[i].data[10], value[i].len - 10);
			if(max_fails == NGX_ERROR) {
				goto invalid;
			}
			continue;
		}
		if(ngx_strncmp(value[i].data, "fail_timeout=", 13) == 0) {
			if(!(uscf->flags & NGX_STREAM_UPSTREAM_FAIL_TIMEOUT)) {
				goto not_supported;
			}
			s.len = value[i].len - 13;
			s.data = &value[i].data[13];
			fail_timeout = ngx_parse_time(&s, 1);
			if(fail_timeout == (time_t)NGX_ERROR) {
				goto invalid;
			}
			continue;
		}
		if(sstreq(value[i].data, "backup")) {
			if(!(uscf->flags & NGX_STREAM_UPSTREAM_BACKUP)) {
				goto not_supported;
			}
			us->backup = 1;
			continue;
		}
		if(sstreq(value[i].data, "down")) {
			if(!(uscf->flags & NGX_STREAM_UPSTREAM_DOWN)) {
				goto not_supported;
			}
			us->down = 1;
			continue;
		}
		goto invalid;
	}
	memzero(&u, sizeof(ngx_url_t));
	u.url = value[1];
	if(ngx_parse_url(cf->pool, &u) != NGX_OK) {
		if(u.err) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s in upstream \"%V\"", u.err, &u.url);
		}
		return NGX_CONF_ERROR;
	}
	if(u.no_port) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "no port in upstream \"%V\"", &u.url);
		return NGX_CONF_ERROR;
	}
	us->name = u.url;
	us->addrs = u.addrs;
	us->naddrs = u.naddrs;
	us->weight = weight;
	us->max_conns = max_conns;
	us->max_fails = max_fails;
	us->fail_timeout = fail_timeout;
	return NGX_CONF_OK;
invalid:
	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%V\"", &value[i]);
	return NGX_CONF_ERROR;
not_supported:
	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "balancing method does not support parameter \"%V\"", &value[i]);
	return NGX_CONF_ERROR;
}

ngx_stream_upstream_srv_conf_t * ngx_stream_upstream_add(ngx_conf_t * cf, ngx_url_t * u, ngx_uint_t flags)
{
	ngx_uint_t i;
	ngx_stream_upstream_server_t   * us;
	ngx_stream_upstream_srv_conf_t * uscf, ** uscfp;
	ngx_stream_upstream_main_conf_t  * umcf;
	if(!(flags & NGX_STREAM_UPSTREAM_CREATE)) {
		if(ngx_parse_url(cf->pool, u) != NGX_OK) {
			if(u->err) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s in upstream \"%V\"", u->err, &u->url);
			}
			return NULL;
		}
	}
	umcf = (ngx_stream_upstream_main_conf_t*)ngx_stream_conf_get_module_main_conf(cf, ngx_stream_upstream_module);
	uscfp = (ngx_stream_upstream_srv_conf_t **)umcf->upstreams.elts;
	for(i = 0; i < umcf->upstreams.nelts; i++) {
		if(uscfp[i]->host.len != u->host.len || ngx_strncasecmp(uscfp[i]->host.data, u->host.data, u->host.len) != 0) {
			continue;
		}
		if((flags & NGX_STREAM_UPSTREAM_CREATE) && (uscfp[i]->flags & NGX_STREAM_UPSTREAM_CREATE)) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "duplicate upstream \"%V\"", &u->host);
			return NULL;
		}
		if((uscfp[i]->flags & NGX_STREAM_UPSTREAM_CREATE) && !u->no_port) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "upstream \"%V\" may not have port %d", &u->host, u->port);
			return NULL;
		}
		if((flags & NGX_STREAM_UPSTREAM_CREATE) && !uscfp[i]->no_port) {
			ngx_log_error(NGX_LOG_EMERG, cf->log, 0, "upstream \"%V\" may not have port %d in %s:%ui",
			    &u->host, uscfp[i]->port, uscfp[i]->file_name, uscfp[i]->line);
			return NULL;
		}
		if(uscfp[i]->port != u->port) {
			continue;
		}
		if(flags & NGX_STREAM_UPSTREAM_CREATE) {
			uscfp[i]->flags = flags;
		}
		return uscfp[i];
	}
	uscf = (ngx_stream_upstream_srv_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_stream_upstream_srv_conf_t));
	if(uscf == NULL) {
		return NULL;
	}
	uscf->flags = flags;
	uscf->host = u->host;
	uscf->file_name = cf->conf_file->file.name.data;
	uscf->line = cf->conf_file->line;
	uscf->port = u->port;
	uscf->no_port = u->no_port;
	if(u->naddrs == 1 && (u->port || u->family == AF_UNIX)) {
		uscf->servers = ngx_array_create(cf->pool, 1, sizeof(ngx_stream_upstream_server_t));
		if(uscf->servers == NULL) {
			return NULL;
		}
		us = (ngx_stream_upstream_server_t *)ngx_array_push(uscf->servers);
		if(us == NULL) {
			return NULL;
		}
		memzero(us, sizeof(ngx_stream_upstream_server_t));
		us->addrs = u->addrs;
		us->naddrs = 1;
	}
	uscfp = (ngx_stream_upstream_srv_conf_t **)ngx_array_push(&umcf->upstreams);
	if(uscfp == NULL) {
		return NULL;
	}
	*uscfp = uscf;
	return uscf;
}

static void * ngx_stream_upstream_create_main_conf(ngx_conf_t * cf)
{
	ngx_stream_upstream_main_conf_t  * umcf = (ngx_stream_upstream_main_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_stream_upstream_main_conf_t));
	if(umcf == NULL) {
		return NULL;
	}
	if(ngx_array_init(&umcf->upstreams, cf->pool, 4, sizeof(ngx_stream_upstream_srv_conf_t *)) != NGX_OK) {
		return NULL;
	}
	return umcf;
}

static char * ngx_stream_upstream_init_main_conf(ngx_conf_t * cf, void * conf)
{
	ngx_stream_upstream_main_conf_t * umcf = (ngx_stream_upstream_main_conf_t *)conf;
	ngx_stream_upstream_srv_conf_t ** uscfp = (ngx_stream_upstream_srv_conf_t **)umcf->upstreams.elts;
	for(ngx_uint_t i = 0; i < umcf->upstreams.nelts; i++) {
		ngx_stream_upstream_init_pt init = uscfp[i]->peer.init_upstream ? uscfp[i]->peer.init_upstream : ngx_stream_upstream_init_round_robin;
		if(init(cf, uscfp[i]) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
	}
	return NGX_CONF_OK;
}