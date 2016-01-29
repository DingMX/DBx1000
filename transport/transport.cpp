#include "global.h"
#include "manager.h"
#include "transport.h"
#include "nn.hpp"
#include "mem_alloc.h"
#include "remote_query.h"
#include "tpcc_query.h"
#include "query.h"
#include "wl.h"

#define MAX_IFADDR_LEN 20

/*
	 HEADER FORMAT:
	 32b destination ID
	 32b source ID
	 32b ??? [unused] ??? transaction ID
	 ===> 32b message type <=== First 4 bytes of data
	 N?	data

	 */
void Transport::shutdown() {
  /*
  printf("Shutting down\n");
//  for(uint64_t i=0;i<_s_cnt;i++) 
    //s[i].sock.shutdown(endpoint_id[i]);
  uint64_t s_thd;
  if(ISCLIENT)
    s_thd = g_client_send_thread_cnt;
  else
    s_thd = g_send_thread_cnt;
  for(uint64_t i = 0; i < s_thd; i++) {
    for(uint64_t j = 0; j < _node_cnt; j++) {
      if(j == g_node_id)
        continue;
      delete &s[i];
    }
  }
  */
  //mem_allocator.free(endpoint_id,sizeof(int)*_s_cnt);
}

void Transport::read_ifconfig(const char * ifaddr_file) {
	uint64_t cnt = 0;
	ifstream fin(ifaddr_file);
	string line;
  while (getline(fin, line)) {
		//memcpy(ifaddr[cnt],&line[0],12);
    strcpy(ifaddr[cnt],&line[0]);
		cnt++;
	}
	for(uint64_t i=0;i<_node_cnt;i++) {
		printf("%ld: %s\n",i,ifaddr[i]);
	}
}

void Transport::init(uint64_t node_id,workload * workload) {
	_wl = workload;
  _node_cnt = g_node_cnt + g_client_node_cnt;
  /*
#if CC_ALG == CALVIN
	_node_cnt++;	// account for the sequencer
#endif
*/
	_node_id = node_id;
  if(ISCLIENT)
    _sock_cnt = g_client_send_thread_cnt * g_servers_per_client + (_node_cnt)*2;
    //_sock_cnt = 1 + g_client_send_thread_cnt * g_servers_per_client + (_node_cnt-1-g_servers_per_client);
    //_sock_cnt = g_client_send_thread_cnt * (_node_cnt-1) + g_node_cnt * g_send_thread_cnt + (g_client_node_cnt-1) * g_client_send_thread_cnt;
  else
    _sock_cnt = (_node_cnt)*2 + g_client_send_thread_cnt;
    //_sock_cnt = 1 + (_node_cnt - 1);
    //_sock_cnt = g_send_thread_cnt  * (_node_cnt-1) + (g_node_cnt-1) * g_send_thread_cnt + g_client_node_cnt * g_client_send_thread_cnt;
  /*
#if CC_ALG == CALVIN
	_thd_id = 0;
  _sock_cnt = (_node_cnt)*2;
#else
	_thd_id = 1;
#endif
*/
	_thd_id = 1;

  s = new Socket[_sock_cnt];
  rr = 0;
  //endpoint_id = (int*)mem_allocator.alloc(sizeof(int)*_sock_cnt,0);
	printf("Tport Init %ld: %ld\n",node_id,_sock_cnt);

#if !TPORT_TYPE_IPC
	// Read ifconfig file
	ifaddr = new char *[_node_cnt];
	for(uint64_t i=0;i<_node_cnt;i++) {
		ifaddr[i] = new char[MAX_IFADDR_LEN];
	}
	string path;
#if SHMEM_ENV
  path = "/dev/shm/";
#else
	char * cpath;
  cpath = getenv("SCHEMA_PATH");
	if(cpath == NULL)
		path = "./";
	else
		path = string(cpath);
#endif
	path += "ifconfig.txt";
	cout << "reading ifconfig file: " << path << endl;
	read_ifconfig(path.c_str());
#endif

	int rc = 0;

	int timeo = 1000; // timeout in ms
	int stimeo = 1000; // timeout in ms
  int opt = 0;
	for(uint64_t i=0;i<_sock_cnt;i++) {
		s[i].sock.setsockopt(NN_SOL_SOCKET,NN_RCVTIMEO,&timeo,sizeof(timeo));
		s[i].sock.setsockopt(NN_SOL_SOCKET,NN_SNDTIMEO,&stimeo,sizeof(stimeo));
    // NN_TCP_NODELAY doesn't cause TCP_NODELAY to be set -- nanomsg issue #118
		s[i].sock.setsockopt(NN_SOL_SOCKET,NN_TCP_NODELAY,&opt,sizeof(opt));
	}

	printf("Node ID: %d/%lu\n",g_node_id,_node_cnt);
  fflush(stdout);

  uint64_t s_cnt = 0;
#if SET_AFFINITY
  uint64_t s_thd;
  if(ISCLIENT)
    s_thd = g_client_send_thread_cnt;
  else
    s_thd = g_send_thread_cnt;
#endif

  // Listening port
  char socket_name[MAX_TPORT_NAME];
  int port;

    for(uint64_t j = 0; j < _node_cnt; j++) {
      if(j == g_node_id) {
        s_cnt++;
        continue;
      }
#if TPORT_TYPE_IPC
      port = j;
      sprintf(socket_name,"%s://node_%d_%d%s",TPORT_TYPE,port,g_node_id,TPORT_PORT);
#elif !SET_AFFINITY
      port = TPORT_PORT + j * _node_cnt + g_node_id;
      sprintf(socket_name,"%s://eth0:%d",TPORT_TYPE,port);
#else
      if(ISCLIENT)
        port = TPORT_PORT + j + _node_cnt;
      else
        port = TPORT_PORT + j;
      sprintf(socket_name,"%s://eth0:%d",TPORT_TYPE,port);
#endif
      printf("Sock[%ld] Binding to %s %d\n",s_cnt,socket_name,g_node_id);
      rc = s[s_cnt++].sock.bind(socket_name);
      if(rc < 0) {
        printf("Bind Error: %d %s\n",errno,strerror(errno));
        assert(false);
      }
    }
#if SET_AFFINITY
  if(!ISCLIENT) {
    for(uint64_t j = 0; j < g_client_send_thread_cnt; j++) {
#if TPORT_TYPE_IPC
      port = j + _node_cnt;
      sprintf(socket_name,"%s://node_%d_%d%s",TPORT_TYPE,port,g_node_id,TPORT_PORT);
#else
      port = TPORT_PORT + j + _node_cnt*2;
      sprintf(socket_name,"%s://eth0:%d",TPORT_TYPE,port);
#endif
      printf("Sock[%ld] Binding to %s %d\n",s_cnt,socket_name,g_node_id);
      rc = s[s_cnt++].sock.bind(socket_name);
      if(rc < 0) {
        printf("Bind Error: %d %s\n",errno,strerror(errno));
        assert(false);
      }

    }
  }
#endif
  // Sending ports
    for(uint64_t j = 0; j < _node_cnt; j++) {
      if(j == g_node_id) {
        s_cnt++;
        continue;
      }
#if TPORT_TYPE_IPC
      port = g_node_id;
      sprintf(socket_name,"%s://node_%d_%ld%s",TPORT_TYPE,port,j,TPORT_PORT);
#elif !SET_AFFINITY
      port = TPORT_PORT + g_node_id * _node_cnt + j;
			sprintf(socket_name,"%s://eth0;%s:%d",TPORT_TYPE,ifaddr[j],port);
#else
      if(ISCLIENTN(j))
        port = TPORT_PORT + g_node_id + _node_cnt;
      else
        port = TPORT_PORT + g_node_id;
			sprintf(socket_name,"%s://eth0;%s:%d",TPORT_TYPE,ifaddr[j],port);
#endif
      printf("Sock[%ld] Connecting to %s %d -> %ld\n",s_cnt,socket_name,g_node_id,j);
      rc = s[s_cnt++].sock.connect(socket_name);
      if(rc < 0) {
        printf("Connect Error: %d %s\n",errno,strerror(errno));
        assert(false);
      }
    }

#if SET_AFFINITY
  if(ISCLIENT) {
    for(uint64_t i = 0; i < s_thd; i++) {
      for(uint64_t j = g_server_start_node; j < g_server_start_node + g_servers_per_client; j++) {
#if TPORT_TYPE_IPC
        port = _node_cnt + i;
        sprintf(socket_name,"%s://node_%d_%ld%s",TPORT_TYPE,port,j,TPORT_PORT);
#else
        port = TPORT_PORT + _node_cnt*2 + i;
        sprintf(socket_name,"%s://eth0;%s:%d",TPORT_TYPE,ifaddr[j],port);
#endif
        printf("Sock[%ld] Connecting to %s %d -> %ld\n",s_cnt,socket_name,g_node_id,j);
        rc = s[s_cnt++].sock.connect(socket_name);
        if(rc < 0) {
          printf("Connect Error: %d %s\n",errno,strerror(errno));
          assert(false);
        }
      }
    }
  } 
#endif

	fflush(stdout);
  //assert(s_cnt == _sock_cnt);

	if(rc < 0) {
		printf("Bind Error: %d %s\n",errno,strerror(errno));
	}
}

uint64_t Transport::get_node_id() {
	return _node_id;
}

void Transport::send_msg(uint64_t sid, uint64_t dest_id, void * sbuf,int size) {
  uint64_t starttime = get_sys_clock();
  uint64_t id;
#if !SET_AFFINITY
  id = dest_id;
#else
  if(ISCLIENT) {
    if(dest_id >= g_server_start_node && dest_id < g_server_start_node + g_servers_per_client) {
      id = _node_cnt * 2 + (sid-g_client_thread_cnt) * g_servers_per_client + (dest_id - g_server_start_node);
    } else {
        id = _node_cnt + dest_id;
    }
  }
  else {
      id = g_client_send_thread_cnt + _node_cnt + dest_id;
  }
#endif
  uint64_t idx = id; 
  if(!_wl->sim_init_done) {
    printf("d:%ld idx:%ld -- %ld: %ld, %ld, %ld\n",dest_id,idx,id,_node_cnt,dest_id,sid);
    fflush(stdout);
  }
  assert(idx < _sock_cnt);

	void * buf = nn_allocmsg(size,0);
	memcpy(buf,sbuf,size);
  int rc = -1;
  //int attempts = 0;

  //while(attempts < 10 && rc < 0 && !_wl->sim_timeout) {
  while(rc < 0 && !_wl->sim_timeout) {
    rc= s[idx].sock.send(&buf,NN_MSG,NN_DONTWAIT);
    // Check for a send error
    /*
    if(rc < 0 || rc != size) {
      printf("send Error: %d %s; Done: %d\n",errno,strerror(errno),_wl->sim_timeout);
      sleep(10);
    }
    */
    //attempts++;
  }
  //assert(rc == size);
	//int rc= s[idx].sock.send(&sbuf,NN_MSG,0);
  //nn_freemsg(sbuf);

  uint64_t endtime = get_sys_clock();
  INC_STATS(_thd_id,time_tport_send,endtime - starttime);
	INC_STATS(_thd_id,msg_sent_cnt,1);
	INC_STATS(_thd_id,msg_bytes,size);
  //printf("Send Msg: -> %ld, %d: %f\n",dest_id,size,(float)(endtime-starttime)/BILLION);
}

void Transport::send_msg(uint64_t dest_id, void ** data, int * sizes, int num) {
    uint64_t starttime = get_sys_clock();
#if CC_ALG == CALVIN
	//RemReqType * rtype = (RemReqType *) data[rtype_offset];
	RemReqType rtype = *((RemReqType *)data[1]);
	//memcpy(&rtype, data[rtype_offset],sizeof(RemReqType));
	if (rtype != INIT_DONE) {
		// Sequencer is last node id
		uint64_t seq_node_id = _node_cnt - 1;
		if (g_node_id != seq_node_id)
			dest_id = seq_node_id;
	} else {
		printf("Node %lu sending INIT_DONE to %lu\n",get_node_id(),dest_id);
	}
#endif

	// 1: Scrape data pointers for actual data
	// Profile data for size
	uint64_t size = HEADER_SIZE;
	for(int i = 0; i < num; i++) {
		size += sizes[i];
	}
	// For step 3
	size += sizeof(ts_t);

	void * sbuf = nn_allocmsg(size,0);
	memset(sbuf,0,size);

	// Copy all data into sbuf
	uint64_t dsize = HEADER_SIZE;
	for(int i = 0; i < num; i++) {
		memcpy(&((char*)sbuf)[dsize],data[i],sizes[i]);
		dsize += sizes[i];
	}


	// 2: Create message header
  // dest_id
	((uint32_t*)sbuf)[0] = dest_id;
  // return_id
	((uint32_t*)sbuf)[1] = get_node_id();

	DEBUG("Sending %ld -> %ld: %ld bytes\n",get_node_id(),dest_id,size);

	// 3: Add time of message sending for stats purposes
	ts_t time = get_sys_clock();
	memcpy(&((char*)sbuf)[dsize],&time,sizeof(ts_t));
	dsize += sizeof(ts_t);

	assert(size == dsize);

	int rc;
	
	// 4: send message
  if(*(RemReqType*)data[1] == EXP_DONE) {
	  rc = s[dest_id].sock.send(&sbuf,NN_MSG,NN_DONTWAIT);
    return;
  }

  if(g_network_delay > 0) {
    RemReqType rem_req_type = *(RemReqType*)data[1];
    if (rem_req_type != INIT_DONE) { // && rem_req_type != RTXN) {
       DelayMessage * msg = new DelayMessage(dest_id, sbuf,size);
       delay_queue->add_entry(msg);
       INC_STATS(_thd_id,time_tport_send,get_sys_clock() - starttime);
       return;
    }
  }

	rc= s[dest_id].sock.send(&sbuf,NN_MSG,0);

	// Check for a send error
	if(rc < 0) {
		printf("send Error: %d %s\n",errno,strerror(errno));
		assert(false);
	}

  INC_STATS(_thd_id,time_tport_send,get_sys_clock() - starttime);
	INC_STATS(_thd_id,msg_sent_cnt,1);
	INC_STATS(_thd_id,msg_bytes,size);
}

void Transport::check_delayed_messages() {
    assert(g_network_delay > 0);
    DelayMessage * dmsg = NULL;
    while ((dmsg = (DelayMessage *) delay_queue->get_next_entry()) != NULL) {
        DEBUG("In check_delayed_messages : sending message on the delay queue\n");
        send_msg_no_delay(dmsg);
    }
}

void Transport::send_msg_no_delay(DelayMessage * msg) {
    assert(g_network_delay > 0);

    uint64_t starttime = get_sys_clock();
    // dest_id
	uint64_t dest_id = ((uint32_t*)msg->_sbuf)[0];
    // return_id
	uint64_t return_id = ((uint32_t*)msg->_sbuf)[1];
    DEBUG("In send_msg_no_delay: dest_id = %lu, return_id = %lu\n",dest_id, return_id);
    assert (dest_id == msg->_dest_id);
    assert (return_id == get_node_id());

    int rc;
	rc= s[msg->_dest_id].sock.send(&msg->_sbuf,NN_MSG,0);

	// Check for a send error
	if(rc < 0) {
		printf("send Error: %d %s\n",errno,strerror(errno));
		assert(false);
	}

	INC_STATS(_thd_id,msg_sent_cnt,1);
  INC_STATS(_thd_id,time_tport_send,get_sys_clock() - starttime);
	INC_STATS(_thd_id,msg_bytes,msg->_size);
}

// Listens to socket for messages from other nodes
bool Transport::recv_msg() {
	int bytes = 0;
	void * buf;
  uint64_t starttime = get_sys_clock();
	
	for(uint64_t i=0;i<_sock_cnt;i++) {
		bytes = s[rr++ % _sock_cnt].sock.recv(&buf, NN_MSG, NN_DONTWAIT);

    if(rr == UINT64_MAX)
      rr = 0;

		if(bytes <= 0 && errno != 11) {
		  printf("Recv Error %d %s\n",errno,strerror(errno));
			nn::freemsg(buf);	
		}

		if(bytes>0)
			break;
	}

    /*
	if(bytes < 0 && errno != 11) {
		printf("Recv Error %d %s\n",errno,strerror(errno));
	}
  */
	// Discard any messages not intended for this node
	if(bytes <= 0 ) {
    return false;
	}

	// Calculate time of message delay
  /*
	ts_t time;
	memcpy(&time,&((char*)buf)[bytes-sizeof(ts_t)],sizeof(ts_t));
  */

	ts_t time2 = get_sys_clock();
	//INC_STATS(_thd_id,tport_lat,time2 - time);

  INC_STATS(_thd_id,time_tport_rcv, time2 - starttime);

	starttime = get_sys_clock();
	INC_STATS(_thd_id,msg_rcv_cnt,1);

  /*
	uint32_t return_id __attribute__ ((unused)); // for debug only
	uint32_t dest_id;
#if CC_ALG == CALVIN
	if (get_node_id() == _node_cnt - 1) {
		query = (void *) rem_qry_man.unpack_client_query(buf,bytes);
		return_id = ((base_client_query *)query)->return_id;
		memcpy(&dest_id,buf,sizeof(uint32_t));
	} else {
		query = (void *) rem_qry_man.unpack(buf,bytes);
		return_id = ((base_query *)query)->return_id;
		dest_id = ((base_query *)query)->dest_id;
	}
#else
	query = (void *) rem_qry_man.unpack(buf,bytes);
	return_id = ((base_query *)query)->return_id;
	dest_id = ((base_query *)query)->dest_id;
#endif
*/
  rem_qry_man.unpack(buf,bytes);

	//DEBUG("Msg delay: %d->%d, %d bytes, %f s\n",return_id,
  //          dest_id,bytes,((float)(time2-time))/BILLION);
	nn::freemsg(buf);	
  //assert(dest_id == get_node_id());

	INC_STATS(_thd_id,time_unpack,get_sys_clock()-starttime);
  return true;
}

void Transport::simple_send_msg(int size) {
	void * sbuf = nn_allocmsg(size,0);

	ts_t time = get_sys_clock();
	memcpy(&((char*)sbuf)[0],&time,sizeof(ts_t));

  int rc = -1;
  while(rc < 0 ) {
    //rc = s[(g_node_id + 1) % _node_cnt].sock.send(&sbuf,NN_MSG,0);
  if(g_node_id == 0)
    rc = s[3].sock.send(&sbuf,NN_MSG,0);
  else
    rc = s[2].sock.send(&sbuf,NN_MSG,0);
	}
}

uint64_t Transport::simple_recv_msg() {
	int bytes;
	void * buf;

  if(g_node_id == 0)
		bytes = s[1].sock.recv(&buf, NN_MSG, NN_DONTWAIT);
  else
		bytes = s[0].sock.recv(&buf, NN_MSG, NN_DONTWAIT);
    if(bytes <= 0 ) {
      if(errno != 11)
        nn::freemsg(buf);	
      return 0;
    }

	ts_t time;
	memcpy(&time,&((char*)buf)[0],sizeof(ts_t));
	//printf("%d bytes, %f s\n",bytes,((float)(get_sys_clock()-time)) / BILLION);

	nn::freemsg(buf);	
	return bytes;
}

void * DelayQueue::get_next_entry() {
  assert (g_network_delay > 0);
  q_entry_t next_entry = NULL;
  DelayMessage * data = NULL;

  pthread_mutex_lock(&mtx);

  assert( ( (cnt == 0) && head == NULL && tail == NULL) || ( (cnt > 0) && head != NULL && tail !=NULL) );

  if(cnt > 0) {
    next_entry = head;
	data = (DelayMessage *) next_entry->entry;
	assert(data != NULL);
    uint64_t nowtime = get_sys_clock();

    DEBUG("DelayQueue: current delay time for head query: %lu\n",nowtime - data->_start_ts);
    // Check whether delay ns have passed
    if (nowtime - data->_start_ts < g_network_delay) {
        pthread_mutex_unlock(&mtx);
        return NULL;
    }
    head = head->next;
	free(next_entry);
    cnt--;

    if(cnt == 0) {
      tail = NULL;
    }
  }

  if(cnt == 0 && last_add_time != 0) {
    //INC_STATS(0,qq_full,nowtime - last_add_time);
    last_add_time = 0;
  }

  pthread_mutex_unlock(&mtx);
  return data;
}

