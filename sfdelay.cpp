/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "sfdelay.hpp"

Delay::Delay(struct sfconf *_sfconf,
	     int step_count)
{
  void *filter;
  int n;

  sfconf = _sfconf;

  if (step_count < 2) {
    fprintf(stderr, "Invalid step_count %d.\n", step_count);
    return;
  }

  return;
}

void Delay::copy_to_delaybuf(void *dbuf,
			     void *buf,
			     int sample_size,
			     int sample_spacing,
			     int n_samples)
{
  int n, i;
  
  if (sample_spacing == 1) {
    MEMCPY(dbuf, buf, n_samples * sample_size);
    return;
  }
  
  switch (sample_size) {
  case 1:
    for (n = i = 0; n < n_samples; n++, i += sample_spacing) {
      ((uint8_t *)dbuf)[n] = ((uint8_t *)buf)[i];
    }	
    break;
  case 2:
    for (n = i = 0; n < n_samples; n++, i += sample_spacing) {
      ((uint16_t *)dbuf)[n] = ((uint16_t *)buf)[i];
    }	
    break;
  case 3:
    n_samples *= 3;
    sample_spacing = sample_spacing * 3 - 3;
    for (n = i = 0; n < n_samples; i += sample_spacing) {
      ((uint8_t *)dbuf)[n++] = ((uint8_t *)buf)[i++];
      ((uint8_t *)dbuf)[n++] = ((uint8_t *)buf)[i++];
      ((uint8_t *)dbuf)[n++] = ((uint8_t *)buf)[i++];
    }	
    break;
  case 4:
    for (n = i = 0; n < n_samples; n++, i += sample_spacing) {
      ((uint32_t *)dbuf)[n] = ((uint32_t *)buf)[i];
    }		
    break;
  case 8:
    for (n = i = 0; n < n_samples; n++, i += sample_spacing) {
      ((uint64_t *)dbuf)[n] = ((uint64_t *)buf)[i];
    }		
    break;
  default:
    fprintf(stderr, "Sample byte size %d not suppported.\n", sample_size);
    exit(SF_EXIT_OTHER);
    break;
  }
}

void Delay::copy_from_delaybuf(void *buf,
			       void *dbuf,		   
			       int sample_size,
			       int sample_spacing,
			       int n_samples)
{
  int n, i;
  
  if (sample_spacing == 1) {
    MEMCPY(buf, dbuf, n_samples * sample_size);
    return;
  }
  
  switch (sample_size) {
  case 1:
    for (n = i = 0; n < n_samples; n++, i += sample_spacing) {
      ((uint8_t *)buf)[i] = ((uint8_t *)dbuf)[n];
    }	
    break;
  case 2:
    for (n = i = 0; n < n_samples; n++, i += sample_spacing) {
      ((uint16_t *)buf)[i] = ((uint16_t *)dbuf)[n];
    }	
    break;
  case 3:
    n_samples *= 3;
    sample_spacing = sample_spacing * 3 - 3;
    for (n = i = 0; n < n_samples; i += sample_spacing) {
      ((uint8_t *)buf)[i++] = ((uint8_t *)dbuf)[n++];
      ((uint8_t *)buf)[i++] = ((uint8_t *)dbuf)[n++];
      ((uint8_t *)buf)[i++] = ((uint8_t *)dbuf)[n++];
    }	
    break;
  case 4:
    for (n = i = 0; n < n_samples; n++, i += sample_spacing) {
      ((uint32_t *)buf)[i] = ((uint32_t *)dbuf)[n];
    }		
    break;
  case 8:
    for (n = i = 0; n < n_samples; n++, i += sample_spacing) {
      ((uint64_t *)buf)[i] = ((uint64_t *)dbuf)[n];
    }		
    break;
  default:
    fprintf(stderr, "Sample byte size %d not suppported.\n", sample_size);
    exit(SF_EXIT_OTHER);
    break;
  }
}

void Delay::shift_samples(void *buf,
			  int sample_size,
			  int sample_spacing,
			  int n_samples,
			  int distance)
{
  int n, i;

  n = (n_samples - 1) * sample_spacing;
  i = (n_samples + distance - 1) * sample_spacing;
  switch (sample_size) {
  case 1:
    for (; n >= 0; n -= sample_spacing, i -= sample_spacing) {
      ((uint8_t *)buf)[i] = ((uint8_t *)buf)[n];
    }	
    break;
  case 2:
    for (; n >= 0; n -= sample_spacing, i -= sample_spacing) {
      ((uint16_t *)buf)[i] = ((uint16_t *)buf)[n];
    }	
    break;
  case 3:
    n *= 3;
    i *= 3;
    sample_spacing *= 3;
    for (; n >= 0; n -= sample_spacing, i -= sample_spacing) {
      ((uint8_t *)buf)[i++] = ((uint8_t *)buf)[n++];
      ((uint8_t *)buf)[i++] = ((uint8_t *)buf)[n++];
      ((uint8_t *)buf)[i++] = ((uint8_t *)buf)[n++];
    }	
    break;
  case 4:
    for (; n >= 0; n -= sample_spacing, i -= sample_spacing) {
      ((uint32_t *)buf)[i] = ((uint32_t *)buf)[n];
    }	
    break;
  case 8:
    for (; n >= 0; n -= sample_spacing, i -= sample_spacing) {
      ((uint64_t *)buf)[i] = ((uint64_t *)buf)[n];
    }	
    break;
  default:
    fprintf(stderr, "Sample byte size %d not suppported.\n", sample_size);
    exit(SF_EXIT_OTHER);
    break;
  }    
}

void Delay::update_delay_buffer(delaybuffer_t *db,
				int sample_size,
				int sample_spacing,
				uint8_t *buf)
{
  uint8_t *lastbuf;

  lastbuf = (uint8_t*)((db->curbuf == db->n_fbufs - 1) ? db->fbufs[0] : db->fbufs[db->curbuf + 1]);

  /* 1. copy buffer to current full-sized delay buffer */
  copy_to_delaybuf(db->fbufs[db->curbuf], buf, sample_size, sample_spacing, db->fragsize);
    
  if (db->n_rest != 0) {
    /* 2. copy from delay rest buffer to the start of buffer */
    copy_from_delaybuf(buf, db->rbuf, sample_size, sample_spacing, db->n_rest);
    
    /* 3. copy from end of last full-sized delay buffer to rest buffer */
    MEMCPY(db->rbuf, lastbuf + (db->fragsize - db->n_rest) * sample_size, db->n_rest * sample_size);
  }

  /* 4. copy from start of last full-sized buffer to end of buffer */
  copy_from_delaybuf(buf + db->n_rest * sample_size * sample_spacing, lastbuf, sample_size, sample_spacing, db->fragsize - db->n_rest);
    
  if (++db->curbuf == db->n_fbufs) {
    db->curbuf = 0;
  }
}

void Delay::update_delay_short_buffer(delaybuffer_t *db,
				      int sample_size,
				      int sample_spacing,
				      uint8_t *buf)
{
  copy_to_delaybuf(db->shortbuf[db->curbuf],  buf + (db->fragsize - db->n_rest) * sample_size * sample_spacing, sample_size, sample_spacing, db->n_rest);

  shift_samples(buf, sample_size, sample_spacing, db->fragsize - db->n_rest, db->n_rest);
    
  db->curbuf = !db->curbuf;
  copy_from_delaybuf(buf, db->shortbuf[db->curbuf], sample_size, sample_spacing, db->n_rest);
}

void Delay::change_delay(delaybuffer_t *db,
			 int sample_size,
			 int newdelay)
{
  int size, i;
  
  if (newdelay == db->curdelay || newdelay > db->maxdelay) {
    return;
  }
  if (newdelay <= db->fragsize) {
    db->n_rest = newdelay;
    size = newdelay * sample_size;
    if (db->curdelay > db->fragsize || db->curdelay < newdelay) {
      MEMSET(db->shortbuf[0], 0, size);
      MEMSET(db->shortbuf[1], 0, size);
    }
    db->n_fbufs = 0;
    db->curbuf = 0;
    db->curdelay = newdelay;
    return;
  }
  db->n_rest = newdelay % db->fragsize;
  db->n_fbufs = newdelay / db->fragsize + 1;
  size = db->fragsize * sample_size;
  if (db->curdelay < newdelay) {
    for (i = 0; i < db->n_fbufs; i++) {
      MEMSET(db->fbufs[i], 0, size);
    }
    if (db->n_rest != 0) {
      MEMSET(db->rbuf, 0, db->n_rest * sample_size);
    }
  }
  db->curbuf = 0;
  db->curdelay = newdelay;
}

void Delay::delay_update(delaybuffer_t *db,
			 void *buf,	     
			 int sample_size,
			 int sample_spacing,
			 int delay,
			 void *optional_target_buf)
{
  change_delay(db, sample_size, delay);
  if (optional_target_buf != NULL) {
    copy_to_delaybuf(optional_target_buf, buf, sample_size, sample_spacing, db->fragsize);
    buf = optional_target_buf;
    sample_spacing = 1;
  }
  if (db->n_fbufs > 0) {
    update_delay_buffer(db, sample_size, sample_spacing, (uint8_t*)buf);
  } else if (db->n_rest > 0) {
    update_delay_short_buffer(db, sample_size, sample_spacing, (uint8_t*)buf);
  }
}

delaybuffer_t * Delay::delay_allocate_buffer(int fragment_size,
					     int initdelay,
					     int maxdelay,
					     int sample_size)
{
  delaybuffer_t *db;
  int n, delay;
  int size;
  
  /* if maxdelay is negative, no delay changing will be allowed, thus
     memory need only to be allocated for the current delay */
  db = new delaybuffer_t [sizeof(delaybuffer_t)];
  MEMSET(db, 0, sizeof(delaybuffer_t));
  db->fragsize = fragment_size;
  
  delay = (maxdelay <= 0) ? initdelay : maxdelay;
  if (maxdelay >= 0 && delay > maxdelay) {
    delay = initdelay = maxdelay;
  }
  db->curdelay = initdelay;
  db->maxdelay = maxdelay;	
  if (delay == 0) {
    return db;
  }
  if (delay <= fragment_size) {
    /* optimise for short delay */
    db->n_rest = initdelay; /* current value */
    size = delay * sample_size;
    db->shortbuf[0] = emallocaligned(size);
    db->shortbuf[1] = emallocaligned(size);
    MEMSET(db->shortbuf[0], 0, size);
    MEMSET(db->shortbuf[1], 0, size);
    return db;
  }
  if (maxdelay > 0) {
    /* allocate full-length short buffers to keep this option if the
       delay is reduced in run-time */
    size = fragment_size * sample_size;
    db->shortbuf[0] = emallocaligned(size);
    db->shortbuf[1] = emallocaligned(size);
    MEMSET(db->shortbuf[0], 0, size);
    MEMSET(db->shortbuf[1], 0, size);
  }
  
  db->n_rest = initdelay % fragment_size;
  db->n_fbufs = initdelay / fragment_size + 1;
  if (db->n_fbufs == 1) {
    db->n_fbufs = 0;
  }
  db->n_fbufs_cap = delay / fragment_size + 1;
  db->fbufs = new void* [db->n_fbufs_cap * sizeof(void *)];
  size = fragment_size * sample_size;
  for (n = 0; n < db->n_fbufs_cap; n++) {
    db->fbufs[n] = emallocaligned(size);
    MEMSET(db->fbufs[n], 0, size);
  }
  if (maxdelay > 0) {
    db->rbuf = emallocaligned(size);
    MEMSET(db->rbuf, 0, size);
  } else if (db->n_rest != 0) {
    size = db->n_rest * sample_size;
    db->rbuf = emallocaligned(size);
    MEMSET(db->rbuf, 0, size);
  }
  return db;
}

