/*
 * (c) Copyright 2012/2022 -- Raul Fernandez
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */


#include "sfdai.hpp"

void Dai::Dai_exit(void)
{
  fprintf(stderr, "Failed to initialise digital audio interfaces.\n");
}

void Dai::trigger_callback_ready(int io)
{
  if (callback_ready_waiting[io] > 0) {
    sem_post(&cbreadywait_sem[io]);
    callback_ready_waiting[io] = 0;
  }
  cbmutex(io, false);    
}

void Dai::wait_callback_ready(int io)
{
  callback_ready_waiting[io]++;
  cbmutex(io, false);
  sem_wait(&cbreadywait_sem[io]);
}

void Dai::cbmutex(int io, bool lock)
{
  if (lock) {
    sem_wait(&cbmutex_pipe[io]);
  } else {
    sem_post(&cbmutex_pipe[io]);
  }
}

bool Dai::output_finish(void)
{ 
  cbmutex(OUT, true);
  if (dev[OUT]->finished) {
    pinfo("\nFinished!\n");
    return true;
  }
  cbmutex(OUT, false);
  return false;
}

void Dai::update_devmap(int io)
{
  int n;
  
  if (dev[io]->fd >= 0) {
    FD_SET(dev[io]->fd, &dev_fds[io]);
    if (dev[io]->fd > dev_fdn[io]) {
      dev_fdn[io] = dev[io]->fd;
    }
    fd2dev[io][dev[io]->fd] = dev[io];
  }
  for (n = 0; n < dev[io]->channels.used_channels; n++) {
    ch2dev[io][dev[io]->channels.channel_name[n]] = dev[io];
  }
}

/* if noninterleaved, update channel layout to fit the noninterleaved access
   mode (it is setup for interleaved layout per default). */

void Dai::noninterleave_modify(int io)
{
  int n;
  
  if (!dev[io]->isinterleaved) {
    dev[io]->channels.open_channels = dev[io]->channels.used_channels;
    for (n = 0; n < dev[io]->channels.used_channels; n++) {
      dev[io]->channels.channel_selection[n] = n;
    }
  }
}

void Dai::update_delay(struct subdev *sd, int io, uint8_t *buf)
{
  struct buffer_format *bf;
  int n, newdelay, virtch;

  if (sd->db == NULL) {
    return;
  }
  for (n = 0; n < sd->channels.used_channels; n++) {
    if (sd->db[n] == NULL) {
      continue;
    }
    bf = &dai_buffer_format[io]->bf[sd->channels.channel_name[n]];
    virtch = sfconf->phys2virt[io][sd->channels.channel_name[n]][0];
    newdelay = ca.delay[io][sd->channels.channel_name[n]];
    /*if (sfconf->use_subdelay[io] && sfconf->subdelay[io][virtch] == SF_UNDEFINED_SUBDELAY) {
      newdelay += sfconf->sdf_length;
      }*/
    sfDelay->delay_update(sd->db[n], (void *)&buf[bf->byte_offset], bf->sf.bytes, bf->sample_spacing, newdelay, NULL);
  }
}

void Dai::allocate_delay_buffers(int io, struct subdev *sd)
{
  int n, virtch;

  sd->db = (delaybuffer_t**) emalloc(sd->channels.used_channels * sizeof(delaybuffer_t *));
  for (n = 0; n < sd->channels.used_channels; n++) {
    /* check if we need a delay buffer here, that is if at least one
       channel has a direct virtual to physical mapping */
    if (sfconf->n_virtperphys[io][sd->channels.channel_name[n]] == 1) {
      virtch = sfconf->phys2virt[io][sd->channels.channel_name[n]][0];
      sd->db[n] = sfDelay->delay_allocate_buffer(period_size, sfconf->delay[io][virtch], sfconf->maxdelay[io][virtch], sd->channels.sf.bytes);
    } else {
      /* this delay is taken care of previous to feeding the channel
	 output to this module */
      sd->db[n] = NULL;
    }
  }
}

void Dai::do_mute(struct subdev *sd,
		  int io,
		  int wsize,
		  void *_buf,
		  int offset)
{
  int n, i, k, n_mute = 0, ch[sd->channels.used_channels], framesize;
  int bsch[sd->channels.used_channels], mid_offset;
  uint8_t *endp, *p, *startp;
  numunion_t *buf;
  uint16_t *p16;
  uint32_t *p32;
  uint64_t *p64;
  
  buf = (numunion_t *)_buf;
  /* Calculate which channels that should be cleared */
  for (n = 0; n < sd->channels.used_channels; n++) {
    if (ca.is_muted[io][sd->channels.channel_name[n]]) {
      ch[n_mute] = sd->channels.channel_selection[n];
      bsch[n_mute] = ch[n_mute] * sd->channels.sf.bytes;
      n_mute++;
    }
  }
  if (n_mute == 0) {
    return;
  }
  
  if (!sd->isinterleaved) {
    offset /= sd->channels.open_channels;
    wsize /= sd->channels.open_channels;
    p = &buf->u8[offset];
    i = sfconf->filter_length * sd->channels.sf.bytes;
    for (n = 0; n < n_mute; n++) {
      memset(p + ch[n] * i, 0, wsize);
    }
    return;
  }
  
  startp = &buf->u8[offset];
  endp = &buf->u8[offset] + wsize;
  framesize = sd->channels.open_channels * sd->channels.sf.bytes;
  mid_offset = offset;
  if ((i = offset % framesize) != 0) {
    for (k = 0; k < n_mute && bsch[k] + sd->channels.sf.bytes <= i; k++);
    for (n = i, p = startp; p < startp + framesize - i && p < endp; p++, n++) {
      if (n >= bsch[k] && n < bsch[k] + sd->channels.sf.bytes) {
	*p = 0;
	if (n == bsch[k] + sd->channels.sf.bytes) {
	  if (++k == n_mute) {
	    break;
	  }
	}
      }
    }
    if (p == endp) {
      return;
    }
    mid_offset += framesize - i;
  }
  
  switch (sd->channels.sf.bytes) {
  case 1:
    for (p = &buf->u8[mid_offset]; p < endp; p += sd->channels.open_channels) {
      for (n = 0; n < n_mute; n++) {
	p[ch[n]] = 0;
      }
    }
    break;
  case 2:
    for (p16 = &buf->u16[mid_offset>>1]; (uint8_t *)p16 < endp; p16 += sd->channels.open_channels) {
      for (n = 0; n < n_mute; n++) {
	p16[ch[n]] = 0;
      }
    }
    break;
  case 3:
    for (p = &buf->u8[mid_offset]; p < endp; p += sd->channels.open_channels) {
      for (n = 0; n < n_mute; n++) {
	p[bsch[n]+0] = 0;
	p[bsch[n]+1] = 0;
	p[bsch[n]+2] = 0;
      }
    }
  case 4: {
    for (p32 = &buf->u32[mid_offset>>2]; (uint8_t *)p32 < endp; p32 += sd->channels.open_channels) {
      for (n = 0; n < n_mute; n++) {
	p32[ch[n]] = 0;
      }
    }
    break;
  }
  case 8: {
    for (p64 = &buf->u64[mid_offset>>3]; (uint8_t *)p64 < endp; p64 += sd->channels.open_channels) {
      for (n = 0; n < n_mute; n++) {
	p64[ch[n]] = 0;
      }
    }
    break;
  }
  default:
    fprintf(stderr, "Sample byte size %d not suppported.\n", sd->channels.sf.bytes);
    exit(SF_EXIT_OTHER);
    break;
  }
  
  if ((i = (offset + wsize) % framesize) != 0) {
    if ((p = endp - i) < startp) {
      return;
    }	
    for (n = k = 0; p < endp; p++, n++) {
      if (n >= bsch[k] && n < bsch[k] + sd->channels.sf.bytes) {
	*p = 0;
	if (n == bsch[k] + sd->channels.sf.bytes) {
	  if (++k == n_mute) {
	    break;
	  }
	}
      }
    }
  }
}

bool Dai::init_input(struct dai_subdevice *dai_subdev)
{
  int n;
  
  dev[IN]->channels = dai_subdev->channels;
  dev[IN]->index = 0;
  dev[IN]->cb.curbuf = 0;
  dev[IN]->cb.iodelay_fill = 0;
  dev[IN]->cb.frames_left = 0;
  sfconf->io->init(IN, dai_subdev->i_handle, dai_subdev->channels.used_channels, dai_subdev->channels.channel_selection, dev[IN]);
  dev[IN]->fd = -1;
  if (dev[IN]->block_size_frames == 0 || sfconf->filter_length % dev[IN]->block_size_frames != 0) {
    fprintf(stderr, "Invalid block size for callback input.\n");
    return false;
  }
  dev[IN]->isinterleaved = !!dev[IN]->isinterleaved;
  //dev[IN]->bad_alignment = false;
  noninterleave_modify(IN);
  dev[IN]->block_size = dev[IN]->block_size_frames * dev[IN]->channels.open_channels * dev[IN]->channels.sf.bytes;
  allocate_delay_buffers(IN, dev[IN]);
  update_devmap(IN);
  return true;
}

bool Dai::init_output(struct dai_subdevice *dai_subdev)
{
  int n;
  
  dev[OUT]->channels = dai_subdev->channels;
  dev[OUT]->index = 0;
  dev[OUT]->cb.curbuf = 0;
  dev[OUT]->cb.iodelay_fill = 0;
  dev[OUT]->cb.frames_left = 0;
  sfconf->io->init(OUT, dai_subdev->i_handle, dai_subdev->channels.used_channels, dai_subdev->channels.channel_selection, dev[OUT]);
  dev[OUT]->fd = -1;
  if (dev[OUT]->block_size_frames == 0 || sfconf->filter_length % dev[OUT]->block_size_frames != 0) {
    fprintf(stderr, "Invalid block size for callback output.\n");
    return false;
  }
  dev[OUT]->isinterleaved = !!dev[OUT]->isinterleaved;
  noninterleave_modify(OUT);
  dev[OUT]->block_size = dev[OUT]->block_size_frames * dev[OUT]->channels.open_channels * dev[OUT]->channels.sf.bytes;
  allocate_delay_buffers(OUT, dev[OUT]);
  update_devmap(OUT);
  return true;
}

void Dai::calc_buffer_format(int fragsize,
			     int io,
			     struct dai_buffer_format *format)
{
  int i, ch;
  
  format->n_samples = fragsize;
  format->n_channels = 0;
  format->n_bytes = 0;
  dev[io]->buf_offset = format->n_bytes;
  format->n_channels += dev[io]->channels.used_channels;
  for (i = 0; i < dev[io]->channels.used_channels; i++) {
    ch = dev[io]->channels.channel_name[i];
    format->bf[ch].sf = dev[io]->channels.sf;
    if (dev[io]->isinterleaved) {
      format->bf[ch].byte_offset = format->n_bytes + dev[io]->channels.channel_selection[i] * dev[io]->channels.sf.bytes;
      format->bf[ch].sample_spacing = dev[io]->channels.open_channels;
    } else {
      format->bf[ch].byte_offset = format->n_bytes;
      format->bf[ch].sample_spacing = 1;
      format->n_bytes += dev[io]->channels.sf.bytes * fragsize;
    }
  }
  dev[io]->buf_size = dev[io]->buf_left = dev[io]->channels.open_channels * dev[io]->channels.sf.bytes * fragsize;
  if (dev[io]->isinterleaved) {
    format->n_bytes += dev[io]->buf_size;
  }
  if (format->n_bytes % ALIGNMENT != 0) {
    format->n_bytes += ALIGNMENT - format->n_bytes % ALIGNMENT;
  }
}

bool Dai::callback_init(void)
{
  sem_init(&cbreadywait_sem[IN], 0, 0);
  sem_init(&cbreadywait_sem[OUT], 0, 0);
    
  /*if (dev[IN]->bad_alignment) {
    fprintf(stderr, "Error: currently no support for bad callback I/O block alignment.\n	\
  BruteFIR's partition length must be divisable with the sound server's\n \
  buffer size.\n");
    return false;
    }*/
  return true;
}

void Dai::callback_process(void)
{
  int sem_value;
  
  callback_init_msg = (char)callback_init();
  sem_post(&cbsem_r);
  if (callback_init_msg == 0) {
    while (true) sleep(1000);
  }
  sem_wait(&cbsem_s);
  callback_init_msg = CB_MSG_START;
  sem_post(&cbsem_r);
  if (callback_init_msg == 0) {
    while (true) sleep(1000);
  }
  while (true) {
    sem_getvalue(&cbsem_s,&sem_value);
    sem_wait(&cbsem_s);
    switch ((int)callback_init_msg) {
    case CB_MSG_START:
      if (sfconf->io->synch_start() != 0) {
	fprintf(stderr, "Failed to start I/O module, aborting.\n");
	exit(SF_EXIT_OTHER);
      }
      break;
    case CB_MSG_STOP:
      sfconf->io->synch_stop();
      callback_init_msg = 1;
      sem_post(&cbsem_r);
      break;
    default:
      fprintf(stderr, "Bug: invalid msg %d, aborting.\n", (int)callback_init_msg);
      exit(SF_EXIT_OTHER);
      break;
    }
  }
  while (true) sleep(1000);
}

Dai::Dai(struct sfconf *_sfconf,
	 intercomm_area *_icomm,
	 SfCallback  *_callbackClass) 
{ 
  sfRun = _callbackClass;
  sfconf = _sfconf;
  icomm = _icomm;
}

void Dai::Dai_init(Delay *_sfDelay)
{
  bool all_bad_alignment, none_clocked;
  int n, msec;
  int IO;

  sfDelay = _sfDelay;
   
  n_fd_devs[SF_IN] = 0;
  n_fd_devs[SF_OUT] = 0;
  memset(dev_fds, 0, sizeof(dev_fds));
  memset(&clocked_wfds, 0, sizeof(clocked_wfds));
  n_clocked_devs = 0;
  dev_fdn[SF_IN] = 0;
  dev_fdn[SF_OUT] = 0;
  min_block_size[SF_IN] = 0;
  min_block_size[SF_OUT] =0;
  cb_min_block_size[SF_IN] = 0;
  cb_min_block_size[SF_OUT] = 0;
  input_poll_mode = false;
  memset(fd2dev, 0, sizeof(fd2dev));
  memset(ch2dev, 0, sizeof(ch2dev));
  dai_buffer_format[0] = NULL;
  dai_buffer_format[1] = NULL;
  
  period_size = sfconf->filter_length;
  //sample_rate = sfconf->sampling_rate;
  monitor_rate_fd = -1;
  callback_ready_waiting[SF_IN] = 0;
  callback_ready_waiting[SF_OUT] = 0 ;
  
  input_isfirst = true; 
  input_startmeasure = true;
  input_buf_index = 0;
  input_frames = 0; 
  input_curbuf = 0;
  
  output_isfirst = true;
  output_islast = false;
  output_buf_index = 0;
  output_curbuf = 0;
  
  ca.frames_left = -1;
  ca.cb_frames_left = -1;
  FOR_IN_AND_OUT {
    dai_buffer_format[IO] = &ca.buffer_format[IO];
    for (n = 0; n < sfconf->n_physical_channels[IO]; n++) {
      if (sfconf->n_virtperphys[IO][n] == 1) {
	ca.delay[IO][n] = sfconf->delay[IO][sfconf->phys2virt[IO][n][0]];
	ca.is_muted[IO][n] = sfconf->mute[IO][sfconf->phys2virt[IO][n][0]];
      } else {
	ca.delay[IO][n] = 0;
	ca.is_muted[IO][n] = false;
      }
    }
    dev[IO] = ca.dev[IO];
  }
  FOR_IN_AND_OUT {
    sem_init(&cbmutex_pipe[IO], 0, 0);
    sem_init(&cbsem_s, 0, 0);
    sem_init(&cbsem_r, 0, 0);
  }
  /* initialise callback io, if any */
  pthread_create(&dai_thread, NULL, Dai::callback_process_thread, this);
  sem_wait(&cbsem_r);
  if (callback_init_msg == 0) {
    Dai_exit();
  }
  FOR_IN_AND_OUT {
    sem_init(&synchsem[IO], 0, 0);
    sem_init(&paramssem_s[IO], 0, 0);
    sem_init(&paramssem_r[IO], 0, 0);
    sem_post(&synchsem[IO]);
  }
  if (!init_input(&sfconf->subdevs[IN])) {
    Dai_exit();
  }    
  if (!init_output(&sfconf->subdevs[OUT])) {
    Dai_exit();
  }
  
  /* calculate buffer format, and allocate buffers */
  FOR_IN_AND_OUT {
    calc_buffer_format(sfconf->filter_length, IO, &ca.buffer_format[IO]);
  }
  if ((buffer = (uint8_t*)shmalloc_id(&ca.buffer_id, 2 * dai_buffer_format[IN]->n_bytes + 2 * dai_buffer_format[OUT]->n_bytes)) == NULL) {
    fprintf(stderr, "Failed to allocate shared memory.\n");
    return;
  }
  //buffer = new uint8_t [2 * dai_buffer_format[IN]->n_bytes + 2 * dai_buffer_format[OUT]->n_bytes];
  memset(buffer, 0, 2 * dai_buffer_format[IN]->n_bytes + 2 * dai_buffer_format[OUT]->n_bytes);
  FOR_IN_AND_OUT {
    iobuffers[IO][0] = buffer;
    buffer += dai_buffer_format[IO]->n_bytes;
    iobuffers[IO][1] = buffer;
    buffer += dai_buffer_format[IO]->n_bytes;
    buffers[IO][0] = iobuffers[IO][0];
    buffers[IO][1] = iobuffers[IO][1];
  }
  dev[OUT]->buf_left = 0;
  dev[OUT]->cb.frames_left = -1;
  dev[OUT]->cb.iodelay_fill = 2 * sfconf->filter_length / dev[OUT]->block_size_frames - 2;
  sem_post(&cbsem_s);
  sem_wait(&cbsem_r);
  if (callback_init_msg == 0) {
    Dai_exit();
  }
  /* decide if to use input poll mode */
  input_poll_mode = false;
  //all_bad_alignment = true;
  none_clocked = true;
  msec = (sfconf->filter_length * 1000) / sfconf->sampling_rate;
  return;
}

Dai::~Dai(void)
{
  die();
}

void Dai::trigger_callback_io(void)
{
  int sem_value;
  
  callback_init_msg = CB_MSG_START;
  sem_post(&cbsem_s);
  sem_getvalue(&cbsem_s,&sem_value);
}

int Dai::minblocksize(void)
{
  int size = 0;
  int IO;
  
  FOR_IN_AND_OUT {
    if (cb_min_block_size[IO] != 0 && (cb_min_block_size[IO] < size || size == 0)) {
      size = cb_min_block_size[IO];
    }
  }
  return size;
}

bool Dai::get_input_poll_mode(void)
{
  return input_poll_mode;
}

bool Dai::isinit(void)
{
  return (dai_buffer_format[IN] != NULL);
}

void Dai::toggle_mute(int io, int channel)
{
  if ((io != IN && io != OUT) || channel < 0 || channel >= SF_MAXCHANNELS) {
    return;
  }
  ca.is_muted[io][channel] = !ca.is_muted[io][channel];
}

int Dai::change_delay(int io,
		      int channel,
		      int delay)
{
  if (delay < 0 || channel < 0 || channel >= SF_MAXCHANNELS || (io != IN && io != OUT) || sfconf->n_virtperphys[io][channel] != 1) {
    return -1;
  }
  ca.delay[io][channel] = delay;	
  return 0;
}

/*
 * Shut down prematurely. Must be called by the input and output process
 * separately. For any other process it has no effect.
 */
void Dai::die(void)
{
  sfconf->io->synch_stop();
}

void* Dai::callback_process_thread(void *args)
{
  Dai *n_dai = reinterpret_cast<Dai*>(args);
  n_dai->callback_process();
  return NULL;
}

void Dai::process_callback_input(struct subdev *sd,
				 void *cbbufs[],
				 int frame_count)
{
  struct buffer_format *sf;
  uint8_t *buf, *copybuf;
  int n, cnt, count;
  float *wbuf;
  
  buf = (uint8_t *)iobuffers[IN][sd->cb.curbuf];
  for (n = 0; n < sd->channels.used_channels; n++) {
    wbuf = (float*)cbbufs[n];
  }
  count = frame_count * sd->channels.used_channels * sd->channels.sf.bytes;
  if (sd->isinterleaved) {
    memcpy(buf + sd->buf_offset + sd->buf_size - sd->buf_left, cbbufs[0], count);
  } else {
    sf = &dai_buffer_format[IN]->bf[sd->channels.channel_name[0]];
    cnt = count / sd->channels.used_channels;
    copybuf = buf + sd->buf_offset + (sd->buf_size - sd->buf_left) / sd->channels.used_channels;
    for (n = 0; n < sd->channels.used_channels; n++) {
      memcpy(copybuf, cbbufs[n], cnt);
      copybuf += sfconf->filter_length * sf->sf.sbytes;
    }
  }
  sd->buf_left -= count;
  if (sd->buf_left == 0) {
    sd->cb.curbuf = !sd->cb.curbuf;
    do_mute(sd, IN, sd->buf_size, (void *)(buf + sd->buf_offset), 0);
    update_delay(sd, IN, buf);
  }
};

void Dai::process_callback_output(struct subdev *sd,
				  void *cbbufs[],
				  int frame_count,
				  bool iodelay_fill)
{
  struct buffer_format *sf;
  uint8_t *buf, *copybuf;
  int n, cnt, count;
  float *wbuf;
  
  buf = (uint8_t *)iobuffers[OUT][sd->cb.curbuf];
  count = frame_count * sd->channels.used_channels * sd->channels.sf.bytes;
  
  if (iodelay_fill) {
    if (sd->isinterleaved) {
      memset(cbbufs[0], 0, count);
    } else {
      cnt = count / sd->channels.used_channels;
      for (n = 0; n < sd->channels.used_channels; n++) {
	memset(cbbufs[n], 0, cnt);
      }
    }
    return;
  }
  
  if (sd->buf_left == sd->buf_size) {
    update_delay(sd, OUT, buf);
  }
  do_mute(sd, OUT, count, (void *)(buf + sd->buf_offset), sd->buf_size - sd->buf_left);
  if (sd->isinterleaved) {
    memcpy(cbbufs[0], buf + sd->buf_offset + sd->buf_size - sd->buf_left, count);
  } else {
    sf = &dai_buffer_format[OUT]->bf[sd->channels.channel_name[0]];
    cnt = count / sd->channels.used_channels;
    copybuf = buf + sd->buf_offset + (sd->buf_size - sd->buf_left) / sd->channels.used_channels;
    for (n = 0; n < sd->channels.used_channels; n++) {
      memcpy(cbbufs[n], copybuf, cnt);
      wbuf = (float *)copybuf;
      copybuf += sfconf->filter_length * sf->sf.sbytes;
    }
  }
  
  sd->buf_left -= count;
  if (sd->buf_left == 0) {
    sd->cb.curbuf = !sd->cb.curbuf;
  }
};

int Dai::process_callback(vector<struct subdev*> states[2],
			  int state_count[2],
			  void **buffers[2],
			  int frame_count,
			  int event)
{
  bool finished, unlock_output;
  int n, i, buf_index;
  struct subdev *sd;
  
  switch (event) {
  case SF_CALLBACK_EVENT_LAST_INPUT:
    if (ca.cb_frames_left == -1 || frame_count < ca.cb_frames_left) {
      ca.cb_frames_left = frame_count;
    }
    ca.cb_lastbuf_index = ca.cb_buf_index[IN];
    return 0;
  case SF_CALLBACK_EVENT_FINISHED:
    for (n = 0; n < state_count[OUT]; n++) {
      sd = states[OUT][n];
      sd->finished = true;
    }
    cbmutex(IN, true);
    trigger_callback_ready(IN);
    cbmutex(OUT, true);
    trigger_callback_ready(OUT);
    if (output_finish()) {
      exit(SF_EXIT_OK);
    }
    return -1;
  case SF_CALLBACK_EVENT_ERROR:
    fprintf(stderr, "An error occured in a callback I/O module.\n");
    exit(SF_EXIT_OTHER);
    break;
  case SF_CALLBACK_EVENT_NORMAL:
    break;
  default:
    fprintf(stderr, "Invalid event: %d\n", event);
    exit(SF_EXIT_OTHER);
    break;
  }
  
  if (state_count == NULL || buffers == NULL || frame_count <= 0) {
    fprintf(stderr, "Invalid parameters: states %p; state_count %p; buffers: %p; frame_count: %d\n", states, state_count, buffers, frame_count);
    exit(SF_EXIT_OTHER);
  }
  if (state_count[IN] > 0) {
    for (n = 0, i = 0; n < state_count[IN]; n++) {
      sd = states[IN][n];
      if (frame_count != sd->block_size_frames) {
	fprintf(stderr, "Error: unexpected callback I/O block alignment (%d != %d)\n", frame_count, sd->block_size_frames);
	exit(SF_EXIT_OTHER);
      }
      process_callback_input(sd, &buffers[IN][n], frame_count);
      if (sd->isinterleaved) {
	i++;
      } else {
	i += sd->channels.used_channels;
      }
    }
    sd = states[IN][0];
    if (sd->buf_left == 0) {
      finished = true;
      sd = dev[IN];
      if (sd->buf_left != 0) {
	finished = false;
      }
      if (finished) {
	sd = dev[IN];
	sd->buf_left = sd->buf_size;
	sfRun->sf_callback_ready(IN);
	ca.cb_buf_index[IN]++;
	trigger_callback_ready(IN);
      } else {
	wait_callback_ready(IN);
      }
    } else {
      cbmutex(IN, false);
    }
  }
  
  if (state_count[OUT] > 0) {
    unlock_output = false; 
    sd = states[OUT][0]; 
    //sfRun->rti_and_overflow(); 
    if (sd->buf_left == 0 && sd->cb.iodelay_fill == 0) {
      finished = true;
      sd = dev[OUT];
      if (sd->buf_left != 0 || sd->cb.iodelay_fill != 0) {
	finished = false;
      }
      if (finished) {
	sd = dev[OUT];
	sd->buf_left = sd->buf_size;
	sfRun->sf_callback_ready(OUT);
	ca.cb_buf_index[OUT]++;
	trigger_callback_ready(OUT);
      } else {
	wait_callback_ready(OUT);
      }
    } else {
      unlock_output = true;
    }
    
    for (n = 0, i = 0; n < state_count[OUT]; n++) {
      sd = states[OUT][n];
      if (frame_count != sd->block_size_frames) {
	fprintf(stderr, "Error: unexpected callback I/O block alignment (%d != %d)\n", frame_count, sd->block_size_frames);
	exit(SF_EXIT_OTHER);
      }
      process_callback_output(sd, &buffers[OUT][i], frame_count, sd->cb.iodelay_fill != 0);
      if (sd->cb.iodelay_fill != 0) {
	sd->cb.iodelay_fill--;
      }
      if (sd->isinterleaved) {
	i++;
      } else {
	i += sd->channels.used_channels;
      }
    }
    
    if (unlock_output) {
      cbmutex(OUT, false);
    }
    
    /* last buffer? */
    n = ca.cb_buf_index[IN];
    i = ca.cb_buf_index[OUT];
    buf_index = n < i ? i : n;
    sd = states[OUT][0];
    if (sd->cb.frames_left == -1 && ((ca.frames_left != -1 && buf_index == ca.lastbuf_index + 1) ||
				     (ca.cb_frames_left != -1 && buf_index == ca.cb_lastbuf_index + 1))) {        
      n = ca.frames_left;
      i = ca.cb_frames_left;
      if (n == -1 || (n > i && i != -1)) {
	n = i;
      }
      sd->cb.frames_left = n;
    }    
    
    if (sd->cb.frames_left != -1) {
      if (sd->cb.frames_left > sd->block_size_frames) {
	sd->cb.frames_left -= sd->block_size_frames;
	return 0;
      }
      if (sd->cb.frames_left == 0) {
	return -1;
      }
      return sd->cb.frames_left;
    }  
  }
  return 0;
}
