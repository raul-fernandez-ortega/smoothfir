/*
 * (c) Copyright 2001 - 2004 -- Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#if REALSIZE == 4
#define RXX r32
#define REAL_T float
#elif REALSIZE == 8
#define RXX r64
#define REAL_T double
#else
 #error invalid REALSIZE
#endif 

#define REAL_OVERFLOW_UPDATE                                                   \
    if (realbuf->RXX[n] < 0.0) {                                               \
        if (realbuf->RXX[n] < rmin) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->RXX[n] < -overflow->largest) {                            \
            overflow->largest = -realbuf->RXX[n];                              \
        }                                                                      \
    } else {                                                                   \
        if (realbuf->RXX[n] > rmax) {                                          \
            overflow->n_overflows++;                                           \
        }                                                                      \
        if (realbuf->RXX[n] > overflow->largest) {                             \
            overflow->largest = realbuf->RXX[n];                               \
        }                                                                      \
    }

static void
RAW2REAL_NAME(void *_realbuf,
              void *_rawbuf,
	      int bits,
              int bytes,
              int shift,
              bool isfloat,
              int spacing,
              bool swap,
              int n_samples,
	      struct sfoverflow *overflow)
{
    numunion_t *realbuf, *rawbuf, sample;
    int32_t imin, imax;
    REAL_T rmin, rmax;
    int n, i;

    realbuf = (numunion_t *)_realbuf;
    rawbuf = (numunion_t *)_rawbuf;
    if (isfloat) {
        rmin = -overflow->max;
        rmax = overflow->max;
	if (shift != 0) {
	    fprintf(stderr, "Shift must be zero for floating point formats.\n");
	    exit(SF_EXIT_OTHER);
	}
#if REALSIZE == 4
	//#define RXX r32
	//#define REAL_T float        
        switch (bytes) {
        case 4:
            if (swap) {
                for (n = i = 0; n < n_samples; n++, i += spacing) {
                    realbuf->u32[n] = SWAP32(rawbuf->u32[i]);
		    REAL_OVERFLOW_UPDATE;
                }
            } else {
                for (n = i = 0; n < n_samples; n++, i += spacing) {
                    realbuf->r32[n] = rawbuf->r32[i];
		    REAL_OVERFLOW_UPDATE;
                }
            }
            break;
        case 8:
            if (swap) {
                for (n = i = 0; n < n_samples; n++, i += spacing) {
                    sample.u64[0] = SWAP64(rawbuf->u64[i]);
                    realbuf->r32[n] = (float)sample.r64[0];
		    REAL_OVERFLOW_UPDATE;
                }
            } else {
                for (n = i = 0; n < n_samples; n++, i += spacing) {
                    realbuf->r32[n] = (float)rawbuf->r64[i];
		    REAL_OVERFLOW_UPDATE;
                }
            }
            break;
        default:
            goto raw2real_invalid_byte_size;
        }
#elif REALSIZE == 8
	//#define RXX r64
	//#define REAL_T double        
        switch (bytes) {
        case 4:
            if (swap) {
                for (n = i = 0; n < n_samples; n++, i += spacing) {
                    sample.u32[0] = SWAP32(rawbuf->u32[i]);
                    realbuf->r64[n] = (double)sample.r32[0];
		    REAL_OVERFLOW_UPDATE;
                }
            } else {
                for (n = i = 0; n < n_samples; n++, i += spacing) {
                    realbuf->r64[n] = (double)rawbuf->r32[i];
		    REAL_OVERFLOW_UPDATE;
                }
            }
            break;
        case 8:
            if (swap) {
                for (n = i = 0; n < n_samples; n++, i += spacing) {
                    realbuf->u64[n] = SWAP64(rawbuf->u64[i]);
		    REAL_OVERFLOW_UPDATE;
                }
            } else {
                for (n = i = 0; n < n_samples; n++, i += spacing) {
                    realbuf->r64[n] = rawbuf->r64[i];
		    REAL_OVERFLOW_UPDATE;
                }
            }
            break;
        default:
            goto raw2real_invalid_byte_size;
        }
#else
#error invalid REALSIZE
#endif        
        return;
    }
    imin = -(1 << (bits - 1));
    imax = (1 << (bits - 1)) - 1;
    rmin = (REAL_T)imin;
    rmax = (REAL_T)imax;

    sample.u64[0] = 0;
    switch (bytes) {
    case 1:
	for (n = i = 0; n < n_samples; n++, i += spacing) {
	    realbuf->RXX[n] = (REAL_T)rawbuf->i8[i];
	}
	break;
    case 2:
	if (shift == 0) {
	    if (swap) {
		for (n = i = 0; n < n_samples; n++, i += spacing) {
		    realbuf->RXX[n] = (REAL_T)((int16_t)SWAP16(rawbuf->u16[i]));
		}
	    } else {
		for (n = i = 0; n < n_samples; n++, i += spacing) {
		    realbuf->RXX[n] = (REAL_T)rawbuf->i16[i];
		}
	    }
	} else {
	    if (swap) {
		for (n = i = 0; n < n_samples; n++, i += spacing) {
		    realbuf->RXX[n] =
                        (REAL_T)((int16_t)SWAP16(rawbuf->u16[i]) >> shift);
		}
	    } else {
		for (n = i = 0; n < n_samples; n++, i += spacing) {
		    realbuf->RXX[n] = (REAL_T)(rawbuf->i16[i] >> shift);
		}
	    }
	}
	break;
    case 3:
	spacing = spacing * 3 - 3;
        shift += 8;
#ifdef __BIG_ENDIAN__
        if (swap) {
            for (n = i = 0; n < n_samples; n++, i += spacing) {
                sample.u8[2] = rawbuf->u8[i++];
                sample.u8[1] = rawbuf->u8[i++];
                sample.u8[0] = rawbuf->u8[i++];
                realbuf->RXX[n] = (REAL_T)(sample.i32[0] >> shift);
            }
        } else {
            for (n = i = 0; n < n_samples; n++, i += spacing) {
                sample.u8[0] = rawbuf->u8[i++];
                sample.u8[1] = rawbuf->u8[i++];
                sample.u8[2] = rawbuf->u8[i++];
                realbuf->RXX[n] = (REAL_T)(sample.i32[0] >> shift);
            }
	}
#endif        
#ifdef __LITTLE_ENDIAN__
        if (swap) {
            for (n = i = 0; n < n_samples; n++, i += spacing) {
                sample.u8[3] = rawbuf->u8[i++];
                sample.u8[2] = rawbuf->u8[i++];
                sample.u8[1] = rawbuf->u8[i++];
                realbuf->RXX[n] = (REAL_T)(sample.i32[0] >> shift);
            }
        } else {
            for (n = i = 0; n < n_samples; n++, i += spacing) {
                sample.u8[1] = rawbuf->u8[i++];
                sample.u8[2] = rawbuf->u8[i++];
                sample.u8[3] = rawbuf->u8[i++];
                realbuf->RXX[n] = (REAL_T)(sample.i32[0] >> shift);
            }
        }
#endif
	break;
    case 4:
	if (shift == 0) {
	    if (swap) {
		for (n = i = 0; n < n_samples; n++, i += spacing) {
		    realbuf->RXX[n] = (REAL_T)((int32_t)SWAP32(rawbuf->u32[i]));
		}
	    } else {
		for (n = i = 0; n < n_samples; n++, i += spacing) {
		    realbuf->RXX[n] = (REAL_T)rawbuf->i32[i];
		}
	    }
	} else {
	    if (swap) {
		for (n = i = 0; n < n_samples; n++, i += spacing) {
		    realbuf->RXX[n] =
                        (REAL_T)((int32_t)SWAP32(rawbuf->u32[i]) >> shift);
		}
	    } else {
		for (n = i = 0; n < n_samples; n++, i += spacing) {
		    realbuf->RXX[n] = (REAL_T)(rawbuf->i32[i] >> shift);
		}
	    }
	}
	break;
    default:
    raw2real_invalid_byte_size:
	fprintf(stderr, "Sample byte size %d is not suppported.\n", bytes);
	exit(SF_EXIT_OTHER);
	break;
    }    
}

#undef RXX
#undef REAL_T
#undef REAL_OVERFLOW_UPDATE
