#include "global.h"
#include "helper.h"
#include "stats.h"
#include "mem_alloc.h"
#include "client_txn.h"
#include <time.h>
#include <sys/times.h>
#include <sys/vtimes.h>

void StatsArr::quicksort(int low_idx, int high_idx) {
  int low = low_idx;
  int high = high_idx;
  uint64_t pivot = arr[(low+high) / 2];
  uint64_t tmp;

  while(low < high) {
    while(arr[low] < pivot)
      low++;
    while(arr[high] > pivot)
      high--;
    if(low <= high) {
      tmp = arr[low];
      arr[low] = arr[high];
      arr[high] = tmp;
      low++;
      high--;
    }
  }

  if(low_idx < high)
    quicksort(low_idx,high);
  if(low < high_idx)
    quicksort(low,high_idx);

}

void StatsArr::init(uint64_t size,StatsArrType type) {
	arr = (uint64_t *)
		mem_allocator.alloc(sizeof(uint64_t) * (size+1), 0);
  for(uint64_t i=0;i<size+1;i++) {
    arr[i] = 0;
  }
  this->size = size+1;
  this->type = type;
  cnt = 0;
}

void StatsArr::resize() {
  size = size * 2;
	arr = (uint64_t *)
		mem_allocator.realloc(arr,sizeof(uint64_t) * size, 0);
  for(uint64_t i=size/2;i<size;i++) {
    arr[i] = 0;
  }
}

void StatsArr::insert(uint64_t item) {
  if(type == ArrIncr) {
    if(cnt == size)
      resize();
    arr[cnt++] = item;
  }
  else if(type == ArrInsert) {
    if(item >= size) {
      arr[size-1]++;
    }
    else {
      arr[item]++;
    }
    cnt++;
  }
}

void StatsArr::print(FILE * f) {
  if(type == ArrIncr) {
	  for (UInt32 i = 0; i < cnt; i ++) {
	    fprintf(f,"%ld,", arr[i]);
	  }
  }
  else if(type == ArrInsert) {
	  for (UInt64 i = 0; i < size; i ++) {
      if(arr[i] > 0)
	      fprintf(f,"%ld=%ld,", i,arr[i]);
	  }
  }
}

void StatsArr::print(FILE * f,uint64_t min, uint64_t max) {
  if(type == ArrIncr) {
	  for (UInt32 i = min * cnt / 100; i < max * cnt / 100; i ++) {
	    fprintf(f,"%ld,", arr[i]);
	  }
  }
}


uint64_t StatsArr::get_idx(uint64_t idx) {
  return arr[idx];
}

uint64_t StatsArr::get_percentile(uint64_t ile) {
  return arr[ile * cnt / 100];
}

uint64_t StatsArr::get_avg() {
  uint64_t sum = 0;
  for(uint64_t i = 0;i < cnt;i++) {
    sum+=arr[i];
  }
  if(cnt > 0)
    return sum / cnt;
  return 0;
}


void Stats_thd::init(uint64_t thd_id) {
  part_cnt = (uint64_t*) mem_allocator.alloc(sizeof(uint64_t)*g_part_cnt,0);
  part_acc = (uint64_t*) mem_allocator.alloc(sizeof(uint64_t)*g_part_cnt,0);
	clear();
//	all_lat = new uint64_t [g_max_txn_per_part]; 
	all_lat.init(g_max_txn_per_part,ArrIncr);
  /*
	all_lat = (uint64_t *)
		mem_allocator.alloc(sizeof(uint64_t) * g_max_txn_per_part, thd_id);
    */

  uint64_t nwh = g_num_wh+1;
  uint64_t nd = g_dist_per_wh+1;
  uint64_t ni = g_max_items+1;
  all_abort.init(STAT_ARR_SIZE,ArrInsert);
  w_cflt.init(nwh,ArrInsert);
  d_cflt.init(nwh*nd,ArrInsert);
  cnp_cflt.init(nwh * nd * g_cust_per_dist,ArrInsert);
  c_cflt.init(nwh * nd * g_cust_per_dist,ArrInsert);
  ol_cflt.init(ni,ArrInsert);
  s_cflt.init(nwh * ni,ArrInsert);
  w_abrt.init(nwh,ArrInsert);
  d_abrt.init(nwh*nd,ArrInsert);
  cnp_abrt.init(nwh * nd * g_cust_per_dist,ArrInsert);
  c_abrt.init(nwh * nd * g_cust_per_dist,ArrInsert);
  ol_abrt.init(ni,ArrInsert);
  s_abrt.init(nwh * ni,ArrInsert);

}

void Stats_thd::clear() {
	txn_cnt = 0;
	txn_rem_cnt = 0;
	abort_cnt = 0;
	abort_rem_row_cnt = 0;
	rem_row_cnt = 0;
	abort_row_cnt = 0;
	abort_wr_cnt = 0;
	abort_rem_cnt = 0;
	abort_rem_txn_row_cnt = 0;
	txn_abort_cnt = 0;
	rbk_abort_cnt = 0;
	abort_from_ts = 0;
	tot_run_time = 0;
	run_time = 0;
  finish_time = 0;
  access_cnt = 0;
  write_cnt = 0;
	time_clock_wait = 0;
	time_clock_rwait = 0;
	time_work = 0;
	time_man = 0;
	time_rqry = 0;
	time_lock_man = 0;
	time_clock_wait = 0;
	time_clock_rwait = 0;
	debug1 = 0;
	debug2 = 0;
	debug3 = 0;
	debug4 = 0;
	debug5 = 0;
	rlk = 0;
	rulk = 0;
	rlk_rsp = 0;
	rulk_rsp = 0;
	rqry = 0;
	rqry_rsp = 0;
	rfin = 0;
	rfwd = 0;
	rack = 0;
	rprep = 0;
	rinit = 0;
	rtxn = 0;
	time_index = 0;
	rtime_index = 0;
	time_abort = 0;
	time_cleanup = 0;
	time_qq = 0;
	time_wait = 0;
	time_wait_lock_rem = 0;
	time_wait_lock = 0;
	time_wait_rem = 0;
  time_tport_send = 0;
  time_tport_rcv = 0;
  time_validate = 0;
	time_ts_alloc = 0;
	latency = 0;
	tport_lat = 0;
	time_query = 0;
	rtime_proc = 0;
	time_unpack = 0;
	lock_diff = 0;
  mq_full = 0;
  mbuf_send_time = 0;
  qq_full = 0;
  qq_cnt = 0;
  qq_lat = 0;
  aq_full = 0;
  owned_time = 0;
  owned_time_rd = 0;
  owned_time_wr = 0;
  twopl_time_diff = 0;
  owned_cnt = 0;
  owned_cnt_rd = 0;
  owned_cnt_wr = 0;

  msg_cnt_serv = 0;
  msg_cnt_cl = 0;
  msg_dly_serv = 0;
  msg_dly_cl = 0;

  prof_time_twopc = 0;
  prof_cc_rel_abort = 0;
  prof_cc_rel_commit = 0;
  thd_prof_get_txn_cnt = 0;
  rthd_prof_1=0; rthd_prof_2=0;
  sthd_prof_1a=0; sthd_prof_1b=0; sthd_prof_2=0; sthd_prof_3=0; sthd_prof_4=0; sthd_prof_5a=0; sthd_prof_5b=0;
  sthd_prof_thd=0;
  thd_prof_thd1=0; thd_prof_thd2=0; thd_prof_thd3=0;
  thd_prof_thd1a=0; thd_prof_thd1b=0; thd_prof_thd1c=0; thd_prof_thd1d=0;
  thd_prof_thd2_loc=0; thd_prof_thd2_rem=0;

  thd_prof_thd_rfin0=0;thd_prof_thd_rfin1=0; thd_prof_thd_rfin2=0;
  thd_prof_thd_rprep0=0;thd_prof_thd_rprep1=0; thd_prof_thd_rprep2=0;
  thd_prof_thd_rqry_rsp0=0;thd_prof_thd_rqry_rsp1=0;
  thd_prof_thd_rqry0=0;thd_prof_thd_rqry1=0;thd_prof_thd_rqry2=0;
  thd_prof_thd_rack0=0;thd_prof_thd_rack1=0; thd_prof_thd_rack2a=0;thd_prof_thd_rack2=0;thd_prof_thd_rack3=0;thd_prof_thd_rack4=0;
  thd_prof_thd_rtxn1a=0;thd_prof_thd_rtxn1b=0; thd_prof_thd_rtxn2=0;thd_prof_thd_rtxn3=0;thd_prof_thd_rtxn4=0;

  for(uint64_t i = 0; i < g_part_cnt; i++) {
    part_cnt[i] = 0;
    part_acc[i] = 0;
  }


  for(int i = 0; i < 17;i++) {
    thd_prof_thd2_type[i] = 0;
    thd_prof_thd3_type[i] = 0;
  }
  thd_prof_wl1=0;
  thd_prof_row1=0;thd_prof_row2=0;thd_prof_row3=0;
  thd_prof_cc1=0;thd_prof_cc2=0;
  thd_prof_cc0=0;thd_prof_cc3=0;
  thd_prof_wq1=0;thd_prof_wq2=0;thd_prof_wq1=3;thd_prof_wq4=0;
  thd_prof_txn1=0;thd_prof_txn2=0;
  thd_prof_txn_table_add=0;thd_prof_txn_table_get=0;
  thd_prof_txn_table_mints1=0;thd_prof_txn_table_mints2=0;
  thd_prof_txn_table0a=0;thd_prof_txn_table1a=0;thd_prof_txn_table2=0;
  thd_prof_txn_table0b=0;thd_prof_txn_table1b=0;thd_prof_txn_table2=0;
  thd_prof_occ_val2a=0;thd_prof_occ_val1=0;thd_prof_occ_val2=0;
  thd_prof_occ_val5=0;thd_prof_occ_val4=0;thd_prof_occ_val3=0;
  thd_prof_mvcc1=0;thd_prof_mvcc2=0;thd_prof_mvcc3=0;
  thd_prof_mvcc4=0;thd_prof_mvcc5=0;thd_prof_mvcc6=0;thd_prof_mvcc7=0;thd_prof_mvcc8=0;thd_prof_mvcc9=0;

  aq_poll=0;
  aq_enqueue=0; aq_dequeue=0;
  wq_enqueue=0; wq_dequeue=0;
  all_wq_enqueue=0; all_wq_dequeue=0;
  rem_wq_enqueue=0; rem_wq_dequeue=0;
  new_wq_enqueue=0; new_wq_dequeue=0;

  time_getqry = 0;
  client_latency = 0;
  txn_sent = 0;

  cc_busy_cnt = 0;
  txn_table_cflt = 0;
  txn_table_cflt_size = 0;
	cc_wait_cnt = 0;
	cc_wait_abrt_cnt = 0;
	cc_wait_time = 0;
	cc_wait_abrt_time = 0;
	cc_hold_time = 0;
	cc_hold_abrt_time = 0;

  txn_time_begintxn = 0;
  txn_time_begintxn2 = 0;
  txn_time_idx = 0;
  txn_time_man = 0;
  txn_time_ts = 0;
  txn_time_abrt = 0;
  txn_time_clean = 0;
  txn_time_copy = 0;
  txn_time_wait = 0;
  txn_time_twopc = 0;
  txn_time_q_abrt = 0;
  txn_time_q_work = 0;
  txn_time_net = 0;
  txn_time_misc = 0;

  occ_check_cnt = 0;
  occ_abort_check_cnt = 0;
  cflt_cnt = 0;
  cflt_cnt_txn = 0;
	mpq_cnt = 0;
  msg_batch_size = 0;
  msg_batch_bytes = 0;
  msg_batch_cnt = 0;
	msg_bytes = 0;
	msg_sent_cnt = 0;
	msg_rcv_cnt = 0;
	time_msg_sent = 0;

  spec_abort_cnt = 0;
  spec_commit_cnt = 0;
  batch_cnt = 0;

  seq_txn_cnt = 0;
  seq_batch_cnt = 0;
  time_seq_batch = 0;
  time_seq_ack = 0;
  time_seq_prep = 0;

}

void Stats_tmp::init() {
	clear();
}

void Stats_tmp::clear() {	
	/*
	time_man = 0;
	time_index = 0;
	time_wait = 0;
	time_wait_lock = 0;
	time_wait_rem = 0;
	*/
	mpq_cnt = 0;
	cc_wait_cnt = 0;
	cc_wait_abrt_cnt = 0;
	cc_wait_time = 0;
	cc_wait_abrt_time = 0;
	cc_hold_time = 0;
	cc_hold_abrt_time = 0;
}

void Stats::init() {
	if (!STATS_ENABLE) 
		return;
	_stats = new Stats_thd * [g_thread_cnt + g_rem_thread_cnt + 2];
	tmp_stats = new Stats_tmp * [g_thread_cnt + g_rem_thread_cnt+ 2];
	dl_detect_time = 0;
	dl_wait_time = 0;
	deadlock = 0;
	cycle_detect = 0;

  last_usr = (long long unsigned int*)
		mem_allocator.alloc(sizeof(long long unsigned int) * (g_thread_cnt + g_rem_thread_cnt), 0);
  last_usr_low = (long long unsigned int*)
		mem_allocator.alloc(sizeof(long long unsigned int) * (g_thread_cnt + g_rem_thread_cnt), 0);
  last_sys = (long long unsigned int*)
		mem_allocator.alloc(sizeof(long long unsigned int) * (g_thread_cnt + g_rem_thread_cnt), 0);
  last_idle = (long long unsigned int*)
		mem_allocator.alloc(sizeof(long long unsigned int) * (g_thread_cnt + g_rem_thread_cnt), 0);
  util_init();

}

void Stats::init(uint64_t thread_id) {
	if (!STATS_ENABLE) 
		return;
	_stats[thread_id] = (Stats_thd *) 
		mem_allocator.alloc(sizeof(Stats_thd), thread_id);
	tmp_stats[thread_id] = (Stats_tmp *)
		mem_allocator.alloc(sizeof(Stats_tmp), thread_id);

	_stats[thread_id]->init(thread_id);
	tmp_stats[thread_id]->init();
}

void Stats::clear(uint64_t tid) {
	if (STATS_ENABLE) {
		_stats[tid]->clear();
		tmp_stats[tid]->clear();

		dl_detect_time = 0;
		dl_wait_time = 0;
		cycle_detect = 0;
		deadlock = 0;
	}
}

/*
void Stats::add_lat(uint64_t thd_id, uint64_t latency) {
#if PRT_LAT_DISTR
	if (g_prt_lat_distr && warmup_finish) {
		uint64_t tnum = _stats[thd_id]->txn_cnt;
		_stats[thd_id]->all_lat[tnum] = latency;
	}
#endif
}
*/

void Stats::commit(uint64_t thd_id) {
	if (STATS_ENABLE) {
		_stats[thd_id]->mpq_cnt += tmp_stats[thd_id]->mpq_cnt;
		_stats[thd_id]->cc_wait_cnt += tmp_stats[thd_id]->cc_wait_cnt;
		_stats[thd_id]->cc_wait_abrt_cnt += tmp_stats[thd_id]->cc_wait_abrt_cnt;
		_stats[thd_id]->cc_wait_time += tmp_stats[thd_id]->cc_wait_time;
		_stats[thd_id]->cc_wait_abrt_time += tmp_stats[thd_id]->cc_wait_abrt_time;
		_stats[thd_id]->cc_hold_time += tmp_stats[thd_id]->cc_hold_time;
		_stats[thd_id]->cc_hold_abrt_time += tmp_stats[thd_id]->cc_hold_abrt_time;
		tmp_stats[thd_id]->init();
	}
}

void Stats::abort(uint64_t thd_id) {	
	if (STATS_ENABLE) 
		tmp_stats[thd_id]->init();
}

void Stats::print_client(bool prog) {
  fflush(stdout);
  if(!STATS_ENABLE)
    return;

	uint64_t total_txn_cnt = 0;
	uint64_t total_txn_sent = 0;
	double total_tot_run_time = 0;
	double total_client_latency = 0;
	double total_time_tport_send = 0;
	double total_time_tport_rcv = 0;
	double total_tport_lat = 0;
	uint64_t total_msg_bytes = 0;
	double total_mbuf_send_time = 0;
	uint64_t total_msg_batch_size = 0;
	uint64_t total_msg_batch_bytes = 0;
	uint64_t total_msg_batch_cnt = 0;
	uint64_t total_msg_sent_cnt = 0;
	uint64_t total_msg_rcv_cnt = 0;
	double total_time_msg_sent = 0;
  double total_time_getqry = 0;
	double total_mq_full = 0;

  uint64_t limit;
  if(g_node_id < g_node_cnt)
    limit =  g_thread_cnt + g_rem_thread_cnt;
  else
    limit =  g_client_thread_cnt + g_client_rem_thread_cnt;
	for (uint64_t tid = 0; tid < limit; tid ++) {
    if(!prog)
		  total_tot_run_time += _stats[tid]->tot_run_time;
		total_txn_cnt += _stats[tid]->txn_cnt;
		total_txn_sent += _stats[tid]->txn_sent;
		total_client_latency += _stats[tid]->client_latency;
		total_time_getqry += _stats[tid]->time_getqry;
		total_time_tport_send += _stats[tid]->time_tport_send;
		total_time_tport_rcv += _stats[tid]->time_tport_rcv;
		total_tport_lat += _stats[tid]->tport_lat;
		total_msg_bytes += _stats[tid]->msg_bytes;
		total_mbuf_send_time += _stats[tid]->mbuf_send_time;
		total_msg_batch_size += _stats[tid]->msg_batch_size;
		total_msg_batch_bytes += _stats[tid]->msg_batch_bytes;
		total_msg_batch_cnt += _stats[tid]->msg_batch_cnt;
		total_msg_sent_cnt += _stats[tid]->msg_sent_cnt;
		total_msg_rcv_cnt += _stats[tid]->msg_rcv_cnt;
		total_time_msg_sent += _stats[tid]->time_msg_sent;
		total_mq_full += _stats[tid]->mq_full;
  }
  if(prog)
		total_tot_run_time += _stats[0]->tot_run_time;
  else {
		total_tot_run_time = total_tot_run_time / g_client_thread_cnt;
	for (uint64_t tid = g_client_thread_cnt; tid < g_client_thread_cnt + g_client_send_thread_cnt; tid ++) {
  printf(
      "sthd_prof_1a=%f,sthd_prof_2=%f,sthd_prof_3=%f,sthd_prof_4=%f,sthd_prof_5a=%f,sthd_prof_1b=%f,sthd_prof_5b=%f,sthd_prof_thd=%f\n"
      ,_stats[tid]->sthd_prof_1a / BILLION
      ,_stats[tid]->sthd_prof_2 / BILLION
       ,_stats[tid]->sthd_prof_3 / BILLION
       ,_stats[tid]->sthd_prof_4 / BILLION
       ,_stats[tid]->sthd_prof_5a / BILLION
      ,_stats[tid]->sthd_prof_1b / BILLION
       ,_stats[tid]->sthd_prof_5b / BILLION
       ,_stats[tid]->sthd_prof_thd / BILLION
      );
  }
	for (uint64_t tid = g_client_thread_cnt+g_client_send_thread_cnt; tid < g_client_thread_cnt + g_client_send_thread_cnt + g_client_rem_thread_cnt; tid ++) {
  printf(
      "rthd_prof_1=%f,rthd_prof_2=%f\n"
      ,_stats[tid]->rthd_prof_1 / BILLION
      ,_stats[tid]->rthd_prof_2 / BILLION
      );
  }
  }

  // Prevent division by 0
  if(total_txn_cnt==0)
    total_txn_cnt = 1;
  if(total_msg_batch_cnt == 0)
    total_msg_batch_cnt = 1;
  if(total_msg_rcv_cnt == 0)
    total_msg_rcv_cnt = 1;

	FILE * outf;
	if (output_file != NULL) 
		outf = fopen(output_file, "w");
  else 
    outf = stdout;
  if(prog)
	  fprintf(outf, "[prog] ");
  else
	  fprintf(outf, "[summary] ");
	fprintf(outf, 
      "clock_time=%f"
      ",txn_cnt=%ld"
      ",txn_sent=%ld"
      ",time_getqry=%f"
      ",latency=%f"
      ",mq_full=%f"
      ",mbuf_send_time=%f"
      ",msg_batch_size=%ld"
      ",msg_batch_cnt=%ld"
      ",avg_msg_batch=%ld"
      ",avg_msg_batch_bytes=%ld"
      ",msg_bytes=%ld"
      ",msg_rcv=%ld"
      ",msg_sent=%ld"
			",time_msg_sent=%f"
      ",time_tport_send=%f"
      ",time_tport_rcv=%f"
      ",tport_lat=%f"
			//"\n"
			,total_tot_run_time / BILLION,
			total_txn_cnt, 
			total_txn_sent, 
			total_time_getqry / BILLION,
		  total_client_latency / total_txn_cnt / BILLION,
			total_mq_full / BILLION,
			total_mbuf_send_time / BILLION / total_msg_batch_cnt, 
			total_msg_batch_size, 
			total_msg_batch_cnt, 
			total_msg_batch_size / total_msg_batch_cnt, 
			total_msg_batch_bytes / total_msg_batch_cnt, 
			total_msg_bytes, 
			total_msg_rcv_cnt, 
			total_msg_sent_cnt, 
      total_time_msg_sent / BILLION,
			total_time_tport_send / BILLION,
			total_time_tport_rcv / BILLION,
			total_tport_lat / BILLION / total_msg_rcv_cnt
		);
    mem_util(outf);
    cpu_util(outf);

    if(prog) {
      fprintf(outf,"\n");
		  //for (uint32_t k = 0; k < g_node_id; ++k) {
		  for (uint32_t k = 0; k < g_servers_per_client; ++k) {
        printf("tif_node%u=%d, "
            ,k,client_man.get_inflight(k)
            );
      }
      printf("\n");
    } else {

      uint64_t tid = 0;
      _stats[tid]->all_lat.quicksort(0,_stats[tid]->all_lat.cnt-1);
	    fprintf(outf, 
          ",lat_min=%ld"
          ",lat_max=%ld"
          ",lat_mean=%ld"
          ",lat_99ile=%ld"
          ",lat_98ile=%ld"
          ",lat_95ile=%ld"
          ",lat_90ile=%ld"
          ",lat_80ile=%ld"
          ",lat_75ile=%ld"
          ",lat_70ile=%ld"
          ",lat_60ile=%ld"
          ",lat_50ile=%ld"
          ",lat_40ile=%ld"
          ",lat_30ile=%ld"
          ",lat_25ile=%ld"
          ",lat_20ile=%ld"
          ",lat_10ile=%ld"
          ",lat_5ile=%ld\n"
          ,_stats[tid]->all_lat.get_idx(0)
          ,_stats[tid]->all_lat.get_idx(_stats[tid]->all_lat.cnt-1)
          ,_stats[tid]->all_lat.get_avg()
          ,_stats[tid]->all_lat.get_percentile(99)
          ,_stats[tid]->all_lat.get_percentile(98)
          ,_stats[tid]->all_lat.get_percentile(95)
          ,_stats[tid]->all_lat.get_percentile(90)
          ,_stats[tid]->all_lat.get_percentile(80)
          ,_stats[tid]->all_lat.get_percentile(75)
          ,_stats[tid]->all_lat.get_percentile(70)
          ,_stats[tid]->all_lat.get_percentile(60)
          ,_stats[tid]->all_lat.get_percentile(50)
          ,_stats[tid]->all_lat.get_percentile(40)
          ,_stats[tid]->all_lat.get_percentile(30)
          ,_stats[tid]->all_lat.get_percentile(25)
          ,_stats[tid]->all_lat.get_percentile(20)
          ,_stats[tid]->all_lat.get_percentile(10)
          ,_stats[tid]->all_lat.get_percentile(5)
          );
	    print_lat_distr(99,100);
    }

	if (output_file != NULL) {
    fflush(outf);
		fclose(outf);
  }
  fflush(stdout);
}

void Stats::print(bool prog) {

  fflush(stdout);
  if(!STATS_ENABLE)
    return;
	
  uint64_t total_part_cnt[g_part_cnt];
  uint64_t total_part_acc[g_part_cnt];
  for(uint64_t i = 0; i < g_part_cnt; i++) {
    total_part_cnt[i] = 0;
    total_part_acc[i] = 0;
  }
	uint64_t total_txn_cnt = 0;
	uint64_t total_txn_rem_cnt = 0;
	uint64_t total_abort_cnt = 0;
	uint64_t total_abort_rem_row_cnt = 0;
	uint64_t total_rem_row_cnt = 0;
	uint64_t total_abort_row_cnt = 0;
	uint64_t total_abort_wr_cnt = 0;
	uint64_t total_abort_rem_cnt = 0;
	uint64_t total_abort_rem_txn_row_cnt = 0;
	uint64_t total_txn_abort_cnt = 0;
	uint64_t total_rbk_abort_cnt = 0;
	uint64_t total_abort_from_ts = 0;
	double total_tot_run_time = 0;
	double total_run_time = 0;
  double total_finish_time = 0;
  uint64_t total_access_cnt = 0;
  uint64_t total_write_cnt = 0;
	double total_time_work = 0;
	double total_time_man = 0;
	double total_time_rqry = 0;
	double total_time_lock_man = 0;
	double total_time_clock_wait = 0;
	double total_time_clock_rwait = 0;
	double total_debug1 = 0;
	double total_debug2 = 0;
	double total_debug3 = 0;
	double total_debug4 = 0;
	double total_debug5 = 0;
	double total_mq_full = 0;
	double total_qq_full = 0;
	double total_qq_cnt = 0;
	double total_qq_lat = 0;
	double total_aq_full = 0;
	double total_time_index = 0;
	double total_rtime_index = 0;
	double total_time_abort = 0;
	double total_time_cleanup = 0;
	double total_time_qq = 0;
	double total_time_wait = 0;
	double total_time_wait_lock_rem = 0;
	double total_time_wait_lock = 0;
	double total_time_wait_rem = 0;
	double total_time_tport_send = 0;
	double total_time_tport_rcv = 0;
	double total_time_validate = 0;
	double total_time_ts_alloc = 0;
	double total_latency = 0;
	double total_tport_lat = 0;
	double total_time_query = 0;
	double total_rtime_proc = 0;
	double total_time_unpack = 0;
	uint64_t total_cc_busy_cnt = 0;
	uint64_t total_txn_table_cflt = 0;
	uint64_t total_txn_table_cflt_size = 0;
	uint64_t total_cc_wait_cnt = 0;
	double total_cc_wait_time = 0;
	double total_cc_hold_time = 0;
	uint64_t total_cc_wait_abrt_cnt = 0;
	double total_cc_wait_abrt_time = 0;
	double total_cc_hold_abrt_time = 0;
	uint64_t total_occ_abort_check_cnt = 0;
	uint64_t total_cflt_cnt = 0;
	uint64_t total_occ_check_cnt = 0;
	uint64_t total_cflt_cnt_txn = 0;
	uint64_t total_spec_commit_cnt = 0;
	uint64_t total_spec_abort_cnt = 0;
	uint64_t total_mpq_cnt = 0;
	double total_mbuf_send_time = 0;
	uint64_t total_msg_batch_size = 0;
	uint64_t total_msg_batch_bytes = 0;
	uint64_t total_msg_batch_cnt = 0;
	uint64_t total_msg_bytes = 0;
	uint64_t total_msg_sent_cnt = 0;
	uint64_t total_msg_rcv_cnt = 0;
	double total_time_msg_sent = 0;
  double total_owned_time = 0;
  double total_owned_time_rd = 0;
  double total_owned_time_wr = 0;
  double total_twopl_time_diff = 0;
  double total_owned_cnt = 0;
  double total_prof_time_twopc = 0;
  double total_prof_cc_rel_abort = 0;
  double total_prof_cc_rel_commit = 0;
  double total_owned_cnt_rd = 0;
  double total_owned_cnt_wr = 0;

  uint64_t total_seq_txn_cnt = 0;
  uint64_t total_seq_batch_cnt = 0;
  double total_time_seq_batch = 0;
  double total_time_seq_ack = 0;
  double total_time_seq_prep = 0;

  double total_aq_poll = 0;
  double total_aq_enqueue=0;double total_aq_dequeue=0;
  double total_wq_enqueue=0;double total_wq_dequeue=0;
  double total_all_wq_enqueue=0;double total_all_wq_dequeue=0;
  double total_rem_wq_enqueue=0;double total_rem_wq_dequeue=0;
  double total_new_wq_enqueue=0;double total_new_wq_dequeue=0;

  double total_txn_time_begintxn = 0;
  double total_txn_time_begintxn2 = 0;
  double total_txn_time_idx = 0;
  double total_txn_time_man = 0;
  double total_txn_time_ts = 0;
  double total_txn_time_abrt = 0;
  double total_txn_time_clean = 0;
  double total_txn_time_copy = 0;
  double total_txn_time_wait = 0;
  double total_txn_time_twopc = 0;
  double total_txn_time_q_abrt = 0;
  double total_txn_time_q_work = 0;
  double total_txn_time_net = 0;
  double total_txn_time_misc = 0;


  uint64_t total_rqry = 0;
  uint64_t total_rqry_rsp = 0;
  uint64_t total_rtxn = 0;
  uint64_t total_rinit = 0;
  uint64_t total_rprep = 0;
  uint64_t total_rfin = 0;
  uint64_t total_rfwd = 0;
  uint64_t total_rack = 0;
  uint64_t total_qry_cnt = 0;

  double total_msg_dly_cl = 0;
  double total_msg_dly_serv = 0;
  uint64_t total_msg_cnt_cl = 0;
  uint64_t total_msg_cnt_serv = 0;
  uint64_t limit;
  if(g_node_id < g_node_cnt)
    limit =  g_thread_cnt + g_rem_thread_cnt;
  else
    limit =  g_client_thread_cnt + g_client_rem_thread_cnt;
	for (uint64_t tid = 0; tid < limit; tid ++) {
		total_txn_cnt += _stats[tid]->txn_cnt;
		total_txn_rem_cnt += _stats[tid]->txn_rem_cnt;
		total_abort_cnt += _stats[tid]->abort_cnt;
		total_rem_row_cnt += _stats[tid]->rem_row_cnt;
		total_abort_rem_row_cnt += _stats[tid]->abort_rem_row_cnt;
		total_abort_row_cnt += _stats[tid]->abort_row_cnt;
		total_abort_wr_cnt += _stats[tid]->abort_wr_cnt;
		total_abort_rem_cnt += _stats[tid]->abort_rem_cnt;
		total_abort_rem_txn_row_cnt += _stats[tid]->abort_rem_txn_row_cnt;
		total_txn_abort_cnt += _stats[tid]->txn_abort_cnt;
		total_rbk_abort_cnt += _stats[tid]->rbk_abort_cnt;
		total_abort_from_ts += _stats[tid]->abort_from_ts;
    if(!prog)
		  total_tot_run_time += _stats[tid]->tot_run_time;
		total_run_time += _stats[tid]->run_time;
    total_finish_time += _stats[tid]->finish_time;
    total_access_cnt += _stats[tid]->access_cnt;
    total_write_cnt += _stats[tid]->write_cnt;
		total_time_work += _stats[tid]->time_work;
		total_time_man += _stats[tid]->time_man;
		total_time_rqry += _stats[tid]->time_rqry;
		total_time_lock_man += _stats[tid]->time_lock_man;
		total_time_clock_wait += _stats[tid]->time_clock_wait;
		total_time_clock_rwait += _stats[tid]->time_clock_rwait;
		total_debug1 += _stats[tid]->debug1;
		total_debug2 += _stats[tid]->debug2;
		total_debug3 += _stats[tid]->debug3;
		total_debug4 += _stats[tid]->debug4;
		total_debug5 += _stats[tid]->debug5;
		total_mbuf_send_time += _stats[tid]->mbuf_send_time;
		total_mq_full += _stats[tid]->mq_full;
		total_qq_full += _stats[tid]->qq_full;
		total_qq_cnt += _stats[tid]->qq_cnt;
		total_qq_lat += _stats[tid]->qq_lat;
		total_aq_full += _stats[tid]->aq_full;
		total_rtxn += _stats[tid]->rtxn;
		total_rqry_rsp += _stats[tid]->rqry_rsp;
		total_rack += _stats[tid]->rack;
		total_rinit += _stats[tid]->rinit;
		total_rqry += _stats[tid]->rqry;
		total_rprep += _stats[tid]->rprep;
		total_rfin += _stats[tid]->rfin;
		total_rfwd += _stats[tid]->rfwd;
		total_time_index += _stats[tid]->time_index;
		total_rtime_index += _stats[tid]->rtime_index;
		total_time_abort += _stats[tid]->time_abort;
		total_time_cleanup += _stats[tid]->time_cleanup;
		total_time_qq += _stats[tid]->time_qq;
		total_time_wait += _stats[tid]->time_wait;
		total_time_wait_lock_rem += _stats[tid]->time_wait_lock_rem;
		total_time_wait_lock += _stats[tid]->time_wait_lock;
		total_time_wait_rem += _stats[tid]->time_wait_rem;
		total_time_tport_send += _stats[tid]->time_tport_send;
		total_time_tport_rcv += _stats[tid]->time_tport_rcv;
		total_time_validate += _stats[tid]->time_validate;
		total_time_ts_alloc += _stats[tid]->time_ts_alloc;
		total_latency += _stats[tid]->latency;
		total_tport_lat += _stats[tid]->tport_lat;
		total_time_query += _stats[tid]->time_query;
		total_rtime_proc += _stats[tid]->rtime_proc;
		total_time_unpack += _stats[tid]->time_unpack;
		total_owned_time += _stats[tid]->owned_time;
		total_owned_time_rd += _stats[tid]->owned_time_rd;
		total_owned_time_wr += _stats[tid]->owned_time_wr;
		total_twopl_time_diff += _stats[tid]->twopl_time_diff;
		total_owned_cnt += _stats[tid]->owned_cnt;
		total_prof_time_twopc += _stats[tid]->prof_time_twopc;
		total_prof_cc_rel_abort += _stats[tid]->prof_cc_rel_abort;
		total_prof_cc_rel_commit += _stats[tid]->prof_cc_rel_commit;
		total_owned_cnt_rd += _stats[tid]->owned_cnt_rd;
		total_owned_cnt_wr += _stats[tid]->owned_cnt_wr;

		total_cc_busy_cnt += _stats[tid]->cc_busy_cnt;
		total_txn_table_cflt += _stats[tid]->txn_table_cflt;
		total_txn_table_cflt_size += _stats[tid]->txn_table_cflt_size;
		total_cc_wait_cnt += _stats[tid]->cc_wait_cnt;
		total_cc_wait_time += _stats[tid]->cc_wait_time;
		total_cc_hold_time += _stats[tid]->cc_hold_time;
		total_cc_wait_abrt_cnt += _stats[tid]->cc_wait_abrt_cnt;
		total_cc_wait_abrt_time += _stats[tid]->cc_wait_abrt_time;
		total_cc_hold_abrt_time += _stats[tid]->cc_hold_abrt_time;
		total_occ_check_cnt += _stats[tid]->occ_check_cnt;
		total_occ_abort_check_cnt += _stats[tid]->occ_abort_check_cnt;
		total_cflt_cnt += _stats[tid]->cflt_cnt;
		total_cflt_cnt_txn += _stats[tid]->cflt_cnt_txn;
		total_spec_commit_cnt += _stats[tid]->spec_commit_cnt;
		total_spec_abort_cnt += _stats[tid]->spec_abort_cnt;
		total_mpq_cnt += _stats[tid]->mpq_cnt;
		total_msg_batch_size += _stats[tid]->msg_batch_size;
		total_msg_batch_bytes += _stats[tid]->msg_batch_bytes;
		total_msg_batch_cnt += _stats[tid]->msg_batch_cnt;
		total_msg_bytes += _stats[tid]->msg_bytes;
		total_msg_sent_cnt += _stats[tid]->msg_sent_cnt;
		total_msg_rcv_cnt += _stats[tid]->msg_rcv_cnt;
		total_time_msg_sent += _stats[tid]->time_msg_sent;
		
  total_txn_time_begintxn += _stats[tid]->txn_time_begintxn;
  total_txn_time_begintxn2 += _stats[tid]->txn_time_begintxn2;
  total_txn_time_idx += _stats[tid]->txn_time_idx;
  total_txn_time_man += _stats[tid]->txn_time_man;
  total_txn_time_ts += _stats[tid]->txn_time_ts;
  total_txn_time_abrt += _stats[tid]->txn_time_abrt;
  total_txn_time_clean += _stats[tid]->txn_time_clean;
  total_txn_time_copy += _stats[tid]->txn_time_copy;
  total_txn_time_wait += _stats[tid]->txn_time_wait;
  total_txn_time_twopc += _stats[tid]->txn_time_twopc;
  total_txn_time_q_abrt += _stats[tid]->txn_time_q_abrt;
  total_txn_time_q_work += _stats[tid]->txn_time_q_work;
  total_txn_time_net += _stats[tid]->txn_time_net;
  total_txn_time_misc += _stats[tid]->txn_time_misc;

  total_seq_txn_cnt += _stats[tid]->seq_txn_cnt;
  total_seq_batch_cnt += _stats[tid]->seq_batch_cnt;
  total_time_seq_batch += _stats[tid]->time_seq_batch;
  total_time_seq_ack += _stats[tid]->time_seq_ack;
  total_time_seq_prep += _stats[tid]->time_seq_prep;
  
 total_aq_poll+= _stats[tid]->aq_poll;
 total_aq_enqueue+= _stats[tid]->aq_enqueue;
 total_aq_dequeue+= _stats[tid]->aq_dequeue;
 total_wq_enqueue+= _stats[tid]->wq_enqueue;
 total_wq_dequeue+= _stats[tid]->wq_dequeue;
 total_all_wq_enqueue+= _stats[tid]->all_wq_enqueue;
 total_all_wq_dequeue+= _stats[tid]->all_wq_dequeue;
 total_rem_wq_enqueue+= _stats[tid]->rem_wq_enqueue;
 total_rem_wq_dequeue+= _stats[tid]->rem_wq_dequeue;
 total_new_wq_enqueue+= _stats[tid]->new_wq_enqueue;
 total_new_wq_dequeue+= _stats[tid]->new_wq_dequeue;

 total_msg_dly_cl += _stats[tid]->msg_dly_cl;
 total_msg_dly_serv += _stats[tid]->msg_dly_serv;
 total_msg_cnt_serv += _stats[tid]->msg_cnt_serv;
 total_msg_cnt_cl += _stats[tid]->msg_cnt_cl;
  for(uint64_t i = 0; i < g_part_cnt; i++) {
    total_part_cnt[i] += _stats[tid]->part_cnt[i];
    total_part_acc[i] += _stats[tid]->part_acc[i];
  }

  total_qry_cnt += _stats[tid]->rtxn +_stats[tid]->rqry_rsp +_stats[tid]->rack +_stats[tid]->rinit +_stats[tid]->rqry +_stats[tid]->rprep +_stats[tid]->rfin;

		printf("[tid=%ld] txn_cnt=%ld,abort_cnt=%ld,txn_rem_cnt=%ld,abort_rem_cnt=%ld\n", 
			tid,
			_stats[tid]->txn_cnt,
			_stats[tid]->abort_cnt,
			_stats[tid]->txn_rem_cnt,
			_stats[tid]->abort_rem_cnt
		);
    fflush(stdout);
	}

  if(prog)
		total_tot_run_time += _stats[0]->tot_run_time;
  else {
		total_tot_run_time = total_tot_run_time / g_thread_cnt;
        if (total_finish_time == 0.0) {
            total_finish_time = total_tot_run_time;
        }

    }
  // prevent division by 0
  total_txn_cnt++;
  if(total_msg_rcv_cnt == 0)
    total_msg_rcv_cnt = 1;
  if(total_msg_batch_cnt == 0)
    total_msg_batch_cnt = 1;
  if(total_qq_cnt == 0)
    total_qq_cnt = 1;

	FILE * outf;
	if (output_file != NULL) 
		outf = fopen(output_file, "w");
  else
    outf = stdout;
  txn_table.snapshot();
  if(prog)
	  fprintf(outf, "[prog] ");
  else
	  fprintf(outf, "[summary] ");
	fprintf(outf, 
      "txn_cnt=%ld"
      ",txn_rem_cnt=%ld"
			",clock_time=%f"
      ",abort_cnt=%ld"
      ",txn_abort_cnt=%ld"
      ",tot_avg_abort_row_cnt=%f"
      ",abort_row_cnt=%ld"
      ",avg_abort_row_cnt=%f"
      ",abort_wr_cnt=%ld"
      ",avg_abort_wr_cnt=%f"
      ",abort_rem_txn_row_cnt=%ld"
      ",abort_rem_txn_cnt=%ld"
      ",rbk_abort_cnt=%ld"
      ",abort_from_ts=%ld"
      ",latency=%f"
      ",run_time=%f"
      ",finish_time=%f"
			,total_txn_cnt-1
			,total_txn_rem_cnt 
			,total_tot_run_time / BILLION
			,total_abort_cnt
			,total_txn_abort_cnt
			,(float)(total_abort_row_cnt + total_abort_rem_row_cnt) / total_abort_cnt
			,total_abort_row_cnt
			,(float)(total_abort_row_cnt) / total_abort_cnt
			,total_abort_wr_cnt
			,(float)total_abort_wr_cnt / total_abort_cnt
			,total_abort_rem_txn_row_cnt
			,total_abort_rem_cnt
			,total_rbk_abort_cnt
			,total_abort_from_ts
			,total_latency / BILLION / total_txn_cnt
			,total_run_time / BILLION
      ,total_finish_time / BILLION
      );

	fprintf(outf, 
      ",access_cnt=%ld"
      ",write_cnt=%ld"
      ",rem_row_cnt=%ld"
      ",aq_full=%f"
			",txn_table_cflt=%ld"
			",txn_table_cflt_size=%ld"
			",cc_busy_cnt=%ld"
			",cc_wait_cnt=%ld"
			",cc_wait_time=%f"
			",cc_hold_time=%f"
			",cc_wait_abrt_cnt=%ld"
			",cc_wait_abrt_time=%f"
			",cc_hold_abrt_time=%f"
      ",owned_time=%f"
      ",owned_time_rd=%f"
      ",owned_time_wr=%f"
      ",owned_cnt=%f"
      ",owned_cnt_rd=%f"
      ",owned_cnt_wr=%f"
      ",twopl_diff=%f"
      ,total_access_cnt
      ,total_write_cnt
      ,total_rem_row_cnt
			,total_aq_full / BILLION
      ,total_txn_table_cflt
      ,total_txn_table_cflt_size
      ,total_cc_busy_cnt
      ,total_cc_wait_cnt
      ,total_cc_wait_time / BILLION
      ,total_cc_hold_time / BILLION
      ,total_cc_wait_abrt_cnt
      ,total_cc_wait_abrt_time / BILLION
      ,total_cc_hold_abrt_time / BILLION
      ,total_owned_time / BILLION / (total_owned_cnt+1)
      ,total_owned_time_rd / BILLION / (total_owned_cnt_rd+1)
      ,total_owned_time_wr / BILLION / (total_owned_cnt_wr+1)
      ,total_owned_cnt
      ,total_owned_cnt_rd
      ,total_owned_cnt_wr
      ,total_twopl_time_diff / BILLION / (total_abort_cnt+1)
      );

	fprintf(outf, 
      ",occ_check_cnt=%ld"
      ",occ_abort_check_cnt=%ld"
      ",cflt_cnt=%ld"
      ",cflt_cnt_txn=%ld"
			",mpq_cnt=%ld"
      ",mbuf_send_time=%f"
      ",msg_batch_size=%ld"
      ",msg_batch_cnt=%ld"
      ",avg_msg_batch=%ld"
      ",avg_msg_batch_bytes=%ld"
      ",msg_bytes=%ld"
      ",msg_rcv=%ld"
      ",msg_sent=%ld"
      ,total_occ_check_cnt
      ,total_occ_abort_check_cnt
      ,total_cflt_cnt
      ,total_cflt_cnt_txn
			,total_mpq_cnt
			,total_mbuf_send_time / BILLION / total_msg_batch_cnt
			,total_msg_batch_size 
			,total_msg_batch_cnt 
			,total_msg_batch_size / total_msg_batch_cnt 
			,total_msg_batch_bytes / total_msg_batch_cnt 
			,total_msg_bytes 
			,total_msg_rcv_cnt 
			,total_msg_sent_cnt
      );

	fprintf(outf, 
      ",seq_txn_cnt=%ld"
      ",seq_batch_cnt=%ld"
      ",time_seq_batch=%f"
      ",time_seq_ack=%f"
      ",time_seq_prep=%f"
      ,total_seq_txn_cnt
      ,total_seq_batch_cnt
      ,total_time_seq_batch / BILLION
      ,total_time_seq_ack / BILLION
      ,total_time_seq_prep / BILLION
      );

	fprintf(outf, 
      ",mq_full=%f"
      ",qq_full=%f"
      ",qq_lat=%f"
      ",qry_cnt=%ld"
      ",qry_rtxn=%ld"
      ",qry_rqry_rsp=%ld"
      ",qry_rack=%ld"
      ",qry_rinit=%ld"
      ",qry_rqry=%ld"
      ",qry_rprep=%ld"
      ",qry_rfin=%ld"
      ",aq_poll=%f"
      ",aq_enqueue=%f"
      ",aq_dequeue=%f"
      ",wq_enqueue=%f"
      ",wq_dequeue=%f"
      ",rem_wq_enqueue=%f"
      ",rem_wq_dequeue=%f"
      ",new_wq_enqueue=%f"
      ",new_wq_dequeue=%f"
      ",all_wq_enqueue=%f"
      ",all_wq_dequeue=%f"
      ",prof_time_twopc=%f"
      ",prof_cc_rel_abort=%f"
      ",prof_cc_rel_commit=%f"
			,total_mq_full / BILLION
			,total_qq_full / BILLION
			,total_qq_lat / total_qq_cnt / BILLION
			,total_qry_cnt
			,total_rtxn
			,total_rqry_rsp 
			,total_rack
			,total_rinit
			,total_rqry 
			,total_rprep
			,total_rfin 
      ,total_aq_poll/BILLION
      ,total_aq_enqueue/BILLION
      ,total_aq_dequeue/BILLION
      ,total_wq_enqueue/BILLION
      ,total_wq_dequeue/BILLION
      ,total_rem_wq_enqueue/BILLION
      ,total_rem_wq_dequeue/BILLION
      ,total_new_wq_enqueue/BILLION
      ,total_new_wq_dequeue/BILLION
      ,total_all_wq_enqueue/BILLION
      ,total_all_wq_dequeue/BILLION
      ,total_prof_time_twopc/BILLION
      ,total_prof_cc_rel_abort/BILLION
      ,total_prof_cc_rel_commit/BILLION
      );
	fprintf(outf, 
      ",spec_abort_cnt=%ld"
      ",spec_commit_cnt=%ld"
      ",time_abort=%f"
      ",time_cleanup=%f"
			",time_index=%f"
      ",time_lock_man=%f"
			",time_man=%f"
			",time_msg_sent=%f"
      ",time_qq=%f"
      ",time_tport_send=%f"
      ",time_tport_rcv=%f"
      ",time_ts_alloc=%f"
      ",time_clock_wait=%f"
      ",time_clock_rwait=%f"
      ",time_validate=%f"
      ",time_wait=%f"
      ",time_wait_lock=%f"
      ",time_wait_lock_rem=%f"
      ",time_wait_rem=%f"
      ",time_work=%f"
      ",time_rqry=%f"
      ",tport_lat=%f"
      ",txn_table_cnt=%ld"
      ",tot_wq_cnt=%ld"
      ",wq_cnt=%ld"
      ",rem_wq_cnt=%ld"
      ",new_wq_cnt=%ld"
      ",aq_cnt=%ld"
      //",mq_cnt=%ld"
      ",mdly_cl=%f"
      ",mdly_serv=%f"
      ,total_spec_abort_cnt
      ,total_spec_commit_cnt
			,total_time_abort / BILLION
			,total_time_cleanup / BILLION
			,total_time_index / BILLION
			,total_time_lock_man / BILLION
			,total_time_man / BILLION
      ,total_time_msg_sent / BILLION
			,total_time_qq / BILLION
			,total_time_tport_send / BILLION
			,total_time_tport_rcv / BILLION
			,total_time_ts_alloc / BILLION
      ,total_time_clock_wait / BILLION
      ,total_time_clock_rwait / BILLION
			,total_time_validate / BILLION
			,total_time_wait / BILLION
			,total_time_wait_lock / BILLION
			,total_time_wait_lock_rem / BILLION
			,total_time_wait_rem / BILLION
			,total_time_work / BILLION
			,total_time_rqry / BILLION
			,total_tport_lat / BILLION / total_msg_rcv_cnt
      ,txn_table.get_cnt()
      ,work_queue.get_cnt()
      ,work_queue.get_wq_cnt()
      ,work_queue.get_rem_wq_cnt()
      ,work_queue.get_new_wq_cnt()
      ,abort_queue.get_abrt_cnt()
      //,msg_queue.get_cnt()
      ,total_msg_dly_cl / BILLION / total_msg_cnt_cl
      ,total_msg_dly_serv / BILLION / total_msg_cnt_serv
      );
	fprintf(outf, 
  ",txn_time_begintxn=%f"
  ",txn_time_begintxn2=%f"
  ",txn_time_idx=%f"
  ",txn_time_man=%f"
  ",txn_time_ts=%f"
  ",txn_time_abrt=%f"
  ",txn_time_clean=%f"
  ",txn_time_copy=%f"
  ",txn_time_wait=%f"
  ",txn_time_twopc=%f"
  ",txn_time_q_abrt=%f"
  ",txn_time_q_work=%f"
  ",txn_time_net=%f"
  ",txn_time_misc=%f"
  ,total_txn_time_begintxn / BILLION
  ,total_txn_time_begintxn2 / BILLION
  ,total_txn_time_idx / BILLION
  ,total_txn_time_man / BILLION
  ,total_txn_time_ts / BILLION
  ,total_txn_time_abrt / BILLION
  ,total_txn_time_clean / BILLION
  ,total_txn_time_copy / BILLION
  ,total_txn_time_wait / BILLION
  ,total_txn_time_twopc / BILLION
  ,total_txn_time_q_abrt / BILLION
  ,total_txn_time_q_work / BILLION
  ,total_txn_time_net / BILLION
  ,total_txn_time_misc / BILLION
		);

  for(uint64_t i = 0; i < g_part_cnt; i++) 
    fprintf(outf,
        ",part_cnt%ld=%ld"
        ,i+1
        ,total_part_cnt[i]
        );
  for(uint64_t i = 0; i < g_part_cnt; i++) 
    fprintf(outf,
        ",part_acc%ld=%ld"
        ,i+1
        ,total_part_acc[i]
        );
  mem_util(outf);
  cpu_util(outf);
  print_prof(outf);

	fprintf(outf, 
  ",debug1=%f"
  ",debug2=%f"
  ",debug3=%f"
  ",debug4=%f"
  ",debug5=%f"
  ,(float)total_debug1 / BILLION
  ,(float)total_debug2 / BILLION
  ,(float)total_debug3 / BILLION
  ,(float)total_debug4 / BILLION
  ,(float)total_debug5 / BILLION
  );
  fprintf(outf,"\n");
  fflush(outf);
  if(!prog) {
    print_cnts(outf);
	  //print_lat_distr();
  }
  fprintf(outf,"\n");
  fflush(outf);
	if (output_file != NULL) {
		fclose(outf);
  }

}

uint64_t Stats::get_txn_cnts() {
    if(!STATS_ENABLE || g_node_id >= g_node_cnt)
        return 0;
    uint64_t limit =  g_thread_cnt + g_rem_thread_cnt;
    uint64_t total_txn_cnt = 0;
	for (uint64_t tid = 0; tid < limit; tid ++) {
		total_txn_cnt += _stats[tid]->txn_cnt;
    }
    //printf("total_txn_cnt: %lu\n",total_txn_cnt);
    return total_txn_cnt;
}

void Stats::print_cnts(FILE * outf) {
  if(!STATS_ENABLE || g_node_id >= g_node_cnt)
    return;
  uint64_t all_abort_cnt = 0;
  uint64_t w_cflt_cnt = 0;
  uint64_t d_cflt_cnt = 0;
  uint64_t cnp_cflt_cnt = 0;
  uint64_t c_cflt_cnt = 0;
  uint64_t ol_cflt_cnt = 0;
  uint64_t s_cflt_cnt = 0;
  uint64_t w_abrt_cnt = 0;
  uint64_t d_abrt_cnt = 0;
  uint64_t cnp_abrt_cnt = 0;
  uint64_t c_abrt_cnt = 0;
  uint64_t ol_abrt_cnt = 0;
  uint64_t s_abrt_cnt = 0;
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) {
   all_abort_cnt += _stats[tid]->all_abort.cnt;
   w_cflt_cnt += _stats[tid]->w_cflt.cnt;
   d_cflt_cnt += _stats[tid]->d_cflt.cnt;
   cnp_cflt_cnt += _stats[tid]->cnp_cflt.cnt;
   c_cflt_cnt += _stats[tid]->c_cflt.cnt;
   ol_cflt_cnt += _stats[tid]->ol_cflt.cnt;
   s_cflt_cnt += _stats[tid]->s_cflt.cnt;
   w_abrt_cnt += _stats[tid]->w_abrt.cnt;
   d_abrt_cnt += _stats[tid]->d_abrt.cnt;
   cnp_abrt_cnt += _stats[tid]->cnp_abrt.cnt;
   c_abrt_cnt += _stats[tid]->c_abrt.cnt;
   ol_abrt_cnt += _stats[tid]->ol_abrt.cnt;
   s_abrt_cnt += _stats[tid]->s_abrt.cnt;
  }
  printf("\n[all_abort %ld] ",all_abort_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->all_abort.print(outf);
#if WORKLOAD == TPCC
  printf("\n[w_cflt %ld] ",w_cflt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->w_cflt.print(outf);
  printf("\n[d_cflt %ld] ",d_cflt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->d_cflt.print(outf);
  printf("\n[cnp_cflt %ld] ",cnp_cflt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->cnp_cflt.print(outf);
  printf("\n[c_cflt %ld] ",c_cflt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->c_cflt.print(outf);
  printf("\n[ol_cflt %ld] ",ol_cflt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->ol_cflt.print(outf);
  printf("\n[s_cflt %ld] ",s_cflt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->s_cflt.print(outf);
  printf("\n[w_abrt %ld] ",w_abrt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->w_abrt.print(outf);
  printf("\n[d_abrt %ld] ",d_abrt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->d_abrt.print(outf);
  printf("\n[cnp_abrt %ld] ",cnp_abrt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->cnp_abrt.print(outf);
  printf("\n[c_abrt %ld] ",c_abrt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->c_abrt.print(outf);
  printf("\n[ol_abrt %ld] ",ol_abrt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->ol_abrt.print(outf);
  printf("\n[s_abrt %ld] ",s_abrt_cnt);
	for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) 
    _stats[tid]->s_abrt.print(outf);
#endif

  fprintf(outf,"\n");

}

void Stats::print_lat_distr() {
#if PRT_LAT_DISTR
  printf("\n[all_lat] ");
  uint64_t limit = 0;
  if(g_node_id < g_node_cnt)
    limit = g_thread_cnt;
  else
    limit = g_client_thread_cnt;
	for (UInt32 tid = 0; tid < limit; tid ++) 
    _stats[tid]->all_lat.print(stdout);
#endif
}

void Stats::print_lat_distr(uint64_t min, uint64_t max) {
#if PRT_LAT_DISTR
  printf("\n[all_lat] ");
  _stats[0]->all_lat.print(stdout,min,max);
#endif
}

void Stats::util_init() {
  struct tms timeSample;
  lastCPU = times(&timeSample);
  lastSysCPU = timeSample.tms_stime;
  lastUserCPU = timeSample.tms_utime;
}

void Stats::print_util() {
}

int Stats::parseLine(char* line){
  int i = strlen(line);
  while (*line < '0' || *line > '9') line++;
  line[i-3] = '\0';
  i = atoi(line);
  return i;
}

void Stats::mem_util(FILE * outf) {
  FILE* file = fopen("/proc/self/status", "r");
  int result = -1;
  char line[128];

// Physical memory used by current process, in KB
  while (fgets(line, 128, file) != NULL){
      if (strncmp(line, "VmRSS:", 6) == 0){
          result = parseLine(line);
          fprintf(outf,
            ",phys_mem_usage=%d"
            ,result
            );
      }
      if (strncmp(line, "VmSize:", 7) == 0){
          result = parseLine(line);
          fprintf(outf,
            ",virt_mem_usage=%d"
            ,result
            );
      }
  }
  fclose(file);

}

void Stats::cpu_util(FILE * outf) {
  clock_t now;
  struct tms timeSample;
  double percent;

  now = times(&timeSample);
  if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
      timeSample.tms_utime < lastUserCPU){
      //Overflow detection. Just skip this value.
      percent = -1.0;
  }
  else{
      percent = (timeSample.tms_stime - lastSysCPU) +
          (timeSample.tms_utime - lastUserCPU);
      percent /= (now - lastCPU);
      if(g_node_id < g_node_cnt) {
        percent /= (g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt);//numProcessors;
      } else {
        percent /= (g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt);//numProcessors;
      }
      percent *= 100;
  }
  fprintf(outf,
      ",cpu_ttl=%f"
      ,percent
    );
  lastCPU = now;
  lastSysCPU = timeSample.tms_stime;
  lastUserCPU = timeSample.tms_utime;
}

void Stats::print_prof(FILE * outf) {

  uint64_t total_thd_prof_get_txn_cnt = 0;
  double total_sthd_prof_1a = 0;
  double total_sthd_prof_2 = 0;
  double total_sthd_prof_3 = 0;
  double total_sthd_prof_4 = 0;
  double total_sthd_prof_5a = 0;
  double total_sthd_prof_1b = 0;
  double total_sthd_prof_5b = 0;
  double total_sthd_prof_thd = 0;
  double total_rthd_prof_1 = 0;
  double total_rthd_prof_2 = 0;
  double total_thd_prof_sum = 0;
        double total_thd_prof_thd1 =0;
        double total_thd_prof_thd2 =0;
        double total_thd_prof_thd3 =0;

        double total_thd_prof_thd1a =0;
        double total_thd_prof_thd1b =0;
        double total_thd_prof_thd1c =0;
        double total_thd_prof_thd1d =0;
        double total_thd_prof_thd2_loc =0;
        double total_thd_prof_thd2_rem =0;

        double total_thd_prof_thd_rfin0 =0;
        double total_thd_prof_thd_rfin1 =0;
        double total_thd_prof_thd_rfin2 =0;
        double total_thd_prof_thd_rprep0 =0;
        double total_thd_prof_thd_rprep1 =0;
        double total_thd_prof_thd_rprep2 =0;
        double total_thd_prof_thd_rqry_rsp0 =0;
        double total_thd_prof_thd_rqry_rsp1 =0;
        double total_thd_prof_thd_rqry0 =0;
        double total_thd_prof_thd_rqry1 =0;
        double total_thd_prof_thd_rqry2 =0;
        double total_thd_prof_thd_rack0 =0;
        double total_thd_prof_thd_rack1 =0;
        double total_thd_prof_thd_rack2a =0;
        double total_thd_prof_thd_rack2 =0;
        double total_thd_prof_thd_rack3 =0;
        double total_thd_prof_thd_rack4 =0;
        double total_thd_prof_thd_rtxn1a =0;
        double total_thd_prof_thd_rtxn1b =0;
        double total_thd_prof_thd_rtxn2 =0;
        double total_thd_prof_thd_rtxn3 =0;
        double total_thd_prof_thd_rtxn4 =0;
        double total_thd_prof_wl1 =0;
        double total_thd_prof_row1 =0;
        double total_thd_prof_row2 =0;
        double total_thd_prof_row3 =0;
        double total_thd_prof_cc0 =0;
        double total_thd_prof_cc1 =0;
        double total_thd_prof_cc2 =0;
        double total_thd_prof_cc3 =0;
        double total_thd_prof_mvcc1 =0;
        double total_thd_prof_mvcc2 =0;
        double total_thd_prof_mvcc3 =0;
        double total_thd_prof_mvcc4 =0;
        double total_thd_prof_mvcc5 =0;
        double total_thd_prof_mvcc6 =0;
        double total_thd_prof_mvcc7 =0;
        double total_thd_prof_mvcc8 =0;
        double total_thd_prof_mvcc9 =0;
        double total_thd_prof_occ_val1 =0;
        double total_thd_prof_occ_val2 =0;
        double total_thd_prof_occ_val2a =0;
        double total_thd_prof_occ_val3 =0;
        double total_thd_prof_occ_val4 =0;
        double total_thd_prof_occ_val5 =0;
        double total_thd_prof_wq1 =0;
        double total_thd_prof_wq2 =0;
        double total_thd_prof_wq3 =0;
        double total_thd_prof_wq4 =0;
        double total_thd_prof_txn1 =0;
        double total_thd_prof_txn2 =0;
        double total_thd_prof_txn_table_add =0;
        double total_thd_prof_txn_table_get =0;
        double total_thd_prof_txn_table_mints1 =0;
        double total_thd_prof_txn_table_mints2 =0;
        double total_thd_prof_txn_table0a =0;
        double total_thd_prof_txn_table1a =0;
        double total_thd_prof_txn_table0b =0;
        double total_thd_prof_txn_table1b =0;
        double total_thd_prof_txn_table2a =0;
        double total_thd_prof_txn_table2 =0;
        double total_thd_prof_type[16];
        for(uint64_t i = 0; i < 16; i++)
          total_thd_prof_type[i] = 0;

	for (uint64_t tid = g_thread_cnt; tid < g_thread_cnt + g_send_thread_cnt; tid ++) {
      total_sthd_prof_1a +=_stats[tid]->sthd_prof_1a;
      total_sthd_prof_2 +=_stats[tid]->sthd_prof_2;
       total_sthd_prof_3 +=_stats[tid]->sthd_prof_3;
       total_sthd_prof_4 +=_stats[tid]->sthd_prof_4;
       total_sthd_prof_5a +=_stats[tid]->sthd_prof_5a;
      total_sthd_prof_1b +=_stats[tid]->sthd_prof_1b;
       total_sthd_prof_5b +=_stats[tid]->sthd_prof_5b;
       total_sthd_prof_thd +=_stats[tid]->sthd_prof_thd;
  }
	for (uint64_t tid = g_thread_cnt+g_send_thread_cnt; tid < g_thread_cnt + g_send_thread_cnt + g_rem_thread_cnt; tid ++) {
      total_rthd_prof_1 +=_stats[tid]->rthd_prof_1;
      total_rthd_prof_2 +=_stats[tid]->rthd_prof_2;
  }


	for (uint64_t tid = 0; tid < g_thread_cnt; tid ++) {
    total_thd_prof_sum += 
        _stats[tid]->thd_prof_thd1
        +_stats[tid]->thd_prof_thd2
        +_stats[tid]->thd_prof_thd3;

        total_thd_prof_get_txn_cnt += _stats[tid]->thd_prof_get_txn_cnt;
        total_thd_prof_thd1 +=_stats[tid]->thd_prof_thd1;
        total_thd_prof_thd2 +=_stats[tid]->thd_prof_thd2;
        total_thd_prof_thd3 +=_stats[tid]->thd_prof_thd3;

        total_thd_prof_thd1a +=_stats[tid]->thd_prof_thd1a;
        total_thd_prof_thd1b +=_stats[tid]->thd_prof_thd1b;
        total_thd_prof_thd1c +=_stats[tid]->thd_prof_thd1c;
        total_thd_prof_thd1d +=_stats[tid]->thd_prof_thd1d;
        total_thd_prof_thd2_loc +=_stats[tid]->thd_prof_thd2_loc;
        total_thd_prof_thd2_rem +=_stats[tid]->thd_prof_thd2_rem;

        total_thd_prof_thd_rfin0 +=_stats[tid]->thd_prof_thd_rfin0;
        total_thd_prof_thd_rfin1 +=_stats[tid]->thd_prof_thd_rfin1;
        total_thd_prof_thd_rfin2 +=_stats[tid]->thd_prof_thd_rfin2;
        total_thd_prof_thd_rprep0 +=_stats[tid]->thd_prof_thd_rprep0;
        total_thd_prof_thd_rprep1 +=_stats[tid]->thd_prof_thd_rprep1;
        total_thd_prof_thd_rprep2 +=_stats[tid]->thd_prof_thd_rprep2;
        total_thd_prof_thd_rqry_rsp0 +=_stats[tid]->thd_prof_thd_rqry_rsp0;
        total_thd_prof_thd_rqry_rsp1 +=_stats[tid]->thd_prof_thd_rqry_rsp1;
        total_thd_prof_thd_rqry0 +=_stats[tid]->thd_prof_thd_rqry0;
        total_thd_prof_thd_rqry1 +=_stats[tid]->thd_prof_thd_rqry1;
        total_thd_prof_thd_rqry2 +=_stats[tid]->thd_prof_thd_rqry2;
        total_thd_prof_thd_rack0 +=_stats[tid]->thd_prof_thd_rack0;
        total_thd_prof_thd_rack1 +=_stats[tid]->thd_prof_thd_rack1;
        total_thd_prof_thd_rack2a +=_stats[tid]->thd_prof_thd_rack2a;
        total_thd_prof_thd_rack2 +=_stats[tid]->thd_prof_thd_rack2;
        total_thd_prof_thd_rack3 +=_stats[tid]->thd_prof_thd_rack3;
        total_thd_prof_thd_rack4 +=_stats[tid]->thd_prof_thd_rack4;
        total_thd_prof_thd_rtxn1a +=_stats[tid]->thd_prof_thd_rtxn1a;
        total_thd_prof_thd_rtxn1b +=_stats[tid]->thd_prof_thd_rtxn1b;
        total_thd_prof_thd_rtxn2 +=_stats[tid]->thd_prof_thd_rtxn2;
        total_thd_prof_thd_rtxn3 +=_stats[tid]->thd_prof_thd_rtxn3;
        total_thd_prof_thd_rtxn4 +=_stats[tid]->thd_prof_thd_rtxn4;
        total_thd_prof_wl1 +=_stats[tid]->thd_prof_wl1;
        total_thd_prof_row1 +=_stats[tid]->thd_prof_row1;
        total_thd_prof_row2 +=_stats[tid]->thd_prof_row2;
        total_thd_prof_row3 +=_stats[tid]->thd_prof_row3;
        total_thd_prof_cc0 +=_stats[tid]->thd_prof_cc0;
        total_thd_prof_cc1 +=_stats[tid]->thd_prof_cc1;
        total_thd_prof_cc2 +=_stats[tid]->thd_prof_cc2;
        total_thd_prof_cc3 +=_stats[tid]->thd_prof_cc3;
        total_thd_prof_mvcc1 +=_stats[tid]->thd_prof_mvcc1;
        total_thd_prof_mvcc2 +=_stats[tid]->thd_prof_mvcc2;
        total_thd_prof_mvcc3 +=_stats[tid]->thd_prof_mvcc3;
        total_thd_prof_mvcc4 +=_stats[tid]->thd_prof_mvcc4;
        total_thd_prof_mvcc5 +=_stats[tid]->thd_prof_mvcc5;
        total_thd_prof_mvcc6 +=_stats[tid]->thd_prof_mvcc6;
        total_thd_prof_mvcc7 +=_stats[tid]->thd_prof_mvcc7;
        total_thd_prof_mvcc8 +=_stats[tid]->thd_prof_mvcc8;
        total_thd_prof_mvcc9 +=_stats[tid]->thd_prof_mvcc9;
        total_thd_prof_occ_val1 +=_stats[tid]->thd_prof_occ_val1;
        total_thd_prof_occ_val2 +=_stats[tid]->thd_prof_occ_val2;
        total_thd_prof_occ_val2a +=_stats[tid]->thd_prof_occ_val2a;
        total_thd_prof_occ_val3 +=_stats[tid]->thd_prof_occ_val3;
        total_thd_prof_occ_val4 +=_stats[tid]->thd_prof_occ_val4;
        total_thd_prof_occ_val5 +=_stats[tid]->thd_prof_occ_val5;
        total_thd_prof_wq1 +=_stats[tid]->thd_prof_wq1;
        total_thd_prof_wq2 +=_stats[tid]->thd_prof_wq2;
        total_thd_prof_wq3 +=_stats[tid]->thd_prof_wq3;
        total_thd_prof_wq4 +=_stats[tid]->thd_prof_wq4;
        total_thd_prof_txn1 +=_stats[tid]->thd_prof_txn1;
        total_thd_prof_txn2 +=_stats[tid]->thd_prof_txn2;
        total_thd_prof_txn_table_add +=_stats[tid]->thd_prof_txn_table_add;
        total_thd_prof_txn_table_get +=_stats[tid]->thd_prof_txn_table_get;
        total_thd_prof_txn_table_mints1 +=_stats[tid]->thd_prof_txn_table_mints1;
        total_thd_prof_txn_table_mints2 +=_stats[tid]->thd_prof_txn_table_mints2;
        total_thd_prof_txn_table0a +=_stats[tid]->thd_prof_txn_table0a;
        total_thd_prof_txn_table1a +=_stats[tid]->thd_prof_txn_table1a;
        total_thd_prof_txn_table0b +=_stats[tid]->thd_prof_txn_table0b;
        total_thd_prof_txn_table1b +=_stats[tid]->thd_prof_txn_table1b;
        total_thd_prof_txn_table2a +=_stats[tid]->thd_prof_txn_table2a;
        total_thd_prof_txn_table2 +=_stats[tid]->thd_prof_txn_table2;
        for(int i = 0; i < 17;i++) {
          total_thd_prof_type[i] +=_stats[tid]->thd_prof_thd2_type[i];
          total_thd_prof_type[i] +=_stats[tid]->thd_prof_thd3_type[i];
        }
  }

    fprintf(outf,
        ",sthd_prof_1a=%f,sthd_prof_2=%f,sthd_prof_3=%f,sthd_prof_4=%f,sthd_prof_5a=%f,sthd_prof_1b=%f,sthd_prof_5b=%f,sthd_prof_thd=%f"
        ",rthd_prof_1=%f,rthd_prof_2=%f"
        ",thd1=%f,thd2=%f,thd3=%f,thd_sum=%f"
        ",thd1a=%f,thd1b=%f,thd1c=%f,thd1d=%f"
        ",thd2_loc=%f,thd2_rem=%f"
        ",rfin0=%f,rfin1=%f,rfin2=%f"
        ",rprep0=%f,rprep1=%f,rprep2=%f"
        ",rqry_rsp0=%f,rqry_rsp1=%f"
        ",rqry0=%f,rqry1=%f,rqry2=%f"
        ",rack0=%f,rack1=%f,rack2a=%f,rack2=%f,rack3=%f,rack4=%f"
        ",rtxn1a=%f,rtxn1b=%f,rtxn2=%f,rtxn3=%f,rtxn4=%f"
        ",wl1=%f"
        ",row1=%f,row2=%f,row3=%f"
        ",cc0=%f,cc1=%f,cc2=%f,cc3=%f"
        ",occ_val1=%f,occ_val2a=%f,occ_val2=%f,occ_val3=%f,occ_val4=%f,occ_val5=%f"
        ",mvcc1=%f,mvcc2=%f,mvcc3=%f,mvcc4=%f,mvcc5=%f,mvcc6=%f,mvcc7=%f,mvcc8=%f,mvcc9=%f"
        ",wq1=%f,wq2=%f"
        ",wq3=%f,wq4=%f"
        ",txn1=%f,txn2=%f"
        ",txn_table_add=%f,txn_table_get=%f"
        ",txn_table_mints1=%f,txn_table_mints2=%f"
        ",txn_table0a=%f,txn_table1a=%f"
        ",txn_table0b=%f,txn_table1b=%f,txn_table2a=%f,txn_table2=%f"
        ",get_txn_cnt=%ld"
  ,total_sthd_prof_1a /BILLION/g_send_thread_cnt
  ,total_sthd_prof_2 /BILLION/g_send_thread_cnt
  ,total_sthd_prof_3 /BILLION/g_send_thread_cnt
  ,total_sthd_prof_4 /BILLION/g_send_thread_cnt
  ,total_sthd_prof_5a /BILLION/g_send_thread_cnt
  ,total_sthd_prof_1b /BILLION/g_send_thread_cnt
  ,total_sthd_prof_5b /BILLION/g_send_thread_cnt
  ,total_sthd_prof_thd /BILLION/g_send_thread_cnt
  ,total_rthd_prof_1 /BILLION/g_rem_thread_cnt
  ,total_rthd_prof_2 /BILLION/g_rem_thread_cnt
        ,total_thd_prof_thd1 /BILLION/g_thread_cnt
        ,total_thd_prof_thd2 /BILLION/g_thread_cnt
        ,total_thd_prof_thd3 /BILLION/g_thread_cnt
  ,total_thd_prof_sum /BILLION/g_thread_cnt

        ,total_thd_prof_thd1a /BILLION/g_thread_cnt
        ,total_thd_prof_thd1b /BILLION/g_thread_cnt
        ,total_thd_prof_thd1c /BILLION/g_thread_cnt
        ,total_thd_prof_thd1d /BILLION/g_thread_cnt
        ,total_thd_prof_thd2_loc /BILLION/g_thread_cnt
        ,total_thd_prof_thd2_rem /BILLION/g_thread_cnt

        ,total_thd_prof_thd_rfin0 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rfin1 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rfin2 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rprep0 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rprep1 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rprep2 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rqry_rsp0 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rqry_rsp1 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rqry0 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rqry1 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rqry2 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rack0 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rack1 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rack2a /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rack2 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rack3 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rack4 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rtxn1a /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rtxn1b /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rtxn2 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rtxn3 /BILLION/g_thread_cnt
        ,total_thd_prof_thd_rtxn4 /BILLION/g_thread_cnt
        ,total_thd_prof_wl1 /BILLION/g_thread_cnt
        ,total_thd_prof_row1 /BILLION/g_thread_cnt
        ,total_thd_prof_row2 /BILLION/g_thread_cnt
        ,total_thd_prof_row3 /BILLION/g_thread_cnt
        ,total_thd_prof_cc0 /BILLION/g_thread_cnt
        ,total_thd_prof_cc1 /BILLION/g_thread_cnt
        ,total_thd_prof_cc2 /BILLION/g_thread_cnt
        ,total_thd_prof_cc3 /BILLION/g_thread_cnt
        ,total_thd_prof_occ_val1 /BILLION/g_thread_cnt
        ,total_thd_prof_occ_val2a /BILLION/g_thread_cnt
        ,total_thd_prof_occ_val2 /BILLION/g_thread_cnt
        ,total_thd_prof_occ_val3 /BILLION/g_thread_cnt
        ,total_thd_prof_occ_val4 /BILLION/g_thread_cnt
        ,total_thd_prof_occ_val5 /BILLION/g_thread_cnt
        ,total_thd_prof_mvcc1 /BILLION/g_thread_cnt
        ,total_thd_prof_mvcc2 /BILLION/g_thread_cnt
        ,total_thd_prof_mvcc3 /BILLION/g_thread_cnt
        ,total_thd_prof_mvcc4 /BILLION/g_thread_cnt
        ,total_thd_prof_mvcc5 /BILLION/g_thread_cnt
        ,total_thd_prof_mvcc6 /BILLION/g_thread_cnt
        ,total_thd_prof_mvcc7 /BILLION/g_thread_cnt
        ,total_thd_prof_mvcc8 /BILLION/g_thread_cnt
        ,total_thd_prof_mvcc9 /BILLION/g_thread_cnt
        ,total_thd_prof_wq1 /BILLION/g_thread_cnt
        ,total_thd_prof_wq2 /BILLION/g_thread_cnt
        ,total_thd_prof_wq3 /BILLION/g_thread_cnt
        ,total_thd_prof_wq4 /BILLION/g_thread_cnt
        ,total_thd_prof_txn1 /BILLION/g_thread_cnt
        ,total_thd_prof_txn2 /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table_add /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table_get /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table_mints1 /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table_mints2 /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table0a /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table1a /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table0b /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table1b /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table2a /BILLION/g_thread_cnt
        ,total_thd_prof_txn_table2 /BILLION/g_thread_cnt
        ,total_thd_prof_get_txn_cnt
        );
        for(int i = 0; i < 16;i++)
          fprintf(outf,
            ",type%d=%f"
            ,i
            ,total_thd_prof_type[i]/BILLION/g_thread_cnt
            );
}
