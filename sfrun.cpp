/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#define __STDC_FORMAT_MACROS

#include "sfrun.hpp"

SfRun::SfRun(struct sfconf* _sfconf, intercomm_area* _icomm) : SfAccess(), SfCallback()
{
  sfconf = _sfconf;
  icomm = _icomm;
  sfDai = new Dai(sfconf, icomm, this);

  lastprinttime = 0;
  isinit = false;
  rti_isinit = false;
  isRunning = true;
}

SfRun::~SfRun(void)
{
  delete sfDai;
}

void SfRun::toggle_mute(int io, int channel)
{
  int physch;
  
  if ((io != IN && io != OUT) || channel < 0 || channel >= sfconf->n_channels[io]) {
    return;
  }
  if (bit_isset_volatile(icomm->ismuted[io], channel)) {
    bit_clr_volatile(icomm->ismuted[io], channel);
  } else {
    bit_set_volatile(icomm->ismuted[io], channel);
  }
  physch = sfconf->virt2phys[io][channel];
  if (sfconf->n_virtperphys[io][physch] == 1) {
    sfDai->toggle_mute(io, physch);
    return;
  }
}

bool SfRun::ismuted(int io, int channel)
{
  if ((io != IN && io != OUT) || channel < 0 || channel >= sfconf->n_channels[io]) {
    return (int)true;
  }
  if ((int)bit_isset_volatile(icomm->ismuted[io], channel) == 1) {
    return true;
  } else {
    return false;
  }
}

int SfRun::set_delay(int io, int channel, int delay)
{
  int physch;
    
  if ((io != IN && io != OUT) || channel < 0 || channel >= sfconf->n_channels[io]) {
    return -1;
  }
  if (delay == icomm->delay[io][channel]) {
    return 0;
  }
  if (delay < 0 || delay > sfconf->maxdelay[io][channel]) {
    return -1;
  }
  physch = sfconf->virt2phys[io][channel];
  if (sfconf->n_virtperphys[io][physch] == 1) {
    if (sfDai->change_delay(io, physch, delay) == -1) {
      return -1;
    }
  }
  icomm->delay[io][channel] = delay;
  return 0;
}

int SfRun::get_delay(int io, int channel)
{
  if ((io != IN && io != OUT) || channel < 0 || channel >= sfconf->n_channels[OUT]) {
    return 0;
  }
  return icomm->delay[io][channel];
}

void SfRun::print_debug(void)
{
  uint64_t tsdiv;
  uint32_t n, i, k;

  printf("\nWARNING: these timestamps only make sense if:\n");
  printf(" - there is only one input device\n");
  printf(" - there is only one output device\n");
  printf(" - there is only one filter process\n");
  printf(" - dai loop does not exceed %d\n\n", DEBUG_MAX_DAI_LOOPS);
  
  tsdiv = (uint64_t)sfconf->cpu_mhz;
  printf("%u periods\n\n", D.periods);
  
  D.periods += 2;
  
  if (sfconf->synched_write) {
    printf("output_process:\n");
    for (n = 0; n < 2; n++) {
      printf("  period %i: (dai loop %d)\n", (int)n - 2, D.o[n].dai_loops);
      if (n == 1) {
	printf("    %" PRIu64 "\tcall synch input 0 (write)\n", (uint64_t)((D.o[n].w_input.ts_call - D.ts_start) / tsdiv));
	printf("    %" PRIu64 "\tret\n\n", (uint64_t)((D.o[n].w_input.ts_ret - D.ts_start) / tsdiv));
      }
      for (i = 0; i < (uint32_t)D.o[n].dai_loops && i < DEBUG_MAX_DAI_LOOPS; i++) {
	printf("    %" PRIu64 "\tcall select fdmax %d\n", (uint64_t)((D.o[n].d[i].select.ts_call - D.ts_start) / tsdiv),D.o[n].d[i].select.fdmax);
	printf("    %" PRIu64 "\tret %d\n", (uint64_t)((D.o[n].d[i].select.ts_ret - D.ts_start) / tsdiv), D.o[n].d[i].select.retval);
	printf("    %" PRIu64 "\twrite(%d, %p, %d, %d)\n", (uint64_t)((D.o[n].d[i].write.ts_call - D.ts_start) / tsdiv),
	       D.o[n].d[i].write.fd, D.o[n].d[i].write.buf, D.o[n].d[i].write.offset, D.o[n].d[i].write.count);
	printf("    %" PRIu64 "\tret %d\n\n", (uint64_t)((D.o[n].d[i].write.ts_ret - D.ts_start) / tsdiv), D.o[n].d[i].write.retval);
      }
      if (n == 0) {
	printf("    %" PRIu64 "\tcall synch input trigger start (write)\n", (uint64_t)((D.o[n].d[0].init.ts_synchfd_call - D.ts_start) / tsdiv));
	printf("    %" PRIu64 "\tret\n\n", (uint64_t)((D.o[n].d[0].init.ts_synchfd_ret - D.ts_start) / tsdiv));
      }
    }
  } else {
    printf("output_process:\n");
    printf("  period -2:\n");
    printf("    %" PRIu64 "\tcall synch input trigger start (write)\n", (uint64_t)((D.o[0].w_input.ts_call - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tret\n\n", (uint64_t)((D.o[0].w_input.ts_ret - D.ts_start) / tsdiv));
    printf("output_process:\n");
    printf("  period -1:\n");
    printf("    %" PRIu64 "\tcall synch input 0 (write)\n", (uint64_t)((D.o[1].w_input.ts_call - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tret\n\n", (uint64_t)((D.o[1].w_input.ts_ret - D.ts_start) / tsdiv));
  }
  
  for (n = 0; n < D.periods && n < DEBUG_RING_BUFFER_SIZE - 2; n++) {
    printf("input_process:\n");
    printf("  period %u: (dai loop %d)\n", n, D.i[n].dai_loops);
    if (n == 0) {
      printf("    %" PRIu64 "\tcall init start\n", (uint64_t)((D.i[n].d[0].init.ts_start_call - D.ts_start) / tsdiv));
      printf("    %" PRIu64 "\tret\n", (uint64_t)((D.i[n].d[0].init.ts_start_ret - D.ts_start) / tsdiv));
    }
    for (i = 0; i < (uint32_t)D.i[n].dai_loops && i < DEBUG_MAX_DAI_LOOPS; i++) {
      printf("    %" PRIu64 "\tcall select fdmax %d\n", (uint64_t)((D.i[n].d[i].select.ts_call - D.ts_start) / tsdiv), D.i[n].d[i].select.fdmax);
      printf("    %" PRIu64 "\tret %d (%" PRIu64 ")\n", (uint64_t)((D.i[n].d[i].select.ts_ret - D.ts_start) / tsdiv),
	     D.i[n].d[i].select.retval, (uint64_t)((D.i[n].d[i].select.ts_ret - D.ts_start) / tsdiv - (D.i[n].d[i].select.ts_call - D.ts_start) / tsdiv));
      printf("    %" PRIu64 "\tread(%d, %p, %d, %d)\n", (uint64_t)((D.i[n].d[i].read.ts_call - D.ts_start) / tsdiv), D.i[n].d[i].read.fd,
	     D.i[n].d[i].read.buf, D.i[n].d[i].read.offset, D.i[n].d[i].read.count);
      printf("    %" PRIu64 "\tret %d\n\n", (uint64_t)((D.i[n].d[i].read.ts_ret - D.ts_start) / tsdiv), D.i[n].d[i].read.retval);
    }
    printf("    %" PRIu64 "\tcall synch output %d (read)\n", (uint64_t)((D.i[n].r_output.ts_call - D.ts_start) / tsdiv), n - 1);
    printf("    %" PRIu64 "\tret\n", (uint64_t)((D.i[n].r_output.ts_ret - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tcall synch filter %d (write)\n", (uint64_t)((D.i[n].w_filter.ts_call - D.ts_start) / tsdiv), n);
    printf("    %" PRIu64 "\tret\n\n", (uint64_t)((D.i[n].w_filter.ts_ret - D.ts_start) / tsdiv));


    printf("filter_process:\n");
    printf("  period %u:\n", n);
    printf("    %" PRIu64 "\tcall synch input %d (read)\n", (uint64_t)((D.f[n].r_input.ts_call - D.ts_start) / tsdiv), n);
    printf("    %" PRIu64 "\tret\n", (uint64_t)((D.f[n].r_input.ts_ret - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tcall mutex\n", (uint64_t)((D.f[n].mutex.ts_call - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tret\n\n", (uint64_t)((D.f[n].mutex.ts_ret - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tcall synch fd\n", (uint64_t)((D.f[n].fsynch_fd.ts_call - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tret\n\n", (uint64_t)((D.f[n].fsynch_fd.ts_ret - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tcall synch td\n", (uint64_t)((D.f[n].fsynch_td.ts_call - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tret\n\n", (uint64_t)((D.f[n].fsynch_td.ts_ret - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tcall synch output %d (write)\n", (uint64_t)((D.f[n].w_output.ts_call - D.ts_start) / tsdiv), n);
    printf("    %" PRIu64 "\tret (%" PRIu64 " from synch input ret)\n\n", (uint64_t)((D.f[n].w_output.ts_ret - D.ts_start) / tsdiv),
	   (uint64_t)((D.f[n].w_output.ts_ret - D.ts_start) / tsdiv - (D.f[n].r_input.ts_ret - D.ts_start) / tsdiv));
    

    n += 2;
    printf("output_process:\n");
    printf("  period %u: (dai loop %d)\n", n - 2, D.o[n].dai_loops);
    printf("    %" PRIu64 "\tcall synch filter %d (read)\n", (uint64_t)((D.o[n].r_filter.ts_call - D.ts_start) / tsdiv), n - 2);
    printf("    %" PRIu64 "\tret (%lld)\n", (uint64_t)((D.o[n].r_filter.ts_ret - D.ts_start) / tsdiv),
	   (int64_t)((D.o[n].r_filter.ts_ret - D.ts_start) / tsdiv - (D.o[n].r_filter.ts_call - D.ts_start) / tsdiv));
    printf("    %" PRIu64 "\tcall synch input %d (write)\n", (uint64_t)((D.o[n].w_input.ts_call - D.ts_start) / tsdiv), n - 1);
    printf("    %" PRIu64 "\tret (%lld)\n\n", (uint64_t)((D.o[n].w_input.ts_ret - D.ts_start) / tsdiv),
	   (int64_t)((D.o[n].w_input.ts_ret - D.ts_start) / tsdiv - (D.i[n-2].r_output.ts_ret - D.ts_start) / tsdiv));
    if (!sfconf->synched_write && n == 2) {
      printf("    %" PRIu64 "\tcall init start\n", (uint64_t)((D.o[n].d[0].init.ts_start_call - D.ts_start) / tsdiv));
      printf("    %" PRIu64 "\tret\n", (uint64_t)((D.o[n].d[0].init.ts_start_ret - D.ts_start) / tsdiv));
    }
    for (i = 0; i < (uint32_t)D.o[n].dai_loops && i < DEBUG_MAX_DAI_LOOPS; i++) {
      printf("    %" PRIu64 "\tcall select fdmax %d\n", (uint64_t)((D.o[n].d[i].select.ts_call - D.ts_start) / tsdiv), D.o[n].d[i].select.fdmax);
      printf("    %" PRIu64 "\tret %d\n", (uint64_t)((D.o[n].d[i].select.ts_ret - D.ts_start) / tsdiv), D.o[n].d[i].select.retval);
      if (D.o[n-1].dai_loops > DEBUG_MAX_DAI_LOOPS) {
	k = DEBUG_MAX_DAI_LOOPS - 1;
      } else {
	k = D.o[n-1].dai_loops - 1;
      }
      printf("    %" PRIu64 "\twrite(%d, %p, %d, %d) (%" PRIu64 ")\n", (uint64_t)((D.o[n].d[i].write.ts_call - D.ts_start) / tsdiv),
	     D.o[n].d[i].write.fd, D.o[n].d[i].write.buf, D.o[n].d[i].write.offset, D.o[n].d[i].write.count,
	     (uint64_t)((D.o[n].d[i].write.ts_call - D.ts_start) / tsdiv - (D.o[n-1].d[k-1].write.ts_call - D.ts_start) / tsdiv));
      printf("    %" PRIu64 "\tret %d\n\n", (uint64_t)((D.o[n].d[i].write.ts_ret - D.ts_start) / tsdiv), D.o[n].d[i].write.retval);
    }
    n -= 2; 
  }
}
#undef D

void SfRun::sighandler(int sig)
{
  exit(SF_EXIT_OK);
}

void SfRun::print_overflows(void)
{
  bool is_overflow = false;
  double peak;
  double new_att;
  int n, j;
    
  for (n = 0; n < sfconf->n_channels[OUT]; n++) {
    if (icomm->out_overflow[n].n_overflows > 0) {
      is_overflow = true;
      break;
    }
  }
  if (!is_overflow && !sfconf->show_progress) {
    return;
  }
  pinfo("peak: ");
  new_att = 0;
  for (n = 0; n < sfconf->n_channels[OUT]; n++) {
    peak = icomm->out_overflow[n].largest;
    if (peak < (double)icomm->out_overflow[n].intlargest) {
      peak = (double)icomm->out_overflow[n].intlargest;
    }
    new_att = max(new_att, peak);
    if (peak != 0.0) {
      if ((peak = 20.0 * log10(peak / icomm->out_overflow[n].max)) == 0.0) {
	peak = -0.0; /* we want to display -0.0 rather than +0.0 */
      }
      pinfo("%d/%u/%+.2f ", n, icomm->out_overflow[n].n_overflows, peak);
    } else {
      pinfo("%d/%u/-Inf ", n, icomm->out_overflow[n].n_overflows);
    }
  }   
  if(is_overflow && sfconf->overflow_control) {
    pinfo(" Overflow control output attenuation.");
    for(n = 0; n < sfconf->filters.size(); n++) {
      for(j = 0; j < icomm->fctrl[n].scale[OUT].size(); j++) {
	icomm->fctrl[n].scale[OUT][j] /= new_att;
      } 
    }
  }
  pinfo("\n");
  reset_peak();
}

void SfRun::check_overflows(void)
{
  int n;
  
  if (!sfconf->overflow_warnings) {
    return;
  }
  for (n = 0; n < sfconf->n_logicmods; n++) {
    sflogic[n]->print_overflows();
  }
  print_overflows();
  return;
}

void SfRun::rti_and_overflow(void)
{  
  double rti, max_rti;
  uint32_t period_us;
  bool full_proc;
  time_t tt;
  int n;
  
  if (!rti_isinit) {
    for (n = 0; n < sfconf->n_channels[OUT]; n++) {
      overflow[n] = icomm->out_overflow[n];
    }
    max_period_us = (uint32_t)((uint64_t)sfconf->filter_length * 1000000 / sfconf->sampling_rate);
    rti_isinit = true;
  }
  
  if (icomm->doreset_overflow) {
    icomm->doreset_overflow = false;
    memset(overflow, 0, sizeof(struct sfoverflow) * sfconf->n_channels[OUT]);
  }

  /* calculate realtime index */
  if ((tt = time(NULL)) != lastprinttime) {
    max_rti = 0;
    full_proc = true;
    period_us = icomm->period_us;
    if (period_us == 0) {
      max_rti = 0;
    }                
    rti = (float)period_us / (float)max_period_us;
    if (rti > max_rti) {
      max_rti = rti;
    }
    if (sfconf->show_progress && max_rti != 0) {
      if (full_proc) {
	pinfo("rti: %.3f ---- \n", max_rti);
      } else {
	pinfo("rti: not full processing - no rti update ---- \n");
      }
    }
    icomm->realtime_index = max_rti;
    check_overflows();
    lastprinttime = tt;
  }
}

void SfRun::icomm_mutex(int lock)
{
  if (lock) {
    if (sem_wait(&mutex_sem)==-1) {
      sfstop();
    }
  } else {
    if (sem_post(&mutex_sem)==-1) {
      sfstop();
    }
  }
}

bool SfRun::memiszero(void *buf, int size)
{
  int n, count;
  uint32_t acc;
  
  /* we go through all memory always, to avoid variations in time it takes */
  
  acc = 0;
  count = size >> 2;
  for (n = 0; n < count; n += 4) {
    acc |= ((uint32_t *)buf)[n+0];
    acc |= ((uint32_t *)buf)[n+1];
    acc |= ((uint32_t *)buf)[n+2];
    acc |= ((uint32_t *)buf)[n+3];
  }
  count = size - (count << 2);
  buf = &((uint32_t *)buf)[n];
  for (n = 0; n < count; n++) {
    acc |= ((uint8_t *)buf)[n];
  }
  return acc == 0;
}

bool SfRun::test_silent(void *buf,
			int size,
			int realsize,
			double analog_powersave,
			double scale)
{
  int n, count;
  double dmax;
  float fmaxi;
  
  if (analog_powersave >= 1.0) {
    return memiszero(buf, size);
  }
  if (realsize == 4) {
    fmaxi = 0;
    count = size >> 2;
    for (n = 0; n < count; n++) {
      if (((float *)buf)[n] < 0) {
	if (-((float *)buf)[n] > fmaxi) {
	  fmaxi = -((float *)buf)[n];
	}
      } else {
	if (((float *)buf)[n] > fmaxi) {
	  fmaxi = ((float *)buf)[n];
	}
      }
    }
    dmax = fmaxi;
  } else {
    dmax = 0;
    count = size >> 3;
    for (n = 0; n < count; n++) {
      if (((double *)buf)[n] < 0) {
	if (-((double *)buf)[n] > dmax) {
	  dmax = -((double *)buf)[n];
	}
      } else {
	if (((double *)buf)[n] > dmax) {
	  dmax = ((double *)buf)[n];
	}
      }
    }
  }
  if (scale * dmax >= analog_powersave) {
    return false;
  }
  /* make it truly zero */
  memset(buf, 0, size);
  return true;
}

void* SfRun::dai_trigger_callback_thread(void *args)
{
  sfDai->trigger_callback_io();
  return NULL;
}

void SfRun::filter_process(void)
{

  int convbufsize  = sfConv->convolver_cbufsize();
  int fragsize = sfconf->filter_length;
  int n_blocks = sfconf->n_blocks;
  int curblock = 0;
  int curbuf = 0;
  
  void *input_timecbuf[n_procinputs][2];
  void ***mixconvbuf_inputs;
  void ***mixconvbuf_filters;
  void ***cbuf;
  void **ocbuf;
  void **evalbuf;
  void *static_evalbuf = NULL;
  void *inbuf_copy = NULL;
  
  double* outscale[SF_MAXCHANNELS][sfconf->fproc.n_filters];
  vector<double> scales;
  vector<double> virtscales[2];
  void *crossfadebuf[2];
  void *mixbuf = NULL;
  void *outconvbuf[SF_MAXCHANNELS][sfconf->fproc.n_filters];
  int outconvbuf_n_filters[SF_MAXCHANNELS];
  unsigned int blockcounter = 0;
  delaybuffer_t *output_db[SF_MAXCHANNELS];
  delaybuffer_t *input_db[SF_MAXCHANNELS];
  void *output_sd_rest[SF_MAXCHANNELS];
  void *input_sd_rest[SF_MAXCHANNELS];
  bool need_crossfadebuf = false;
  bool need_mixbuf = false;
  bool mixbuf_is_filled;
  int inbuf_copy_size;
  
  int n, i, j, coeff, delay, cblocks, prevcblocks, physch, virtch;
  struct buffer_format *sf, inbuf_copy_sf;
  uint8_t *memptr, *baseptr;
  struct sfoverflow of;
  
  int memsize;
  vector<int> icomm_delay[2];
  vector<sffilter_control> icomm_fctrl;
  uint32_t icomm_ismuted[2][SF_MAXCHANNELS/32];
  bool change_prio, first_print;
  int dbg_pos;
  
  int prevcoeff[sfconf->fproc.n_filters];
  int procblocks[sfconf->fproc.n_filters];
  uint32_t partial_proc[sfconf->fproc.n_filters / 32 + 1];
  int *mixconvbuf_filters_map[sfconf->fproc.n_filters];
  int outconvbuf_map[SF_MAXCHANNELS][sfconf->fproc.n_filters];
  bool input_freqcbuf_zero[sfconf->n_channels[IN]];
  bool output_freqcbuf_zero[sfconf->n_channels[OUT]];
  bool cbuf_zero[sfconf->fproc.n_filters][n_blocks];
  bool ocbuf_zero[sfconf->fproc.n_filters];
  bool evalbuf_zero[sfconf->fproc.n_filters];
  bool temp_buffer_zero;
  bool iszero;    
  int IO;
  
  struct timeval period_start, period_end, tv;
  int32_t period_length;
  double clockmul;
  uint64_t t1, t2, t3, t4;
  uint64_t t[10];
  uint32_t cc = 0;

  mixconvbuf_inputs = (void***)emalloc(sfconf->fproc.n_filters*sizeof(void**));
  mixconvbuf_filters = (void***)emalloc(sfconf->fproc.n_filters*sizeof(void**));
  cbuf = (void***)emalloc(sfconf->fproc.n_filters*sizeof(void**));
  ocbuf = (void**)emalloc(sfconf->fproc.n_filters*sizeof(void*));
  evalbuf = (void**)emalloc(sfconf->fproc.n_filters*sizeof(void*));
  for (n = 0; n < sfconf->fproc.n_filters; n++) {
    cbuf[n] = (void**)emalloc(n_blocks*sizeof(void*));
  }

  icomm_fctrl.resize(sfconf->fproc.n_filters);
  for (n = 0; n < sfconf->fproc.n_filters; n++) {
    icomm_fctrl[n].scale[IN].resize(sfconf->fproc.filters[n].n_channels[IN], 0);
    icomm_fctrl[n].scale[OUT].resize(sfconf->fproc.filters[n].n_channels[OUT], 0);
    icomm_fctrl[n].fscale.resize(sfconf->fproc.filters[n].n_filters[IN], 0);
  }
  scales.resize(sfconf->fproc.n_filters + sfconf->n_channels[IN], 0);

  dbg_pos = 0;
  first_print = true;
  change_prio = false;
  if (sfDai->minblocksize() == 0 || sfDai->minblocksize() < sfconf->filter_length) {
    change_prio = true;
  }
  temp_buffer_zero = false;
  memset(procblocks, 0, sfconf->fproc.n_filters * sizeof(int));
  memset(partial_proc, 0xFF, (sfconf->fproc.n_filters / 32 + 1) * sizeof(uint32_t));
  memset(evalbuf_zero, 0, sfconf->fproc.n_filters * sizeof(bool));
  memset(ocbuf_zero, 0, sfconf->fproc.n_filters * sizeof(bool));
  memset(cbuf_zero, 0, n_blocks * sfconf->fproc.n_filters * sizeof(bool));
  memset(output_freqcbuf_zero, 0, sfconf->n_channels[OUT] * sizeof(bool));
  memset(input_freqcbuf_zero, 0, sfconf->n_channels[IN] * sizeof(bool));
  memset(crossfadebuf, 0, sizeof(crossfadebuf));
  
  if (sem_wait(&bl_input_2_filter_sem)==-1) { /* for init */
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
  }
    
  /* allocate input delay buffers */
  for (n = j = 0; n < n_procinputs; n++) {
    virtch = channels[IN][n];
    physch = sfconf->virt2phys[IN][virtch];
    input_sd_rest[virtch] = NULL;
    if (sfconf->n_virtperphys[IN][physch] > 1) {
      delay = 0;
      input_db[virtch] = sfDelay->delay_allocate_buffer(fragsize,
                                                        icomm->delay[IN][virtch] + delay,
                                                        sfconf->maxdelay[IN][virtch] + delay,
                                                        sfconf->subdevs[IN].channels.sf.bytes);
      if (sfconf->subdevs[IN].channels.sf.bytes > j) {
        j = sfconf->subdevs[IN].channels.sf.bytes;
      }
    } else {
      /* delays on channels with direct 1-1 virtual-physical mapping are
	 taken care of in the dai module instead */
      input_db[virtch] = NULL;
    }
  }
  inbuf_copy_size = j * fragsize;
  inbuf_copy_sf.sample_spacing = 1;
  inbuf_copy_sf.byte_offset = 0;
  
 /* allocate output delay buffers */
  for (n = 0; n < n_procoutputs; n++) {
    virtch = channels[OUT][n];
    physch = sfconf->virt2phys[OUT][virtch];
    output_sd_rest[virtch] = NULL;
    if (sfconf->n_virtperphys[OUT][physch] > 1) {
      delay = 0;
      output_db[virtch] = sfDelay->delay_allocate_buffer(fragsize,
                                                         icomm->delay[OUT][virtch] + delay,
                                                         sfconf->maxdelay[OUT][virtch] + delay,
                                                         sfconf->realsize);
      need_mixbuf = true;
    } else {
      output_db[virtch] = NULL;
    }
  }
  
  /* find out if there is a need of evaluation buffers, and how many,
     and if there is a need for a crossfade buffer */
  for (n = i = j = 0; n < sfconf->fproc.n_filters; n++) {
    if (sfconf->fproc.filters[n].n_filters[IN] > 0) {
      i++;
    }
    if (sfconf->fproc.filters[n].crossfade) {
      need_crossfadebuf = true;
    }
  }
  
  /* allocate input/output/evaluation convolve buffers */
  if (inbuf_copy_size > convbufsize) {
    /* this should never happen, since convbufsize should be
       2 * fragsize * realsize, sample sizes should never exceed
       8 bytes, and realsize never be smaller than 4 bytes */
    fprintf(stderr, "Unexpected buffer sizes.\n");
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
  }
  if (n_blocks > 1) {
    memsize = sfconf->fproc.n_filters * n_blocks * convbufsize + sfconf->fproc.n_filters * convbufsize + 
      i * (convbufsize + convbufsize / 2) + 2 * n_procinputs * convbufsize;
  } else {
    memsize = sfconf->fproc.n_filters * convbufsize + i * (convbufsize + convbufsize / 2) + 2 * n_procinputs * convbufsize;
  }
  if (i > 0) {
    memsize += convbufsize;
    if (need_crossfadebuf) {
      memsize += convbufsize;
    }
  } else {
    if (need_crossfadebuf) {
      memsize += 2 * convbufsize;
    } else if (need_mixbuf) {
      memsize += fragsize * sfconf->realsize;
    }
  }
  memptr = (uint8_t*)emallocaligned(memsize);
  baseptr = memptr;
  if (i > 0) {
    if (need_crossfadebuf) {
      crossfadebuf[0] = memptr + memsize - 2 * convbufsize;
      crossfadebuf[1] = memptr + memsize - convbufsize;
      static_evalbuf = crossfadebuf[0];
    } else {
      static_evalbuf = memptr + memsize - convbufsize;
    }
    if (need_mixbuf) {
      mixbuf = static_evalbuf;
    }
  } else {
    if (need_crossfadebuf) {
      crossfadebuf[0] = memptr + memsize - 2 * convbufsize;
      crossfadebuf[1] = memptr + memsize - convbufsize;
      if (need_mixbuf) {
	mixbuf = crossfadebuf[0];
      }
    } else if (need_mixbuf) {
      mixbuf = memptr + memsize - fragsize * sfconf->realsize;
    }
  }
  if (n_blocks > 1) {
    for (n = 0; n < sfconf->fproc.n_filters; n++) {
      for (i = 0; i < n_blocks; i++) {
	cbuf[n][i] = memptr;
	memptr += convbufsize;
      }
      if (sfconf->fproc.filters[n].n_filters[IN] > 0) {
	evalbuf[n] = memptr;
	memptr += (convbufsize + convbufsize / 2);
      } else {
	evalbuf[n] = NULL;
      }
      ocbuf[n] = memptr;
      memptr += convbufsize;
    }
  } else {
    for (n = 0; n < sfconf->fproc.filters[n].n_filters[IN]; n++) {
      cbuf[n][0] = ocbuf[n] = memptr;
      memptr += convbufsize;
      if (sfconf->fproc.filters[n].n_filters[IN] > 0) {
	evalbuf[n] = memptr;
	memptr += (convbufsize + convbufsize / 2);
      } else {
	evalbuf[n] = NULL;
      }
    }
  }
  inbuf_copy = ocbuf[0];
  for (n = 0; n < n_procinputs; n++, memptr += 2 * convbufsize) {
    input_timecbuf[n][0] = memptr;
    input_timecbuf[n][1] = memptr + convbufsize;
  }
  /* for each filter, find out which channel-inputs that are mixed */
  for (n = 0; n < sfconf->fproc.n_filters; n++) {
    if (sfconf->fproc.filters[n].n_filters[IN] > 0) {
      /* allocate extra position for filter-input evaluation buffer */
      mixconvbuf_inputs[n] = (void**)alloca((sfconf->fproc.filters[n].n_channels[IN] + 1) * sizeof(void **));
      mixconvbuf_inputs[n][sfconf->fproc.filters[n].n_channels[IN]] = NULL;
    } else if (sfconf->fproc.filters[n].n_channels[IN] == 0) {
      mixconvbuf_inputs[n] = NULL;
      continue;
    } else {
      mixconvbuf_inputs[n] = (void**)alloca(sfconf->fproc.filters[n].n_channels[IN] * sizeof(void **));
    }
    for (i = 0; i < sfconf->fproc.filters[n].n_channels[IN]; i++) {
      mixconvbuf_inputs[n][i] = input_freqcbuf[sfconf->fproc.filters[n].channels[IN][i]];
    }
  }
  /* for each filter, find out which filter-inputs that are mixed */
  for (n = 0; n < sfconf->fproc.n_filters; n++) {
    prevcoeff[n] = icomm->fctrl[sfconf->fproc.filters[n].intname].coeff;
    if (sfconf->fproc.filters[n].n_filters[IN] == 0) {
      mixconvbuf_filters[n] = NULL;
      mixconvbuf_filters_map[n] = NULL;
      continue;
    }
    mixconvbuf_filters[n] = (void**)alloca(sfconf->fproc.filters[n].n_filters[IN] * sizeof(void **));
    mixconvbuf_filters_map[n] = (int*)alloca(sfconf->fproc.filters[n].n_filters[IN] * sizeof(int));
    for (i = 0; i < sfconf->fproc.filters[n].n_filters[IN]; i++) {
      /* find out index of filter */
      for (j = 0; j < sfconf->fproc.n_filters; j++) {
	if (sfconf->fproc.filters[n].filters[IN][i] == sfconf->fproc.filters[j].intname) {
	  break;
	}
      }
      mixconvbuf_filters_map[n][i] = j;
      mixconvbuf_filters[n][i] = ocbuf[j];
    }
  }
  
  /* for each unique output channel, find out which filters that
     mixes its output to it */
  memset(outconvbuf_n_filters, 0, sizeof(outconvbuf_n_filters));
  for (n = 0; n < sfconf->fproc.n_unique_channels[OUT]; n++) {
    for (i = 0; i < sfconf->fproc.n_filters; i++) {
      for (j = 0; j < sfconf->fproc.filters[i].n_channels[OUT]; j++) {
	if (sfconf->fproc.filters[i].channels[OUT][j] == sfconf->fproc.unique_channels[OUT][n]) {
	  outconvbuf_map[n][outconvbuf_n_filters[n]] = i;
	  outconvbuf[n][outconvbuf_n_filters[n]] = ocbuf[i];
	  outscale[n][outconvbuf_n_filters[n]] = &icomm_fctrl[i].scale[OUT][j];
	  outconvbuf_n_filters[n]++;
	  /* output exists only once per filter, we can break here */
	  break;
	}
      }
    }
  }
  
  /* calculate scales */
  FOR_IN_AND_OUT {
    virtscales[IO].resize(sfconf->n_channels[IO], 0);
    for (n = 0; n < sfconf->n_channels[IO]; n++) {
      physch = sfconf->virt2phys[IO][n];
      virtscales[IO][n] = sfDai->dai_buffer_format[IO]->bf[physch].sf.scale;
    }	
  }
  
  if (sfconf->debug) {
    fprintf(stderr, "(%d) got %d inputs, %d outputs\n", (int)getpid(), n_procinputs, n_procoutputs);
    for (n = 0; n < n_procinputs; n++) {
      fprintf(stderr, "(%d) input: %d\n", (int)getpid(), channels[IN][n]);
    }
    for (n = 0; n < n_procoutputs; n++) {
      fprintf(stderr, "(%d) output: %d\n", (int)getpid(), channels[OUT][n]);
    }
  }
  
  /* access all memory while being nobody, so we don't risk getting killed
     later if memory is scarce */
  memset(baseptr, 0, memsize);    
  for (n = 0; n < sfconf->n_coeffs; n++) {
    for (i = 0; i < sfconf->coeffs[n].n_blocks; i++) {
      memcpy(ocbuf[0], sfconf->coeffs_data[n][i], convbufsize);
    }
  }
  memset(ocbuf[0], 0, convbufsize);
  memset(sfDai->buffers[IN][0], 0, sfDai->dai_buffer_format[IN]->n_bytes);
  memset(sfDai->buffers[IN][1], 0, sfDai->dai_buffer_format[IN]->n_bytes);
  memset(sfDai->buffers[OUT][0], 0, sfDai->dai_buffer_format[OUT]->n_bytes);
  memset(sfDai->buffers[OUT][1], 0, sfDai->dai_buffer_format[OUT]->n_bytes);
  for (n = 0; n < sfconf->fproc.n_unique_channels[IN]; n++) {
    memset(input_freqcbuf[sfconf->fproc.unique_channels[IN][n]], 0, convbufsize);
  }
  for (n = 0; n < sfconf->fproc.n_unique_channels[OUT]; n++) {
    memset(output_freqcbuf[sfconf->fproc.unique_channels[OUT][n]], 0, convbufsize);
  }
  
  if (sem_post(&filter_2_bl_output_sem)==-1) { // for init
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    } 
    sfstop();
  }
  
  /* main filter loop starts here */
  memset(t, 0, sizeof(t));
  while (isRunning) {
    gettimeofday(&period_end, NULL);
    
    /* wait for next input buffer */
    timestamp(&icomm->debug.f[dbg_pos].r_input.ts_call);
    if (sem_wait(&cb_input_2_filter_sem) == -1) {
      if (sfconf != NULL && sfconf->debug) {
	print_debug();
      }
      sfstop();
    }
    timestamp(&icomm->debug.f[dbg_pos].r_input.ts_ret);
    
    /* we only calculate period length if all filters are processing
       full length */
    /*if (bit_find(partial_proc, 0, sfconf->fproc.n_filters - 1) == -1) {
      timersub(&period_end, &period_start, &tv);
      period_length = tv.tv_sec * 1000000 + tv.tv_usec;
      icomm->period_us = period_length;
      icomm->full_proc = true;
    } else {
      icomm->full_proc = false;
      }*/
    timersub(&period_end, &period_start, &tv);
    period_length = tv.tv_sec * 1000000 + tv.tv_usec;
    icomm->period_us = period_length;
    icomm->full_proc = true;

    gettimeofday(&period_start, NULL);
    /* get all shared memory data we need where mutex is important */
    timestamp(&icomm->debug.f[dbg_pos].mutex.ts_call);
    icomm_mutex(1);
    for (n = 0; n < sfconf->fproc.n_filters; n++) {
      icomm_fctrl[n].coeff = icomm->fctrl[sfconf->fproc.filters[n].intname].coeff;
      icomm_fctrl[n].delayblocks = icomm->fctrl[sfconf->fproc.filters[n].intname].delayblocks;
      for (i = 0; i < sfconf->fproc.filters[n].n_channels[IN]; i++) {
	icomm_fctrl[n].scale[IN][i] = icomm->fctrl[sfconf->fproc.filters[n].intname].scale[IN][i];
      }
      for (i = 0; i < sfconf->fproc.filters[n].n_channels[OUT]; i++) {
	icomm_fctrl[n].scale[OUT][i] = icomm->fctrl[sfconf->fproc.filters[n].intname].scale[OUT][i];
      }
      for (i = 0; i < sfconf->fproc.filters[n].n_filters[IN]; i++) {
	icomm_fctrl[n].fscale[i] = icomm->fctrl[sfconf->fproc.filters[n].intname].fscale[i];
      }
    }    
    memcpy(icomm_ismuted, icomm->ismuted, sizeof(icomm_ismuted));
    //memcpy(icomm_delay, icomm->delay, sizeof(icomm_delay));
    FOR_IN_AND_OUT {
      icomm_delay[IO] = icomm->delay[IO];
    }
    icomm_mutex(0);

    /* change to lower priority so we can be pre-empted, but we only do so
       if required by the input (or output) process */
    timestamp(&icomm->debug.f[dbg_pos].mutex.ts_ret);
    
    timestamp(&t3);
    for (n = 0; n < n_procinputs; n++) {
      /* convert inputs */
      timestamp(&t1);
      virtch = channels[IN][n];
      physch = sfconf->virt2phys[IN][virtch];
      sf = &sfDai->dai_buffer_format[IN]->bf[physch];
      if (sfconf->n_virtperphys[IN][physch] == 1) {
	of = icomm->in_overflow[virtch];
	convolver_raw2cbuf(sfDai->buffers[IN][curbuf], input_timecbuf[n][curbuf], input_timecbuf[n][!curbuf], sf, &of);
	icomm->in_overflow[virtch] = of;
      } else {
	if (!bit_isset(icomm_ismuted[IN], virtch)) {
	  delay = icomm_delay[IN][virtch];
	  sfDelay->delay_update(input_db[virtch], &((uint8_t *)sfDai->buffers[IN][curbuf])[sf->byte_offset], sf->sf.bytes, sf->sample_spacing, delay, inbuf_copy);

	} else {
	  memset(inbuf_copy, 0, fragsize * sf->sf.bytes);
	}
	inbuf_copy_sf.sf = sf->sf;
	of = icomm->in_overflow[virtch];
	convolver_raw2cbuf(inbuf_copy, input_timecbuf[n][curbuf], input_timecbuf[n][!curbuf], &inbuf_copy_sf, &of);
	icomm->in_overflow[virtch] = of;
      }
      for (i = 0; i < sfconf->n_logicmods; i++) {
	sflogic[i]->input_timed(input_timecbuf[n][curbuf], channels[IN][n]);
      }
      timestamp(&t2);
      t[0] += t2 - t1;
      
      /* transform to frequency domain */
      timestamp(&t1);
      if (!sfconf->powersave || !test_silent(input_timecbuf[n][curbuf], convbufsize, sfconf->realsize, sfconf->analog_powersave, sf->sf.scale)) {
	sfConv->convolver_time2freq(input_timecbuf[n][curbuf], input_freqcbuf[channels[IN][n]]);
	input_freqcbuf_zero[channels[IN][n]] = false;
      } else if (!input_freqcbuf_zero[channels[IN][n]]) {
	memset(input_freqcbuf[channels[IN][n]], 0, convbufsize);
	input_freqcbuf_zero[channels[IN][n]] = true;
      }
      timestamp(&t2);
      t[1] += t2 - t1;
    }
    
    timestamp(&icomm->debug.f[dbg_pos].fsynch_fd.ts_call);
    timestamp(&icomm->debug.f[dbg_pos].fsynch_fd.ts_ret);
    
    for (n = 0; n < sfconf->fproc.n_filters; n++) {
      if (procblocks[n] < n_blocks) {
	procblocks[n]++;
      } else {
	bit_clr(partial_proc, n);
      }
      timestamp(&t1);
      coeff = icomm_fctrl[n].coeff;
      delay = icomm_fctrl[n].delayblocks;
      if (delay < 0) {
	delay = 0;
      } else if (delay > n_blocks - 1) {
	delay = n_blocks - 1;
      }
      if (coeff < 0 ||
	  sfconf->coeffs[coeff].n_blocks > n_blocks - delay)
	{
	  cblocks = n_blocks - delay;
	} else {
	cblocks = sfconf->coeffs[coeff].n_blocks;
      }
      if (prevcoeff[n] < 0 ||
	  sfconf->coeffs[prevcoeff[n]].n_blocks > n_blocks - delay)
	{
	  prevcblocks = n_blocks - delay;
	} else {
	prevcblocks = sfconf->coeffs[prevcoeff[n]].n_blocks;
      }
      
      curblock = (int)((blockcounter + delay) % (unsigned int)(n_blocks));
      
      /* mix and scale inputs prior to convolution */
      if (sfconf->fproc.filters[n].n_filters[IN] > 0) {
	/* mix, scale and reorder filter-inputs for evaluation in the
	   time domain. */
	iszero = true;
	for (i = 0; i < sfconf->fproc.filters[n].n_filters[IN]; i++) {
	  if (!ocbuf_zero[mixconvbuf_filters_map[n][i]]) {
	    iszero = false;
	    break;
	  }
	}
	if (!iszero || !sfconf->powersave) {
	  sfConv->convolver_mixnscale(mixconvbuf_filters[n],
				      static_evalbuf,
				      icomm_fctrl[n].fscale,
				      sfconf->fproc.filters[n].n_filters[IN],
				      CONVOLVER_MIXMODE_OUTPUT);
	  temp_buffer_zero = false;
	} else if (!temp_buffer_zero) {
	  memset(static_evalbuf, 0, convbufsize);
	  temp_buffer_zero = true;
	}
        
	/* evaluate convolution */
	if (!temp_buffer_zero || !evalbuf_zero[n] || !sfconf->powersave) {
	  sfConv->convolver_convolve_eval(static_evalbuf,
					  evalbuf[n],
					  static_evalbuf);
	  evalbuf_zero[n] = false;
	  if (temp_buffer_zero) {
	    evalbuf_zero[n] = true;
	    temp_buffer_zero = false;
	  }
	}
	
	/* mix and scale channel-inputs and reorder prior to
	   convolution */
	iszero = temp_buffer_zero;
	for (i = 0; i < sfconf->fproc.filters[n].n_channels[IN]; i++) {
	  scales[i] = icomm_fctrl[n].scale[IN][i] * virtscales[IN][sfconf->fproc.filters[n].channels[IN][i]];
	  if (!input_freqcbuf_zero[sfconf->fproc.filters[n].channels[IN][i]]) {
	    iszero = false;
	  }
	}
	/* FIXME: unecessary scale multiply for filter-inputs */
	scales[i] = 1.0;
	mixconvbuf_inputs[n][i] = static_evalbuf;
	if (!iszero || !sfconf->powersave) {
	  sfConv->convolver_mixnscale(mixconvbuf_inputs[n],
				      cbuf[n][curblock],
				      scales,
				      sfconf->fproc.filters[n].n_channels[IN] + 1,
				      CONVOLVER_MIXMODE_INPUT);
	  cbuf_zero[n][curblock] = false;
	} else if (!cbuf_zero[n][curblock]) {
	  memset(cbuf[n][curblock], 0, convbufsize);
	  cbuf_zero[n][curblock] = true;
	}
      } else {
	iszero = true;
	for (i = 0; i < sfconf->fproc.filters[n].n_channels[IN]; i++) {
	  scales[i] = icomm_fctrl[n].scale[IN][i] * virtscales[IN][sfconf->fproc.filters[n].channels[IN][i]];
	  if (!input_freqcbuf_zero[sfconf->fproc.filters[n].channels[IN][i]]) {
	    iszero = false;
	  }
	}
	if (!iszero || !sfconf->powersave) {
	  sfConv->convolver_mixnscale(mixconvbuf_inputs[n],
				      cbuf[n][curblock],
				      scales,
				      sfconf->fproc.filters[n].n_channels[IN],
				      CONVOLVER_MIXMODE_INPUT);
	  cbuf_zero[n][curblock] = false;
	} else if (!cbuf_zero[n][curblock]) {
	  memset(cbuf[n][curblock], 0, convbufsize);
                    cbuf_zero[n][curblock] = true;
	}
      }
      timestamp(&t2);
      t[2] += t2 - t1;
      /* convolve (or not) */
      timestamp(&t1);
      
      curblock = (int)(blockcounter % (unsigned int)n_blocks);
      /*
      for (i = 0; i < events.n_pre_convolve; i++) {
	events.pre_convolve[i](cbuf[n][curblock], n);
      }
      */
      if (coeff >= 0) {
	if (n_blocks == 1) {
	  /* curblock is always zero when n_blocks == 1 */
	  if (!cbuf_zero[n][0] || !sfconf->powersave) {
	    if (sfconf->fproc.filters[n].crossfade && prevcoeff[n] != coeff) {
	      if (prevcoeff[n] < 0) {
		sfConv->convolver_dirac_convolve(cbuf[n][0], crossfadebuf[0]);
	      } else {
		sfConv->convolver_convolve(cbuf[n][0], sfconf->coeffs_data[prevcoeff[n]][0], crossfadebuf[0]);
	      }
	      sfConv->convolver_convolve_inplace(cbuf[n][0], sfconf->coeffs_data[coeff][0]); 
	      sfConv->convolver_crossfade_inplace(cbuf[n][0], crossfadebuf[0], crossfadebuf[1]);
	      temp_buffer_zero = false;
	    } else {
	      sfConv->convolver_convolve_inplace(cbuf[n][0], sfconf->coeffs_data[coeff][0]);
	    }
	    /* cbuf points at ocbuf when n_blocks == 1 */
	    ocbuf_zero[n] = false;
	  } else {
	    ocbuf_zero[n] = true;
	    procblocks[n] = 0;
	    bit_set(partial_proc, n);
	  }
	} else {
	  if (!cbuf_zero[n][curblock] || !sfconf->powersave) {
	    if (sfconf->fproc.filters[n].crossfade && prevcoeff[n] != coeff) {
	      if (prevcoeff[n] < 0) {
		sfConv->convolver_dirac_convolve(cbuf[n][curblock], crossfadebuf[0]);
	      } else {
		sfConv->convolver_convolve(cbuf[n][curblock], sfconf->coeffs_data[prevcoeff[n]][0], crossfadebuf[0]);
	      }
	    }
	    sfConv->convolver_convolve(cbuf[n][curblock], sfconf->coeffs_data[coeff][0], ocbuf[n]);
	    ocbuf_zero[n] = false;
	  } else if (!ocbuf_zero[n]) {
	    memset(ocbuf[n], 0, convbufsize);
	    ocbuf_zero[n] = true;
	  }
	  for (i = 1; i < cblocks && i < procblocks[n]; i++) {
	    j = (int)((blockcounter - i) % (unsigned int)n_blocks);
	    if (!cbuf_zero[n][j] || !sfconf->powersave) {
	      sfConv->convolver_convolve_add(cbuf[n][j], sfconf->coeffs_data[coeff][i], ocbuf[n]);
	      ocbuf_zero[n] = false;
	    }
	  }
	  timestamp(&t2);
	  t[3] += t2 - t1;
	  if (sfconf->fproc.filters[n].crossfade && prevcoeff[n] != coeff && prevcoeff[n] >= 0) {
	    for (i = 1; i < prevcblocks && i < procblocks[n]; i++) {
	      j = (int)((blockcounter - i) % (unsigned int)n_blocks);
	      if (!cbuf_zero[n][j] || !sfconf->powersave) {
		sfConv->convolver_convolve_add(cbuf[n][j], sfconf->coeffs_data[prevcoeff[n]][i], crossfadebuf[0]);
	      }
	      ocbuf_zero[n] = false;
	    }
	  }
	  if (ocbuf_zero[n]) {
	    procblocks[n] = 0;
	    bit_set(partial_proc, n);
	  } else if (sfconf->fproc.filters[n].crossfade && prevcoeff[n] != coeff) {
	    sfConv->convolver_crossfade_inplace(ocbuf[n], crossfadebuf[0], crossfadebuf[1]);
	    temp_buffer_zero = false;
	  }
	}
      } else {
	if (n_blocks == 1) {
	  if (!cbuf_zero[n][0] || !sfconf->powersave) {
	    if (sfconf->fproc.filters[n].crossfade && prevcoeff[n] != coeff) {
	      sfConv->convolver_convolve(cbuf[n][0], sfconf->coeffs_data[prevcoeff[n]][0], crossfadebuf[0]);
	      sfConv->convolver_dirac_convolve_inplace(cbuf[n][0]);
	      sfConv->convolver_crossfade_inplace(cbuf[n][0], crossfadebuf[0], crossfadebuf[1]);
	      temp_buffer_zero = false;
	    } else {
	      sfConv->convolver_dirac_convolve_inplace(cbuf[n][0]);
	    }
	    ocbuf_zero[n] = false;
	  } else {
	    ocbuf_zero[n] = true;
	    procblocks[n] = 0;
	    bit_set(partial_proc, n);
	  }
	} else {
	  if (!cbuf_zero[n][curblock] || !sfconf->powersave) {
	    if (sfconf->fproc.filters[n].crossfade && prevcoeff[n] != coeff) {
	      sfConv->convolver_convolve(cbuf[n][curblock], sfconf->coeffs_data[prevcoeff[n]][0], crossfadebuf[0]);
	    }
	    sfConv->convolver_dirac_convolve(cbuf[n][curblock], ocbuf[n]);
	    ocbuf_zero[n] = false;
	  } else if (!ocbuf_zero[n]) {
	    memset(ocbuf[n], 0, convbufsize);
	    ocbuf_zero[n] = true;
	  }
	  if (sfconf->fproc.filters[n].crossfade && prevcoeff[n] != coeff) {
	    for (i = 1; i < prevcblocks && i < procblocks[n]; i++) {
	      j = (int)((blockcounter - i) % (unsigned int)n_blocks);
	      if (!cbuf_zero[n][j] || !sfconf->powersave) {
		sfConv->convolver_convolve_add(cbuf[n][j], sfconf->coeffs_data[prevcoeff[n]][i], crossfadebuf[0]);
	      }
	      ocbuf_zero[n] = false;
	    }
	  }
	  if (ocbuf_zero[n]) {
	    procblocks[n] = 0;
	    bit_set(partial_proc, n);
	  } else if (sfconf->fproc.filters[n].crossfade && prevcoeff[n] != coeff) {
	    sfConv->convolver_crossfade_inplace(ocbuf[n], crossfadebuf[0], crossfadebuf[1]);
	    temp_buffer_zero = false;
	  }
	}
      }
      prevcoeff[n] = coeff;
      timestamp(&t2);
      t[3] += t2 - t1;
    }
    
    timestamp(&t1);
    for (n = 0; n < sfconf->fproc.n_unique_channels[OUT]; n++) {
      iszero = true;
      for (i = 0; i < outconvbuf_n_filters[n]; i++) {
	scales[i] = *outscale[n][i] / virtscales[OUT][sfconf->fproc.unique_channels[OUT][n]];
	if (!ocbuf_zero[outconvbuf_map[n][i]]) {
	  iszero = false;
	}
      }
      /* mix and scale convolve outputs prior to conversion to time
	 domain */
      if (!iszero || !sfconf->powersave) {
	sfConv->convolver_mixnscale(outconvbuf[n], 
				    output_freqcbuf[sfconf->fproc.unique_channels[OUT][n]], 
				    scales, 
				    outconvbuf_n_filters[n], 
				    CONVOLVER_MIXMODE_OUTPUT);
	output_freqcbuf_zero[sfconf->fproc.unique_channels[OUT][n]] = false;
      } else if (!output_freqcbuf_zero[sfconf->fproc.unique_channels[OUT][n]]) {
	memset(output_freqcbuf[sfconf->fproc.unique_channels[OUT][n]], 0, convbufsize);
	output_freqcbuf_zero[sfconf->fproc.unique_channels[OUT][n]] = true;
      }
    }
    timestamp(&t2);
    t[4] += t2 - t1;
    
    timestamp(&icomm->debug.f[dbg_pos].fsynch_td.ts_call);
    timestamp(&icomm->debug.f[dbg_pos].fsynch_td.ts_ret);
    
    mixbuf_is_filled = false;
    for (n = j = 0; n < n_procoutputs; n++) {	    
      /* transform back to time domain */
      timestamp(&t1);
      virtch = channels[OUT][n];
      physch = sfconf->virt2phys[OUT][virtch];
      /*
      for (i = 0; i < events.n_output_freqd; i++) {
	events.output_freqd[i](output_freqcbuf[virtch], virtch);
	}*/
      /* ocbuf[0] happens to be free, that's why we use it */
      if (!output_freqcbuf_zero[virtch] || !sfconf->powersave) {
	sfConv->convolver_freq2time(output_freqcbuf[virtch], ocbuf[0]);
	ocbuf_zero[0] = false;
	if (n_blocks == 1) {
	  cbuf_zero[0][0] = false;
	}
      } else if (!ocbuf_zero[0]) {
	memset(ocbuf[0], 0, convbufsize);
	ocbuf_zero[0] = true;
	if (n_blocks == 1) {
	  cbuf_zero[0][0] = true;
	}
      }
      
      /* Check if there is NaN or Inf values, and abort if so. We cannot
	 afford to check all values, but NaN/Inf tend to spread, so
	 checking only one value usually catches the problem. */
      if ((sfconf->realsize == sizeof(float) &&
	   !finite((double)((float *)ocbuf[0])[0])) ||
	  (sfconf->realsize == sizeof(double) &&
	   !finite(((double *)ocbuf[0])[0])))
	{
	  fprintf(stderr, "NaN or Inf values in the system! Invalid input? Aborting.\n");
	  sfstop();
	}
            
      timestamp(&t2);
      t[5] += t2 - t1;
      
      /* write to output buffer */
      timestamp(&t1);
      for (i = 0; i < sfconf->n_logicmods; i++) {
	sflogic[i]->output_timed(ocbuf[0], virtch);
      }
      if (sfconf->n_virtperphys[OUT][physch] == 1) {
	/* only one virtual channel allocated to this physical one, so
	   we write to it directly */                
	of = icomm->out_overflow[virtch];
	convolver_cbuf2raw(ocbuf[0], sfDai->buffers[OUT][curbuf], &sfDai->dai_buffer_format[OUT]->bf[physch], sfconf->dither_state[physch] != NULL, sfconf->dither_state[physch], &of);
	icomm->out_overflow[virtch] = of;
      } else {
	/* Mute, delay and mix. This is done in the dai module normally,
	   where we get lower I/O-delay on mute and delay operations.
	   However, when mixing to a single physical channel we cannot
	   do it there, so we must do it here instead. */
	delay = icomm_delay[OUT][virtch];
	sfDelay->delay_update(output_db[virtch], ocbuf[0], sfconf->realsize, 1, delay, NULL);
	if (!bit_isset(icomm_ismuted[OUT], virtch)) {
	  if (!mixbuf_is_filled) {
	    memcpy(mixbuf, ocbuf[0], fragsize * sfconf->realsize);
	  } else {
	    if (sfconf->realsize == 4) {
	      for (i = 0; i < fragsize; i += 4) {
		((float *)mixbuf)[i+0] += ((float *)ocbuf[0])[i+0];
		((float *)mixbuf)[i+1] += ((float *)ocbuf[0])[i+1];
		((float *)mixbuf)[i+2] += ((float *)ocbuf[0])[i+2];
		((float *)mixbuf)[i+3] += ((float *)ocbuf[0])[i+3];
	      }
	    } else {
	      for (i = 0; i < fragsize; i += 4) {
		((double *)mixbuf)[i+0] += ((double *)ocbuf[0])[i+0];
		((double *)mixbuf)[i+1] += ((double *)ocbuf[0])[i+1];
		((double *)mixbuf)[i+2] += ((double *)ocbuf[0])[i+2];
		((double *)mixbuf)[i+3] += ((double *)ocbuf[0])[i+3];
	      }
	    }
	  }
	  temp_buffer_zero = false;
	  mixbuf_is_filled = true;
	}
	if (++j == sfconf->n_virtperphys[OUT][physch]) {
	  if (!mixbuf_is_filled) {
	    /* we cannot set temp_buffer_zero here since
	       fragsize * sfconf->realsize is smaller than
	       convbufsize */
	    memset(mixbuf, 0, fragsize * sfconf->realsize);
	  }
	  j = 0;
	  mixbuf_is_filled = false;
	  /* overflow structs are same for all virtual channels
	     assigned to a single physical one, so we copy them */
	  of = icomm->out_overflow[virtch];
	  convolver_cbuf2raw(mixbuf, sfDai->buffers[OUT][curbuf], &sfDai->dai_buffer_format[OUT]->bf[physch], sfconf->dither_state[physch] != NULL, sfconf->dither_state[physch], &of);
	  for (i = 0; i < sfconf->n_virtperphys[OUT][physch]; i++) {
	    icomm->out_overflow[sfconf->phys2virt[OUT][physch][i]] = of;
	  }
	}
      }
      timestamp(&t2);
      t[6] += t2 - t1;
    }
    timestamp(&t4);
    t[7] += t4 - t3;
 
    /* signal the output process */
    timestamp(&icomm->debug.f[dbg_pos].w_output.ts_call);
    if (sem_post(&filter_2_cb_output_sem)==-1) {
      if (sfconf != NULL && sfconf->debug) {
	print_debug();
      }
      sfstop();
    }
    if (sfconf->realtime_priority) {
      sched_yield();
    }
    timestamp(&icomm->debug.f[dbg_pos].w_output.ts_ret);
    
    /* swap convolve buffers */
    curbuf = !curbuf;
    
    /* advance input block */
    blockcounter++;
    if (sfconf->debug || sfconf->benchmark) {
      if (++cc % 10 == 0) {
	if (first_print) {
	  first_print = false;
	  fprintf(stderr, "\n\
  pid ......... process id of filter process\n\
  raw2real .... sample format conversion from input to internal format\n\
  time2freq ... forward fast fourier transform of input buffers\n\
  mixscale1 ... mixing and scaling (volume) of filter input buffers\n\
  convolve .... convolution of filter buffers (and crossfade if used)\n\
  mixscale2 ... mixing and scaling of filter output buffers\n\
  freq2time ... inverse fast fouirer transform of input buffers\n\
  real2raw .... sample format conversion from internal format to output\n\
  total ....... total time required per period\n\
  periods ..... number of periods processed so far\n\
  rti ......... current realtime index\n\
\n  all times are in milliseconds, mean value over 10 periods\n\n\
  pid |  raw2real | time2freq | mixscale1 |  convolve | mixscale2 | \
freq2time |  real2raw |     total | periods | rti \n\
--------------------------------------------------------------------\
----------------------------------------------------\n");
	}
	clockmul = 1.0 / (sfconf->cpu_mhz * 1000.0);
	for (n = 0; n < 8; n++) {
	  t[n] /= 10;
	}
	fprintf(stderr, "%5d | %9.3f | %9.3f | %9.3f | %9.3f |"
		" %9.3f | %9.3f | %9.3f | %9.3f | %7lu | %.3f\n",
		(int)getpid(),
		(double)t[0] * clockmul,
		(double)t[1] * clockmul,
		(double)t[2] * clockmul,
		(double)t[3] * clockmul,
		(double)t[4] * clockmul,
		(double)t[5] * clockmul,
		(double)t[6] * clockmul,
		(double)t[7] * clockmul,
		(unsigned long int)cc,
		icomm->realtime_index);
	memset(t, 0, sizeof(t));
      }
    }
    
    if (++dbg_pos == DEBUG_RING_BUFFER_SIZE) {
      dbg_pos = 0;
    }
  }
}

void* SfRun::filter_process_thread(void *args)
{
  reinterpret_cast<SfRun *>(args)->filter_process();
  return NULL;
}

void SfRun::sf_callback_ready(int io)
{ 
  isinit = true;
  
  if (io == IN) {
    /* trigger filter process(es). Other end will read for each dev */
    if (sem_post(&cb_input_2_filter_sem)==-1) {
      if (sfconf != NULL && sfconf->debug) {
	print_debug();
      }
      sfstop();
    }
  } else {
    /* wait for filter process(es) */
    if (sem_wait(&filter_2_cb_output_sem)==-1) {
      if (sfconf != NULL && sfconf->debug) {
	print_debug();
      }
      sfstop();
    }
    rti_and_overflow(); 
  }
}

void* SfRun::module_init_thread(void *args) 
{
  
  struct module_init_input *input_args;
  input_args = (struct module_init_input*)args;   
  input_args->sflogic->init(input_args->event_fd, input_args->synch_sem);
  return NULL;
}

void SfRun::preinit(Convolver *_sfConv, Delay *_sfDelay, vector<SFLOGIC*> _sflogic)
{
  int nc[2], cpos[2];
  int n, i, j, cbufsize, physch;
  int IO;

  /* FIXME: check so that unused pipe file descriptors are closed */
      
  sfConv = _sfConv;
  sfDelay = _sfDelay;
  sflogic = _sflogic;
  
  sfDai->Dai_init();  
    
  /* allocate shared memory for I/O buffers and interprocess communication */
  cbufsize = sfConv->convolver_cbufsize();
  if ((input_freqcbuf_base = shmalloc(sfconf->n_channels[IN] * cbufsize)) == NULL || (output_freqcbuf_base = shmalloc(sfconf->n_channels[OUT] * cbufsize)) == NULL)  {
    fprintf(stderr, "Failed to allocate shared memory: %s.\n",strerror(errno));
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
    return;
  }
  input_freqcbuf = (void**)shmalloc(sfconf->n_channels[IN]*sizeof(void*));
  for (n = 0; n < sfconf->n_channels[IN]; n++) {
    input_freqcbuf[n] = input_freqcbuf_base;
    input_freqcbuf_base = (uint8_t *)input_freqcbuf_base + cbufsize;
  }
  output_freqcbuf= (void**)shmalloc(sfconf->n_channels[OUT]*sizeof(void*));
  for (n = 0; n < sfconf->n_channels[OUT]; n++) {
    output_freqcbuf[n] = output_freqcbuf_base;
    output_freqcbuf_base = (uint8_t *)output_freqcbuf_base + cbufsize;
  }
  
  /* initialise process intercomm area */
  for (n = 0; (uint32_t)n < sizeof(icomm->debug); n++) {
    ((volatile uint8_t *)&icomm->debug)[n] = ~0;
  }
  for (n = 0; n < sfconf->n_filters; n++) {
    icomm->fctrl.push_back(sfconf->initfctrl[n]);
  }
  FOR_IN_AND_OUT {
    for (n = 0; n < sfconf->n_channels[IO]; n++) {
      icomm->delay[IO].push_back(sfconf->delay[IO][n]);
      if (sfconf->mute[IO][n]) {
	bit_set_volatile(icomm->ismuted[IO], n);
      } else {
	bit_clr_volatile(icomm->ismuted[IO], n);
      }
    }
  }
  if( sem_init(&bl_output_2_bl_input_sem, 1, 0) == -1 ||
      sem_init(&bl_output_2_cb_input_sem, 1, 0) == -1 ||
      sem_init(&cb_output_2_bl_input_sem, 1, 0) == -1 ||
      sem_init(&bl_input_2_filter_sem, 1, 0) == -1 ||
      sem_init(&filter_2_bl_output_sem, 1, 0) == -1 ||
      sem_init(&cb_input_2_filter_sem, 1, 0) == -1 ||
      sem_init(&filter_2_cb_output_sem, 1, 0) == -1 ||
      sem_init(&mutex_sem, 1, 0) == -1) 
    {
      fprintf(stderr, "Failed sem_init: %s.\n", strerror(errno));
      if (sfconf != NULL && sfconf->debug) {
	print_debug();
      }
      sfstop();
      return;
    }
  if (sem_post(&mutex_sem)== -1) {
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }        
    sfstop();
    return;
  }
  /* init digital audio interfaces for input and output */
  if (sfDai->dai_buffer_format[IN]->n_samples != sfconf->filter_length || sfDai->dai_buffer_format[OUT]->n_samples != sfconf->filter_length) {
    /* a bug if it happens */
    fprintf(stderr, "Fragment size mismatch.\n");
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
    return;
  }
  
  /* init overflow structure */
  reset_overflow_out = new struct sfoverflow [sfconf->n_channels[OUT]];
  memset(reset_overflow_out, 0, sizeof(struct sfoverflow) * sfconf->n_channels[OUT]);
  for (n = 0; n < sfconf->n_channels[OUT]; n++) {
    physch = sfconf->virt2phys[OUT][n];
    if (sfDai->dai_buffer_format[OUT]->bf[physch].sf.isfloat) {
      reset_overflow_out[n].max = 1.0;
    } else {
      reset_overflow_out[n].max = (double)(1 << ((sfDai->dai_buffer_format[OUT]->bf[physch].sf.sbytes << 3) - 1)) - 1;
    }
    icomm->out_overflow.push_back(reset_overflow_out[n]);
  }
  
  reset_overflow_in = new struct sfoverflow [sfconf->n_channels[IN]];
  memset(reset_overflow_in, 0, sizeof(struct sfoverflow) * sfconf->n_channels[IN]);
  for (n = 0; n < sfconf->n_channels[IN]; n++) {
    physch = sfconf->virt2phys[IN][n];
    if (sfDai->dai_buffer_format[IN]->bf[physch].sf.isfloat) {
      reset_overflow_in[n].max = 1.0;
    } else {
      reset_overflow_in[n].max = (double)(1 << ((sfDai->dai_buffer_format[IN]->bf[physch].sf.sbytes << 3) - 1)) - 1;
    }
    icomm->in_overflow.push_back(reset_overflow_in[n]);
  }

  /* initialise event listener structure */
    
  timestamp(&icomm->debug.ts_start);

  FOR_IN_AND_OUT {
    nc[IO] = sfconf->n_channels[IO];
    j = 0;
    while (j < nc[IO] && cpos[IO] < sfconf->n_physical_channels[IO]) {
      for (i = 0; i < sfconf->n_virtperphys[IO][cpos[IO]]; i++, j++) {
	channels[IO][j] = sfconf->phys2virt[IO][cpos[IO]][i];
      }		
      cpos[IO]++;
    }
    nc[IO] = j;
  }   
  n_procinputs = nc[IN];
  n_procoutputs = nc[OUT];
}

void SfRun::sfrun(Convolver *_sfConv, Delay *_sfDelay, vector<SFLOGIC*> _sflogic)
{
  int nc[2], cpos[2];
  int n, i, j, cbufsize, physch;
  struct module_init_input module_init_args;
  int IO;

  /* FIXME: check so that unused pipe file descriptors are closed */
      
  sfConv = _sfConv;
  sfDelay = sfDelay;
  sflogic = _sflogic;

  isRunning = true;

  sfDai->Dai_init();  
    
  /* allocate shared memory for I/O buffers and interprocess communication */
  cbufsize = sfConv->convolver_cbufsize();
  if ((input_freqcbuf_base = shmalloc(sfconf->n_channels[IN] * cbufsize)) == NULL || (output_freqcbuf_base = shmalloc(sfconf->n_channels[OUT] * cbufsize)) == NULL)  {
    fprintf(stderr, "Failed to allocate shared memory: %s.\n",strerror(errno));
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
    return;
  }
  input_freqcbuf = (void**)shmalloc(sfconf->n_channels[IN]*sizeof(void*));
  for (n = 0; n < sfconf->n_channels[IN]; n++) {
    input_freqcbuf[n] = input_freqcbuf_base;
    input_freqcbuf_base = (uint8_t *)input_freqcbuf_base + cbufsize;
  }
  output_freqcbuf= (void**)shmalloc(sfconf->n_channels[OUT]*sizeof(void*));
  for (n = 0; n < sfconf->n_channels[OUT]; n++) {
    output_freqcbuf[n] = output_freqcbuf_base;
    output_freqcbuf_base = (uint8_t *)output_freqcbuf_base + cbufsize;
  }
  
  /* initialise process intercomm area */
  for (n = 0; (uint32_t)n < sizeof(struct intercomm_area); n++) {
    ((volatile uint8_t *)icomm)[n] = 0;
  }
  for (n = 0; (uint32_t)n < sizeof(icomm->debug); n++) {
    ((volatile uint8_t *)&icomm->debug)[n] = ~0;
  }
  for (n = 0; n < sfconf->n_filters; n++) {
    icomm->fctrl.push_back(sfconf->initfctrl[n]);
  }
  FOR_IN_AND_OUT {
    icomm->ismuted[IO] = (uint32_t*)emalloc((sfconf->n_channels[IO]/32+1)*sizeof(int));
    for (n = 0; n < sfconf->n_channels[IO]; n++) {
      icomm->delay[IO].push_back(sfconf->delay[IO][n]);
      if (sfconf->mute[IO][n]) {
	bit_set_volatile(icomm->ismuted[IO], n);
      } else {
	bit_clr_volatile(icomm->ismuted[IO], n);
      }
    }
  }
  if( sem_init(&bl_output_2_bl_input_sem, 1, 0) == -1 ||
      sem_init(&bl_output_2_cb_input_sem, 1, 0) == -1 ||
      sem_init(&cb_output_2_bl_input_sem, 1, 0) == -1 ||
      sem_init(&bl_input_2_filter_sem, 1, 0) == -1 ||
      sem_init(&filter_2_bl_output_sem, 1, 0) == -1 ||
      sem_init(&cb_input_2_filter_sem, 1, 0) == -1 ||
      sem_init(&filter_2_cb_output_sem, 1, 0) == -1 ||
      sem_init(&mutex_sem, 1, 0) == -1) 
    {
      fprintf(stderr, "Failed sem_init: %s.\n", strerror(errno));
      if (sfconf != NULL && sfconf->debug) {
	print_debug();
      }
      sfstop();
      return;
    }
  if (sem_post(&mutex_sem)== -1) {
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }        
    sfstop();
    return;
  }
  /* init digital audio interfaces for input and output */
  if (sfDai->dai_buffer_format[IN]->n_samples != sfconf->filter_length || sfDai->dai_buffer_format[OUT]->n_samples != sfconf->filter_length) {
    /* a bug if it happens */
    fprintf(stderr, "Fragment size mismatch.\n");
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
    return;
  }
  
  /* init overflow structure */
  reset_overflow_out = new struct sfoverflow [sfconf->n_channels[OUT]];
  memset(reset_overflow_out, 0, sizeof(struct sfoverflow) * sfconf->n_channels[OUT]);
  for (n = 0; n < sfconf->n_channels[OUT]; n++) {
    physch = sfconf->virt2phys[OUT][n];
    if (sfDai->dai_buffer_format[OUT]->bf[physch].sf.isfloat) {
      reset_overflow_out[n].max = 1.0;
    } else {
      reset_overflow_out[n].max = (double)(1 << ((sfDai->dai_buffer_format[OUT]->bf[physch].sf.sbytes << 3) - 1)) - 1;
    }
    icomm->out_overflow.push_back(reset_overflow_out[n]);
  }  

  reset_overflow_in = new struct sfoverflow [sfconf->n_channels[IN]];
  memset(reset_overflow_in, 0, sizeof(struct sfoverflow) * sfconf->n_channels[IN]);
  for (n = 0; n < sfconf->n_channels[IN]; n++) {
    physch = sfconf->virt2phys[IN][n];
    if (sfDai->dai_buffer_format[IN]->bf[physch].sf.isfloat) {
      reset_overflow_in[n].max = 1.0;
    } else {
      reset_overflow_in[n].max = (double)(1 << ((sfDai->dai_buffer_format[IN]->bf[physch].sf.sbytes << 3) - 1)) - 1;
    }
    icomm->in_overflow.push_back(reset_overflow_in[n]);
  }

  /* initialise event listener structure */
    
  timestamp(&icomm->debug.ts_start);
  
  /* create filter processes */
  cpos[IN] = cpos[OUT] = 0;
    
  /* calculate how many (and which) inputs/outputs the process should do FFTs for */
  FOR_IN_AND_OUT {
    nc[IO] = sfconf->n_channels[IO];
    j = 0;
    while (j < nc[IO] && cpos[IO] < sfconf->n_physical_channels[IO]) {
      for (i = 0; i < sfconf->n_virtperphys[IO][cpos[IO]]; i++, j++) {
	channels[IO][j] = sfconf->phys2virt[IO][cpos[IO]][i];
      }		
      cpos[IO]++;
    }
    nc[IO] = j;
  }     
  n_procinputs = nc[IN];
  n_procoutputs = nc[OUT];
  pthread_create(&sfrun_thread, NULL,&SfRun::filter_process_thread, this);
  if (sem_post(&cb_input_2_filter_sem) == -1) {
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
  } 
  module_thread = (pthread_t*)emalloc(sfconf->n_logicmods*sizeof(pthread_t));
  synch_module_sem = (sem_t *)emalloc(sfconf->n_logicmods*sizeof(sem_t));
  for (n = 0; n < sfconf->n_logicmods; n++) {
    if (sem_init(&synch_module_sem[n], 1, 0) == -1) {
      if (sfconf != NULL && sfconf->debug) {
	print_debug();
      }
      sfstop();
      return;
    }
    if (sflogic[n]->fork_mode != SF_FORK_DONT_FORK) {
      module_init_args.sflogic = sflogic[n];
      module_init_args.n = n;
      module_init_args.event_fd = 0;
      module_init_args.synch_sem = &synch_module_sem[n];
      if(pthread_create(&module_thread[n], NULL, module_init_thread, &module_init_args)!=0) {
	fprintf(stderr, "Init module thread failed: %s.\n", strerror(errno));
	if (sfconf != NULL && sfconf->debug) {
	  print_debug();
	}
	sfstop();
	return;
      }	
      if (sem_wait(&synch_module_sem[n]) == -1) {
	if (sfconf != NULL && sfconf->debug) {
	  print_debug();
	}
	sfstop();
	return;
      }
    } else {
      sflogic[n]->init(0, &synch_module_sem[n]);
    }
  }    
  if (sem_post(&bl_input_2_filter_sem)==-1 || sem_wait(&filter_2_bl_output_sem)==-1) {
    fprintf(stderr, "Error: ran probably out of memory, aborting.\n"); 
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
    return;
  }
  // no blocking I/O: finish startup, start callback I/O and exit 
  pinfo("Audio processing starts now \n");
  sfDai->trigger_callback_io();
  return;
}

/* void SfRun::post_start(void)
{
  sem_t sem_n, sem_i, sem_j;

  sem_wait(&filter_2_bl_output_sem);
  if (n_blocking_devs[OUT] == 0) {
    sem_wait(&filter_2_bl_output_sem);
    }
  // start the input process (this code is reached only if necessary) 
  //if( sem_post(&bl_input_2_filter_sem)==-1 || (n_blocking_devs[OUT] == 0 && sem_wait(&filter_2_bl_output_sem))) {
  if( sem_post(&bl_input_2_filter_sem)==-1 || sem_wait(&filter_2_bl_output_sem)) {
    fprintf(stderr, "Error: ran probably out of memory, aborting.\n");
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
    return;
  }
  //if (sem_post(&bl_input_2_filter_sem)==-1 || n_blocking_devs[OUT] == 0) {
  if (sem_post(&bl_input_2_filter_sem)==-1) {
    fprintf(stderr, "Error: ran probably out of memory, aborting.\n");
    if (sfconf != NULL && sfconf->debug) {
      print_debug();
    }
    sfstop();
    return;
  }
  sem_n = cb_output_2_bl_input_sem;
  sem_init(&sem_i, 1, -1);
  sem_init(&sem_j, 1, 0);
  if (n_callback_devs[OUT] > 0 && n_blocking_devs[OUT] > 0) {
    sem_init(&bl_output_2_bl_input_sem, 1, 0);
    sem_n = bl_output_2_bl_input_sem;
    sem_init(&cb_output_2_bl_input_sem, 1, 0);
    sem_i = cb_output_2_bl_input_sem;
  } else if (n_callback_devs[OUT] > 0) {
    sem_n = cb_output_2_bl_input_sem;
    sem_init(&sem_i, 1, -1);
  } else {
    sem_init(&bl_output_2_bl_input_sem, 1, 0);
    sem_n = bl_output_2_bl_input_sem;
    sem_init(&sem_i, 1, -1);
  }
  if (n_blocking_devs[OUT] > 0) {
    sem_init(&synch_sem, 1, 0);
    sem_j = synch_sem;
  } else {
    sem_init(&sem_j, 1, 0);
  }
  input_process(sfDai->buffers[IN], bl_input_2_filter_sem, sem_n, sem_i, sem_j);
  // never reached    
}*/

double SfRun::realtime_index(void)
{
  return icomm->realtime_index;
}

void SfRun::reset_peak(void)
{
  int n;
  
  icomm->doreset_overflow = true;
  for (n = 0; n <  sfconf->n_channels[OUT]; n++) {
    icomm->out_overflow[n] = reset_overflow_out[n];
  }

  for (n = 0; n <  sfconf->n_channels[IN]; n++) {
    icomm->in_overflow[n] = reset_overflow_in[n];
  }
}

vector<string> SfRun::sflogic_names(int *n_names)
{
  if (n_names != NULL) {
    *n_names = sfconf->n_logicmods;
  }
  return sfconf->logicnames;
}

/*int SfRun::sflogic_command(int modindex,
			   const char params[],
			   char **message)
{
  static char *msgstr = NULL;
  int ans;
  
  efree(msgstr);
  msgstr = NULL;
  if (modindex < 0 || modindex >= sfconf->n_logicmods) {
    if (message != NULL) {
      msgstr = estrdup("Invalid module index");
      *message = msgstr;
    }
    return -1;
  }
  if (params == NULL) {
    if (message != NULL) {
      msgstr = estrdup("Missing parameters");
      *message = msgstr;
    }
    return -1;
  }
  ans = sflogic[modindex]->command(params);
  msgstr = estrdup(sflogic[modindex]->message());
  *message = msgstr;
  return ans;
  }*/

void SfRun::sfio_range(int io, int modindex, int range[2])
{
  if (range != NULL) {
    range[0] = 0;
    range[1] = 0;
  }
  if ((io != IN && io != OUT) || modindex < 0 || modindex >= 1 || range == NULL) {
    return;
  }
  range[0] = sfconf->subdevs[io].channels.channel_name[0];
  range[1] = sfconf->subdevs[io].channels.channel_name[sfconf->subdevs[io].channels.open_channels - 1];
}

void SfRun::sf_make_realtime(pid_t pid,
			     int priority,
			     const char name[])
{
  struct sched_param schp;
  
  if (icomm->ignore_rtprio) {        
    return;
  }
  
  memset(&schp, 0, sizeof(schp));
  schp.sched_priority = priority;
  
  if (sched_setscheduler(pid, SCHED_FIFO, &schp) != 0) {
    if (errno == EPERM) {
      pinfo("Warning: not allowed to set realtime priority. Will run with default priority\n  instead, which is less reliable (underflow may occur).\n");
      icomm->ignore_rtprio = true;
      return;
    } else {
      if (name != NULL) {
	fprintf(stderr, "Could not set realtime priority for %s process: %s.\n", name, strerror(errno));
      } else {
	fprintf(stderr, "Could not set realtime priority: %s.\n", strerror(errno));
      }
      if (sfconf != NULL && sfconf->debug) {
	print_debug();
      }
      sfstop();
    }
  }
  
  if (sfconf->lock_memory) {
#ifdef __OS_FREEBSD__
    pinfo("Warning: lock_memory not supported on this platform.\n");
#else        
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
      if (name != NULL) {
	fprintf(stderr, "Could not lock memory for %s process: %s.\n", name, strerror(errno));
      } else {
	fprintf(stderr, "Could not lock memory: %s.\n", strerror(errno));
      }
      if (sfconf != NULL && sfconf->debug) {
	print_debug();
      }
      sfstop();
    }
#endif        
  }
  if (name != NULL) {
    pinfo("Realtime priority %d set for %s process (pid %d)\n",priority, name, (int)getpid());
  }
}

#define real_t float
#define REALSIZE 4
#define RAW2REAL_NAME raw2realf
#define MIXNSCALE_NAME mixnscalef
#define CONVOLVE_INPLACE_NAME convolve_inplacef
#define CONVOLVE_NAME convolvef
#define CONVOLVE_ADD_NAME convolve_addf
#define DIRAC_CONVOLVE_INPLACE_NAME dirac_convolve_inplacef
#define DIRAC_CONVOLVE_NAME dirac_convolvef
#include "raw2real.h"

#undef real_t
#undef REALSIZE
#undef RAW2REAL_NAME
#undef MIXNSCALE_NAME
#undef CONVOLVE_INPLACE_NAME
#undef CONVOLVE_NAME
#undef CONVOLVE_ADD_NAME
#undef DIRAC_CONVOLVE_INPLACE_NAME
#undef DIRAC_CONVOLVE_NAME

#define real_t double
#define REALSIZE 8
#define RAW2REAL_NAME raw2reald
#define MIXNSCALE_NAME mixnscaled
#define CONVOLVE_INPLACE_NAME convolve_inplaced
#define CONVOLVE_NAME convolved
#define CONVOLVE_ADD_NAME convolve_addd
#define DIRAC_CONVOLVE_INPLACE_NAME dirac_convolve_inplaced
#define DIRAC_CONVOLVE_NAME dirac_convolved
#include "raw2real.h"

#undef real_t
#undef REALSIZE
#undef RAW2REAL_NAME
#undef MIXNSCALE_NAME
#undef CONVOLVE_INPLACE_NAME
#undef CONVOLVE_NAME
#undef CONVOLVE_ADD_NAME
#undef DIRAC_CONVOLVE_INPLACE_NAME
#undef DIRAC_CONVOLVE_NAME

#define real_t float
#define REALSIZE 4
#define REAL2RAW_NAME real2rawf_hp_tpdf
#define REAL2INT_CALL ditherf_real2int_hp_tpdf(((float *)realbuf)[n], rmin,    \
                                               rmax, imin, imax, overflow,     \
                                               dither_state, n)
#define REAL2RAW_EXTRA_PARAMS , struct dither_state *dither_state
#include "real2raw.h"
#undef REAL2RAW_NAME
#undef REAL2INT_CALL
#undef REAL2RAW_EXTRA_PARAMS

#define REAL2RAW_NAME real2rawf_no_dither
#define REAL2INT_CALL ditherd_real2int_no_dither(((float *)realbuf)[n], rmin,  \
                                                 rmax, imin, imax, overflow)
#define REAL2RAW_EXTRA_PARAMS 
#include "real2raw.h"
#undef REAL2RAW_NAME
#undef REAL2INT_CALL
#undef REAL2RAW_EXTRA_PARAMS
#undef REALSIZE
#undef real_t

#define real_t double
#define REALSIZE 8
#define REAL2RAW_NAME real2rawd_hp_tpdf
#define REAL2INT_CALL ditherd_real2int_hp_tpdf(((double *)realbuf)[n], rmin,   \
                                               rmax, imin, imax, overflow,     \
                                               dither_state, n)
#define REAL2RAW_EXTRA_PARAMS , struct dither_state *dither_state
#include "real2raw.h"
#undef REAL2RAW_NAME
#undef REAL2INT_CALL
#undef REAL2RAW_EXTRA_PARAMS

#define REAL2RAW_NAME real2rawd_no_dither
#define REAL2INT_CALL ditherd_real2int_no_dither(((double *)realbuf)[n], rmin, \
                                                 rmax, imin, imax, overflow)
#define REAL2RAW_EXTRA_PARAMS 
#include "real2raw.h"
#undef REAL2RAW_NAME
#undef REAL2INT_CALL
#undef REAL2RAW_EXTRA_PARAMS
#undef REALSIZE
#undef real_t

void SfRun::convolver_raw2cbuf(void *rawbuf,
			       void *cbuf,
			       void *next_cbuf,
			       struct buffer_format *sf,
			       struct sfoverflow *overflow)
{ 
  if (sfconf->realsize == 4) {
    raw2realf(next_cbuf, (void *)&((uint8_t *)rawbuf)[sf->byte_offset],
	      sf->sf.sbytes << 3, sf->sf.bytes, (sf->sf.bytes - sf->sf.sbytes) << 3,
	      sf->sf.isfloat, sf->sample_spacing, sf->sf.swap, sfconf->filter_length, overflow);
  } else {
    raw2reald(next_cbuf, (void *)&((uint8_t *)rawbuf)[sf->byte_offset],
	      sf->sf.sbytes << 3, sf->sf.bytes, (sf->sf.bytes - sf->sf.sbytes) << 3,
	      sf->sf.isfloat, sf->sample_spacing, sf->sf.swap, sfconf->filter_length, overflow);
  }
  memcpy(&((uint8_t *)cbuf)[sfconf->filter_length * sfconf->realsize], next_cbuf, sfconf->filter_length * sfconf->realsize);
}

void SfRun::convolver_cbuf2raw(void *cbuf,
			       void *outbuf,
			       struct buffer_format *sf,
			       bool apply_dither,
			       void *dither_state,
			       struct sfoverflow *overflow)
{
  int n_fft2;
  
  n_fft2 = sfconf->filter_length;

  if (sfconf->realsize == 4) {
    if (apply_dither && !sf->sf.isfloat) {
      dither_preloop_real2int_hp_tpdf((struct dither_state*)dither_state, n_fft2);
      real2rawf_hp_tpdf((void *)&((uint8_t *)outbuf)[sf->byte_offset], 
			cbuf, sf->sf.sbytes << 3, sf->sf.bytes,
			(sf->sf.bytes - sf->sf.sbytes) << 3,
			sf->sf.isfloat, sf->sample_spacing,
			sf->sf.swap, n_fft2, overflow, (struct dither_state*)dither_state);
    } else {
      real2rawf_no_dither((void *)&((uint8_t *)outbuf)[sf->byte_offset],
			  cbuf, sf->sf.sbytes << 3, sf->sf.bytes,
			  (sf->sf.bytes - sf->sf.sbytes) << 3,
			  sf->sf.isfloat, sf->sample_spacing,
			  sf->sf.swap, n_fft2, overflow);
    }
  } else {
    if (apply_dither && !sf->sf.isfloat) {
      dither_preloop_real2int_hp_tpdf((struct dither_state*)dither_state, n_fft2);
      real2rawd_hp_tpdf((void *)&((uint8_t *)outbuf)[sf->byte_offset],
			cbuf, sf->sf.sbytes << 3, sf->sf.bytes,
			(sf->sf.bytes - sf->sf.sbytes) << 3,
			sf->sf.isfloat, sf->sample_spacing,
			sf->sf.swap, n_fft2, overflow, (struct dither_state*)dither_state);
    } else {
      real2rawd_no_dither((void *)&((uint8_t *)outbuf)[sf->byte_offset],
			  cbuf, sf->sf.sbytes << 3,
			  sf->sf.bytes,
			  (sf->sf.bytes - sf->sf.sbytes) << 3,
			  sf->sf.isfloat, sf->sample_spacing,
			  sf->sf.swap, n_fft2, overflow);
    }
  }
}

void SfRun::sfstop(void)
{   
  isRunning = false;
  sfDai->die();
}
