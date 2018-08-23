/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "sflogic_cli.hpp"

void SFLOGIC_CLI::clear_changes(void)
{
  int n, i;
  if (icomm->fctrl.empty()) {
    return;
  }
  for (n = 0; n < sfconf->n_filters; n++) {
    newstate.fctrl[n].coeff = icomm->fctrl[n].coeff;
    newstate.fctrl[n].delayblocks = icomm->fctrl[n].delayblocks;
    for (i = 0; i < sfconf->filters[n].n_channels[OUT]; i++) {
      newstate.fctrl[n].scale[OUT][i] = icomm->fctrl[n].scale[OUT][i];
    }
    for (i = 0; i < sfconf->filters[n].n_channels[IN]; i++) {
      newstate.fctrl[n].scale[IN][i] = icomm->fctrl[n].scale[IN][i];
      newstate.fctrl[n].fscale[i] = icomm->fctrl[n].fscale[i];
    }
  }
  MEMSET(newstate.toggle_mute, 0, sizeof(newstate.toggle_mute));
  MEMSET(newstate.fchanged, 0, sizeof(newstate.fchanged));
  MEMSET(newstate.delay, -1, sizeof(newstate.delay));
  for (n = 0; n < SF_MAXCHANNELS; n++) {
    newstate.subdelay[IN][n] = SF_UNDEFINED_SUBDELAY;
    newstate.subdelay[OUT][n] = SF_UNDEFINED_SUBDELAY;
  }
}

void SFLOGIC_CLI::commit_changes(FILE *stream)
{
  int n, i;
  int IO;
  
  FOR_IN_AND_OUT {
    for (n = 0; n < sfconf->n_channels[IO]; n++) {
      if (newstate.delay[IO][n] != -1) {
	if (sfaccess->set_delay(IO, n, newstate.delay[IO][n]) == -1) {
	  fprintf(stream, "Could not change %s delay.\n", IO == IN ? "input" : "output");
	}
      }
      if (newstate.subdelay[IO][n] != SF_UNDEFINED_SUBDELAY) {
	if (sfaccess->set_subdelay(IO, n, newstate.subdelay[IO][n]) == -1) {
	  fprintf(stream, "Could not change %s subdelay.\n", IO == IN ? "input" : "output");
	}
      }
      if (newstate.toggle_mute[IO][n]) {
	sfaccess->toggle_mute(IO, n);
      }
    }
  }
  for (n = 0; n < sfconf->n_filters; n++) {
    if (!newstate.fchanged[n]) {
      continue;
    }
    icomm->fctrl[n].coeff = newstate.fctrl[n].coeff;
    icomm->fctrl[n].delayblocks = newstate.fctrl[n].delayblocks;
    for (i = 0; i < sfconf->filters[n].n_channels[OUT]; i++) {
      icomm->fctrl[n].scale[OUT][i] = newstate.fctrl[n].scale[OUT][i];
    }
    for (i = 0; i < sfconf->filters[n].n_channels[IN]; i++) {
      icomm->fctrl[n].scale[IN][i] = newstate.fctrl[n].scale[IN][i];
      icomm->fctrl[n].fscale[i] = newstate.fctrl[n].fscale[i];
    }
  }
}

bool SFLOGIC_CLI::are_changes(void)
{
  int n;
  int IO;
  
  FOR_IN_AND_OUT {
    for (n = 0; n < sfconf->n_channels[IO]; n++) {
      if (newstate.delay[IO][n] != -1 || newstate.subdelay[IO][n] != SF_UNDEFINED_SUBDELAY || newstate.toggle_mute[IO][n]) {
	return true;
      }
    }
  }
  for (n = 0; n < sfconf->n_filters; n++) {
    if (newstate.fchanged[n]) {
      return true;
    }
  }
  return false;
}

void SFLOGIC_CLI::print_overflows(FILE *stream)
{
  double peak;
  int n;
  
  fprintf(stream, "peak: ");
  for (n = 0; n < sfconf->n_channels[OUT]; n++) {
    peak = (volatile double)icomm->out_overflow[n].largest;
    if (peak < (double)(volatile int32_t)icomm->out_overflow[n].intlargest) {
      peak = (double)(volatile int32_t)icomm->out_overflow[n].intlargest;
    }
    if (peak != 0.0) {
      if ((peak = 20.0 * log10(peak / icomm->out_overflow[n].max)) == 0.0) {                
	peak = -0.0;
      }
      fprintf(stream, "%d/%u/%+.2f ", n, (volatile unsigned int)icomm->out_overflow[n].n_overflows, peak);
    } else {
      fprintf(stream, "%d/%u/-Inf ", n, (volatile unsigned int)icomm->out_overflow[n].n_overflows);
    }
  }
  fprintf(stream, "\n");
}

char * SFLOGIC_CLI::strtrim(char s[])
{
  char *p;
  
  while (*s == ' ' || *s == '\t') {
    s++;
  }
  if (*s == '\0') {
    return s;
  }
  p = s + strlen(s) - 1;
  while ((*p == ' ' || *p == '\t') && p != s) {
    p--;
  }
  *(p + 1) = '\0';
  return s;
}

bool SFLOGIC_CLI::get_id(FILE *stream,
			 char str[],
			 char **p,
			 int *id,
			 int type,
			 int rid)
{
  int io = -1, n;
  
  str = strtrim(str);
  if (str[0] == '\"') {
    str += 1;
    if ((*p = strchr(str, '\"')) == NULL) {
      fprintf(stream, "Invalid string.\n");
      return false;
    }
    **p = '\0';
    switch (type) {
    case FILTER_ID:
      io = -2;
      for (*id = 0; *id < sfconf->n_filters; (*id)++) {
	if (sfconf->filters[*id].name.compare(str) == 0) {
	  break;
	}
      }
      if (*id == sfconf->n_filters) {
	fprintf(stream, "There is no filter with name \"%s\".\n", str);
	return false;
      }
      break;
    case COEFF_ID:
      for (*id = 0; *id < sfconf->n_coeffs; (*id)++) {
	//if (strcmp(sfconf->coeffs[*id].name, str) == 0) {
	if (sfconf->coeffs[*id].name.compare(str) == 0) {
	  break;
	}
      }
      if (*id == sfconf->n_coeffs) {
	fprintf(stream, "There is no coefficient set with name \"%s\".\n", str);
	return false;
      }
      break;	    
    case INPUT_ID:
    case OUTPUT_ID:
      io = (type == INPUT_ID) ? IN : OUT;
      for (*id = 0; *id <sfconf-> n_channels[io]; (*id)++) {
	if (sfconf->channels[io][*id].name.compare(str) == 0) {
	  break;
	}
      }
      if (*id == sfconf->n_channels[io]) {
	fprintf(stream, "There is no %s with name \"%s\".\n", (io == IN) ? "input" : "output", str);
	return false;
      }
      break;
    }
  } else {
    *id = strtol(str, p, 10);
    if (*p == str) {
      fprintf(stream, "Invalid number.\n");
      return false;
    }
    if (*id < 0 && type != COEFF_ID) {
      fprintf(stream, "Negative number (%d) is not allowed.\n", *id);
      return false;
    }
    switch (type) {	    
    case FILTER_ID:
      io = -2;
      if (*id >= sfconf->n_filters) {
	fprintf(stream, "Filter id %d is out of range.\n", *id);
	return false;
      }
      break;
    case COEFF_ID:
      if (*id >= (int)sfconf->n_coeffs) {
	fprintf(stream, "Coefficient set id %d is out of range.\n", *id);
	return false;
      }
      break;
    case INPUT_ID:
    case OUTPUT_ID:
      io = (type == INPUT_ID) ? IN : OUT;
      if (*id >= sfconf->n_channels[io]) {
	fprintf(stream, "%s id %d is out of range.\n", (io == IN) ? "Input" : "Output", *id);
	return false;
      }
      break;	    
    }
  }
  if (io != -1 && rid != -1) {
    if (io == -2) {
      for (n = 0; n < sfconf->filters[rid].n_filters[IN]; n++) {
	if (sfconf->filters[rid].filters[IN][n] == *id) {
	  break;
	}
      }
      if (n == sfconf->filters[rid].n_filters[IN]) {
	fprintf(stream, "Filter id %d does not exist in the given filter.\n", *id);
	return false;
      }
    } else {
      for (n = 0; n < sfconf->filters[rid].n_channels[io]; n++) {
	if (sfconf->filters[rid].channels[io][n] == *id) {
	  break;
	}
      }
      if (n == sfconf->filters[rid].n_channels[io]) {
	fprintf(stream, "%s id %d does not exist in the given filter.\n", (io == IN) ? "Input" : "Output", *id);
	return false;
      }
    }
    *id = n;
  }
  *p += 1;
  while (**p == ' ' || **p == '\t') {
    (*p)++;
  }
  return true;
}

bool SFLOGIC_CLI::parse_command(FILE *stream,
				char cmd[],
				struct sleep_task *_sleep_task)
{
  struct sleep_task sleep_task;
  int n, i, rid, id, range[2];
  vector<string> names;
  double att;
  char *p;
  int IO;
  
  if (strcmp(cmd, "lf") == 0) {
    fprintf(stream, "Filters:\n");
    for (n = 0; n < sfconf->n_filters; n++) {
      fprintf(stream, "  %d: \"%s\"\n", n, sfconf->filters[n].name.data());
      if (icomm->fctrl[n].coeff < 0) {
	fprintf(stream, "      coeff set: %d (no filter)\n", icomm->fctrl[n].coeff);
      } else {
	fprintf(stream, "      coeff set: %d\n", icomm->fctrl[n].coeff);
      }
      fprintf(stream, "      delay blocks: %d (%d samples)\n", icomm->fctrl[n].delayblocks, icomm->fctrl[n].delayblocks * sfconf->filter_length);
      FOR_IN_AND_OUT {
	fprintf(stream, (IO == IN) ? "      from inputs: ":"      to outputs:   ");
	for (i = 0; i < sfconf->filters[n].n_channels[IO]; i++) {
	  if (icomm->fctrl[n].scale[IO][i] < 0) {
	    att = -20.0 * log10(-icomm->fctrl[n].scale[IO][i]);
	  } else {
	    att = -20.0 * log10(icomm->fctrl[n].scale[IO][i]);
	  }
	  if (att == 0.0) {
	    att = 0.0000001; /* to show up as 0.0 and not -0.0 */
	  }
	  fprintf(stream, "%d/%.1f", sfconf->filters[n].channels[IO][i], att);
	  if (icomm->fctrl[n].scale[IO][i] < 0) {
	    fprintf(stream, "/-1 ");
	  } else {
	    fprintf(stream, " ");
	  }
	}
	fprintf(stream, "\n");
      }
      FOR_IN_AND_OUT {
	fprintf(stream, (IO == IN) ? "      from filters: " :"      to filters:   ");
	for (i = 0; i < sfconf->filters[n].n_filters[IO]; i++) {
	  if (IO == IN) {                        
	    if (icomm->fctrl[n].fscale[i] < 0) {
	      att = -20.0 * log10(-icomm->fctrl[n].fscale[i]);
	    } else {
	      att = -20.0 * log10(icomm->fctrl[n].fscale[i]);
	    }
	    if (att == 0.0) {
	      att = 0.0000001;
	    }
	    fprintf(stream, "%d/%.1f", sfconf->filters[n].filters[IO][i], att);
	    if (icomm->fctrl[n].fscale[i] < 0) {
	      fprintf(stream, "/-1 ");
	    } else {
	      fprintf(stream, " ");
	    }
	  } else {
	    fprintf(stream, "%d ", sfconf->filters[n].filters[IO][i]);
	  }
	}
	fprintf(stream, "\n");
      }
    }
    fprintf(stream, "\n");
  } else if (strcmp(cmd, "lc") == 0) {
    fprintf(stream, "Coefficient sets:\n");
    for (n = 0; n < sfconf->n_coeffs; n++) {
      fprintf(stream, "  %d: \"%s\" (%d blocks)\n", n, sfconf->coeffs[n].name.data(), sfconf->coeffs[n].n_blocks);
    }
    fprintf(stream, "\n");
  } else if (strcmp(cmd, "li") == 0) {
    fprintf(stream, "Input channels:\n");
    for (n = 0; n < sfconf->n_channels[IN]; n++) {
      fprintf(stream, "  %d: \"%s\" (delay: %d:%d) %s\n", n, sfconf->channels[IN][n].name.data(), sfaccess->get_delay(IN, n),
	      sfaccess->get_subdelay(IN, n),
	      sfaccess->ismuted(IN, n) ? "(muted)" : "");
    }
    fprintf(stream, "\n");
  } else if (strcmp(cmd, "lo") == 0) {
    fprintf(stream, "Output channels:\n");
    for (n = 0; n < sfconf->n_channels[OUT]; n++) {
      fprintf(stream, "  %d: \"%s\" (delay: %d:%d) %s\n", n, sfconf->channels[OUT][n].name.data(), sfaccess->get_delay(OUT, n),
	      sfaccess->get_subdelay(OUT, n),
	      sfaccess->ismuted(OUT, n) ? "(muted)" : "");
    }
    fprintf(stream, "\n");	
  } else if (strcmp(cmd, "lm") == 0) {
    names = sfaccess->sflogic_names(&i);
    if (names.empty()) {
      fprintf(stream, "Logic modules:\n");
      for (n = 0; n < i; n++) {
	fprintf(stream, "  %d: \"%s\"\n", n, names[n].data());
      }
      fprintf(stream, "\n");	
    }
    FOR_IN_AND_OUT {
      names = sfaccess->sfio_names(IO, &i);
      if (names.empty()) {
	fprintf(stream, "%s modules:\n", (IO == IN) ? "Input" : "Output");
	for (n = 0; n < i; n++) {
	  sfaccess->sfio_range(IO, n, range);
	  fprintf(stream, "  %d (%d - %d): \"%s\"\n", n, range[0], range[1], names[n].data());
	}
	fprintf(stream, "\n");	
      }
    }
  } else if (strstr(cmd, "cffa") == cmd) {
    if (get_id(stream, cmd + 4, &cmd, &rid, FILTER_ID, -1) &&
	get_id(stream, cmd, &cmd, &id, FILTER_ID, rid))
      {
	if (*cmd == 'M' || *cmd == 'm') {
	  cmd++;
	  att = strtod(cmd, &p);
	  if (cmd == p) {
	    fprintf(stream, "Invalid input multiplier.\n");
	  } else {
	    newstate.fctrl[rid].fscale[id] = att;
	    newstate.fchanged[rid] = true;
	  }
	} else {
	  att = strtod(cmd, &p);
	  if (cmd == p) {
	    fprintf(stream, "Invalid input attenuation.\n");
	  } else {
	    if (newstate.fctrl[rid].fscale[id] < 0) {
	      newstate.fctrl[rid].fscale[id] = -pow(10, -att / 20);
	    } else {
	      newstate.fctrl[rid].fscale[id] = pow(10, -att / 20);
	    }
	    newstate.fchanged[rid] = true;
	  }
	}
      }
  } else if (strstr(cmd, "cfia") == cmd) {
    if (get_id(stream, cmd + 4, &cmd, &rid, FILTER_ID, -1) &&
	get_id(stream, cmd, &cmd, &id, INPUT_ID, rid))
      {
	if (*cmd == 'M' || *cmd == 'm') {
	  cmd++;
	  att = strtod(cmd, &p);
	  if (cmd == p) {
	    fprintf(stream, "Invalid input multiplier.\n");
	  } else {
	    newstate.fctrl[rid].scale[IN][id] = att;
	    newstate.fchanged[rid] = true;
	  }
	} else {
	  att = strtod(cmd, &p);
	  if (cmd == p) {
	    fprintf(stream, "Invalid input attenuation.\n");
	  } else {
	    if (newstate.fctrl[rid].scale[IN][id] < 0) {
	      newstate.fctrl[rid].scale[IN][id] = -pow(10, -att / 20);
	    } else {
	      newstate.fctrl[rid].scale[IN][id] = pow(10, -att / 20);
	    }
	    newstate.fchanged[rid] = true;
	  }
	}
      }
  } else if (strstr(cmd, "cfoa") == cmd) {
    if (get_id(stream, cmd + 4, &cmd, &rid, FILTER_ID, -1) &&
	get_id(stream, cmd, &cmd, &id, OUTPUT_ID, rid))
      {
	if (*cmd == 'M' || *cmd == 'm') {
	  cmd++;
	  att = strtod(cmd, &p);
	  if (cmd == p) {
	    fprintf(stream, "Invalid output multiplier.\n");
	  } else {
	    newstate.fctrl[rid].scale[OUT][id] = att;
	    newstate.fchanged[rid] = true;
	  }
	} else {
	  att = strtod(cmd, &p);
	  if (cmd == p) {
	    fprintf(stream, "Invalid output attenuation.\n");
	  } else {
	    if (newstate.fctrl[rid].scale[OUT][id] < 0) {
	      newstate.fctrl[rid].scale[OUT][id] =
		-pow(10, -att / 20);
	    } else {
	      newstate.fctrl[rid].scale[OUT][id] = pow(10, -att / 20);
	    }
	    newstate.fchanged[rid] = true;
	  }
	}
      }
  } else if (strstr(cmd, "cid") == cmd) {
    if (get_id(stream, cmd + 3, &cmd, &id, INPUT_ID, -1)) {
      n = strtol(cmd, &p, 10);
      if (cmd == p || n < 0) {
	fprintf(stream, "Invalid input delay.\n");
      } else {
	newstate.delay[IN][id] = n;
      }
      cmd = p;
      n = strtol(cmd, &p, 10);
      if (cmd != p) {
	if (n <= -SF_SAMPLE_SLOTS || n >= SF_SAMPLE_SLOTS) {
	  fprintf(stream, "Invalid input subdelay.\n");
	} else {
	  newstate.subdelay[IN][id] = n;
	}
      }
    }
  } else if (strstr(cmd, "cod") == cmd) {
    if (get_id(stream, cmd + 3, &cmd, &id, OUTPUT_ID, -1)) {
      n = strtol(cmd, &p, 10);
      if (cmd == p || n < 0) {
	fprintf(stream, "Invalid output delay.\n");
      } else {
	newstate.delay[OUT][id] = n;
      }
      cmd = p;
      n = strtol(cmd, &p, 10);
      if (cmd != p) {
	if (n <= -SF_SAMPLE_SLOTS || n >= SF_SAMPLE_SLOTS) {
	  fprintf(stream, "Invalid output subdelay.\n");
	} else {
	  newstate.subdelay[OUT][id] = n;
	}
      }
    }
  } else if (strstr(cmd, "cfc") == cmd) {
    if (get_id(stream, cmd + 3, &cmd, &rid, FILTER_ID, -1) &&
	get_id(stream, cmd, &cmd, &id, COEFF_ID, rid))
      {
	newstate.fctrl[rid].coeff = id;
	newstate.fchanged[rid] = true;
      }
  } else if (strstr(cmd, "cfd") == cmd) {
    if (get_id(stream, cmd + 3, &cmd, &rid, FILTER_ID, -1)) {
      n = strtol(cmd, &p, 10);
      if (cmd == p || n < 0 || n > sfconf->n_blocks - 1) {
	fprintf(stream, "Invalid filter delay.\n");
      } else {
	newstate.fctrl[rid].delayblocks = n;
	newstate.fchanged[rid] = true;
      }
    }
  } else if (strstr(cmd, "tmo") == cmd) {
    if (get_id(stream, cmd + 3, &cmd, &id, OUTPUT_ID, -1)) {
      newstate.toggle_mute[OUT][id] = !newstate.toggle_mute[OUT][id];
    }
  } else if (strstr(cmd, "tmi") == cmd) {
    if (get_id(stream, cmd + 3, &cmd, &id, INPUT_ID, -1)) {
      newstate.toggle_mute[IN][id] = !newstate.toggle_mute[IN][id];
    }
  } else if (strstr(cmd, "imc") == cmd) {
    id = strtol(cmd + 3, &p, 10);
    if (p == cmd + 3) {
      id = -1;
    }
    /*if (sfaccess->sfio_command(IN, id, p, &p) == -1) {
      fprintf(stream, "Command failed: %s\n", p);
    } else {
      fprintf(stream, "%s", p);
      }*/
    sfaccess->sfio_command(IN, id, p, &p);
    fprintf(stream, "%s", p);
  } else if (strstr(cmd, "omc") == cmd) {
    id = strtol(cmd + 3, &p, 10);
    if (p == cmd + 3) {
      id = -1;
    }
    /*if (sfaccess->sfio_command(OUT, id, p, &p) == -1) {
      fprintf(stream, "Command failed: %s\n", p);
    } else {
      fprintf(stream, "%s", p);
      }*/
    sfaccess->sfio_command(OUT, id, p, &p);
    fprintf(stream, "%s", p);
  } else if (strstr(cmd, "lmc") == cmd) {
    id = strtol(cmd + 3, &p, 10);
    if (p == cmd + 3) {
      id = -1;
      names = sfaccess->sflogic_names(&i);
      if (!names.empty()) {
	p = strtrim(cmd + 3);
	for (n = 0; n < i; n++) {
	  if (strstr(p, names[n].data()) == p) {
	    id = n;
	    p += strlen(names[n].data());
	    break;
	  }
	}
      }
    }
    if (sfaccess->sflogic_command(id, p, &p) == -1) {
      fprintf(stream, "Command failed: %s\n", p);
    } else {
      fprintf(stream, "%s", p);
    }
  } else if (strcmp(cmd, "ppk") == 0) {
    print_overflows(stream);
  } else if (strcmp(cmd, "rpk") == 0) {
    sfaccess->reset_peak();
  } else if (strcmp(cmd, "upk") == 0) {
    print_peak_updates = !print_peak_updates;
  } else if (strcmp(cmd, "tp") == 0) {
    print_prompt = !print_prompt;
  } else if (strcmp(cmd, "rti") == 0) {
    fprintf(stream, "Realtime index: %.3f\n", sfaccess->realtime_index());
  } else if (strcmp(cmd, "quit") == 0) {
    return false;
  } else if (strstr(cmd, "sleep") == cmd) {
    MEMSET(&sleep_task, 0, sizeof(struct sleep_task));
    if ((p = strchr(cmd + 5, 'b')) != NULL) {
      n = strtol(p + 1, &p, 10);
      if (p != NULL && n >= 0) {
	sleep_task.block_sleep = true;
	sleep_task.do_sleep = true;
	sleep_task.blocks = n;
      }
    } else {
      n = strtol(cmd + 5, &p, 10);
      if (p != NULL && n >= 0) {
	sleep_task.do_sleep = true;
	sleep_task.seconds = n;
	sleep_task.useconds = atoi(p) * 1000;
      }
    }
    if (sleep_task.do_sleep) {
      if (_sleep_task == NULL) {
	if (sleep_task.block_sleep) {
	  fprintf(stream, "Block sleep only valid in scripts\n");
	} else {
	  if (sleep_task.seconds > 0) {
	    sleep(sleep_task.seconds);
	  }
	  if (sleep_task.useconds > 0) {
	    usleep(sleep_task.useconds);
	  }
	}
            } else {
	MEMCPY(_sleep_task, &sleep_task, sizeof(struct sleep_task));
      }
    }
  } else if (strstr(cmd, "abort") == cmd) {
    sfaccess->exit(SF_EXIT_OK);
    } else if (strcmp(cmd, "help") == 0) {
    fprintf(stream, HELP_TEXT);
  } else {
    fprintf(stream, "Unknown command \"%s\", type \"help\" for help.\n",
	    cmd);
  }
  return true;
}

void SFLOGIC_CLI::wait_data(FILE *client_stream,
			    int client_fd,
			    int callback_fd)
{
    fd_set rfds;
    uint32_t msg;
    int n;

    FD_ZERO(&rfds);
    
    do {
	FD_SET(client_fd, &rfds);
	FD_SET(callback_fd, &rfds);
	if (client_stream != NULL) {
	    fflush(client_stream);
	}
	while ((n = select(client_fd < callback_fd ? callback_fd + 1 :
			   client_fd + 1, &rfds, NULL, NULL, NULL)) == -1
	       && errno == EINTR);
	if (n == -1) {
	    fprintf(stderr, "CLI: Select failed: %s.\n", strerror(errno));
	    sfaccess->exit(SF_EXIT_OTHER);
	}
	if (FD_ISSET(callback_fd, &rfds)) {
	    if (!readfd(callback_fd, &msg, 4)) {
                sfaccess->exit(SF_EXIT_OK);
            }
	    switch (msg) {
	    case SF_FDEVENT_PEAK:
		if (print_peak_updates) {
		  print_overflows(client_stream);
		}
		break;
	    default:
		fprintf(stderr, "CLI: Invalid callback code: %d.\n", msg);
		sfaccess->exit(SF_EXIT_OTHER);
		break;
	    }
	}
    } while (!FD_ISSET(client_fd, &rfds));
}

bool SFLOGIC_CLI::parse(FILE *stream,
			const char cmd[],
			struct sleep_task *sleep_task)
{
    char *p, *s, *s1, *buf;
    bool do_quit;
    int len;

    if (sleep_task != NULL) {
        MEMSET(sleep_task, 0, sizeof(struct sleep_task));
    }
    do_quit = false;
    len = strlen(cmd);
    if (len == 0) {
        return true;
    }
    buf = (char*)alloca(len + 1);
    MEMCPY(buf, cmd, len + 1);
    if (buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
    if (len > 1 && buf[len - 2] == '\r') {
        buf[len - 2] = '\0';
    }
    s = buf;
    clear_changes();
    do {
        if ((p = strchr(s, ';')) != NULL) {
            *p = '\0';
        }
        while (*s == ' ' || *s == '\t') s++;
        if (*s != '\0') {
            s1 = strtrim(s);
            if (print_commands) {
                fprintf(stream, "%s\n", s1);
            }
            if (!parse_command(stream, s1, sleep_task)) {
                do_quit = true;
            }
        } else if (are_changes()) {
            sfaccess->control_mutex(1);
            commit_changes(stream);
            sfaccess->control_mutex(0);
            clear_changes();
        }
    } while ((s = p + 1) != (char *)1);
    if (are_changes()) {
        sfaccess->control_mutex(1);
        commit_changes(stream);
        sfaccess->control_mutex(0);
    }    
    return !do_quit;
}

void SFLOGIC_CLI::block_start(unsigned int block_index,
			      struct timeval *current_time)
{

    struct sleep_task sleep_task;
    struct timeval tv;

    if (do_quit) {
        return;
    }
    if (do_sleep) {
        if (sleep_block) {
            if (block_index <= sleep_block_index) {
                return;
            }
        } else {
            if (timercmp(current_time, &sleep_time, <)) {
                return;
            }
        }
        do_sleep = false;
    }
    if (p == NULL) {
        p = script;
    }
    cmdchr = false;
    retchr = false;
    p1 = p;
    /* this loop extracts the next non-empty line, and handles wrap */
    while ((*p1 != '\0' && *p1 != '\n') || !cmdchr) {
        switch ((int)*p1) {
        case ' ': case '\t': case '\r':
            break;
        case '\n':
            p = &p1[1];
            break;
        case '\0':
            if (p == script) {
                fprintf(stderr, "CLI: the script is empty.\n");
                sfaccess->exit(SF_EXIT_INVALID_CONFIG);
            }
            p = script;
            p1 = &p[-1];
            break;
        default:
            cmdchr = true;
            break;
        }
        p1++;
    }
    /* search for empty statements on the line */
    cmdchr = false;
    p2 = p;
    while (p2 < p1) {
        switch ((int)*p2) {
        case ';':
            if (!cmdchr) {
                /* empty statement, treat as linebreak */
                p1 = p2;
            }
            cmdchr = false;
            break;
        case ' ': case '\t': case '\r':
            break;
        default:
            cmdchr = true;
            break;
        }
        p2++;
    }
    /* handle \r\n line ends */
    restore_char = *p1;
    *p1 = '\0';
    if (p1 != script && p1[-1] == '\r') {
        retchr = true;
        p1[-1] = '\0';
    }

    if (!parse(stderr, p, &sleep_task)) {
        do_quit = true;
    }

    if (retchr) {
        p1[-1] = '\r';
    }
    *p1 = restore_char;
    if (*p1 == '\0') {
        /* wrap */
        p = script;
    } else {
        p = &p1[1];
    }

    if (sleep_task.do_sleep) {
        do_sleep = true;
        if (sleep_task.block_sleep) {
            sleep_block = true;
            sleep_block_index = block_index + sleep_task.blocks;
        } else {
            sleep_block = false;
            tv.tv_sec = sleep_task.seconds;
            tv.tv_usec = sleep_task.useconds;
            timeradd(current_time, &tv, &sleep_time);
        }
    }
}

void SFLOGIC_CLI::parse_string(FILE *stream,
			       const char inbuf[MAXCMDLINE],
			       char cmd[MAXCMDLINE])
{
    int slen, n, i;
    
    slen = strlen(inbuf);
    cmd[0] = '\0';
    for (n = 0, i = 0; n < slen; n++) {
        switch ((int)inbuf[n]) {
        case 27:
            fprintf(stream, "ESC sequences not supported, "
                    "discarding line.\n");
            cmd[0] = '\0';
            return;
        case '\b':
            if (i > 0) {
                i--;
            }
            break;
        default:
            if (inbuf[n] == '\n' || inbuf[n] == '\r' ||
                (inbuf[n] > 31 && inbuf[n] < 127))
            {
                cmd[i++] = inbuf[n];
            } else {
                fprintf(stream, "unsupported character in input (%u), "
                        "discarding line.\n", (unsigned int)inbuf[n]);
                cmd[0] = '\0';
                return;
            }
            break;
        }
    }
    cmd[i] = '\0';
}

void SFLOGIC_CLI::stream_loop(int event_fd,
			      int infd,
			      FILE *instream,
			      FILE *outstream)
{
    char inbuf[MAXCMDLINE], cmd[MAXCMDLINE];
    
    inbuf[MAXCMDLINE - 1] = '\0';	
    while (true) {
      wait_data(outstream, infd, event_fd);
      if (fgets(inbuf, MAXCMDLINE - 1, instream) == NULL) {
	fprintf(stderr, "CLI: fgets failed: %s.\n", strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      parse_string(outstream, inbuf, cmd);
      parse(outstream, cmd, NULL);
    }
}

void SFLOGIC_CLI::socket_loop(int event_fd, int lsock)
{
  char inbuf[MAXCMDLINE], cmd[MAXCMDLINE];
  FILE *stream;
  int sock;
  
  while (true) {
    wait_data(NULL, lsock, event_fd);
    if ((sock = accept(lsock, NULL, NULL)) == -1) {
      fprintf(stderr, "CLI: Accept failed: %s.\n", strerror(errno));
      sfaccess->exit(SF_EXIT_OTHER);
    }
    if ((stream = fdopen(sock, "r+")) == NULL) {
      fprintf(stderr, "CLI: fdopen failed: %s.\n", strerror(errno));
      sfaccess->exit(SF_EXIT_OTHER);
    }
    setvbuf(stream, NULL, _IOLBF, 0);
    
    fprintf(stream, WELCOME_TEXT);
    fprintf(stream, PROMPT_TEXT);
    
    wait_data(stream, sock, event_fd);
    cmd[MAXCMDLINE - 1] = '\0';	
    while (fgets(inbuf, MAXCMDLINE - 1, stream) != NULL) {
      parse_string(stream, inbuf, cmd);
      if (!parse(stream, cmd, NULL)) {
	break;
      }
      if (print_prompt) {
	fprintf(stream, PROMPT_TEXT);
      }
      wait_data(stream, sock, event_fd);
    }
    print_peak_updates = false;
    fclose(stream);
  }
}

SFLOGIC_CLI::SFLOGIC_CLI(struct sfconf *_sfconf,
			 intercomm_area *_icomm,
			 SfAccess *_sfaccess)  : SFLOGIC(_sfconf, _icomm, _sfaccess)
{
  unique = true;
  lport = NULL;
  script = NULL;
  print_peak_updates = false;
  print_prompt = true;
  print_commands = true;
  debug = false;
  MEMSET(&newstate, 0, sizeof(newstate));
  error[0] = '\0';
  port = -1;
  port2 = -1;
  line_speed = 9600;
  do_quit = false;
  do_sleep = false;
  p = NULL;
  //fork_mode = SF_FORK_PRIO_OTHER;
}

int SFLOGIC_CLI::preinit(xmlNodePtr sfparams, int _debug)
{
    debug = !!_debug;

    while (sfparams != NULL) {
      if (!xmlStrcmp(sfparams->name, (const xmlChar *)"lport")) {
	lport = strdup((char*)xmlNodeGetContent(sfparams));
      } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"port")) {
	port = (int)atoi((char*)xmlNodeGetContent(sfparams));
      } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"script")) {
	script = strdup((char*)xmlNodeGetContent(sfparams));
      } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"echo")) {
	print_commands = (!xmlStrcmp(xmlNodeGetContent(sfparams), (const xmlChar *)"true") ? true : false);
      } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"line_speed")) {
	line_speed = atoi((char*)xmlNodeGetContent(sfparams));
      }
      sfparams = sfparams->next;
    }

    if (script == NULL) {
      if (port == -1 && lport == NULL) {
	fprintf(stderr, "CLI: \"port\" or \"script\" must be set.\n");
	return -1;
      }    
      //sfevents->fdevents = SF_FDEVENT_PEAK;
      fork_mode = SF_FORK_PRIO_MAX;
    } else {
      if (port != -1 || lport != NULL) {
	fprintf(stderr, "CLI: Cannot have both \"script\" and \"port\" set.\n");
	return -1;
      }    
      //sfevents->block_start = block_start;
      fork_mode = SF_FORK_DONT_FORK;
    }
    return 0;
}

#define WRITE_TO_SYNCH_FD                                               \
if (sem_post(synch_sem)==-1) {						\
  sfaccess->exit(SF_EXIT_OTHER);	                                \
}

int SFLOGIC_CLI::init(int event_fd, sem_t *synch_sem)
{
    FILE *stream, *instream, *outstream;
    int lsock, fd, speed, opt;
    struct sockaddr_in s_in;
    struct sockaddr_un s_un;
    struct termios newtio;

    //fctrl = icomm->fctrl;
    
    if (script != NULL) {
      return 0;
    }

    if (lport != NULL && strstr(lport, "/dev/") == lport) {
      /* serial line interface */
      if ((fd = open(lport, O_RDWR | O_NOCTTY)) == -1) {
	fprintf(stderr, "CLI: Failed to open serial device: %s.\n", strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      speed = B9600;
      switch (line_speed) {
      case 0: speed = B9600; break;
      case 1200: speed = B1200; break;
      case 2400: speed = B2400; break;
      case 4800: speed = B4800; break;
      case 9600: speed = B9600; break;
      case 19200: speed = B19200; break;
      case 38400: speed = B38400; break;
      case 57600: speed = B57600; break;
      case 115200: speed = B115200; break;
      case 230400: speed = B230400; break;
      default:
	fprintf(stderr, "CLI: Invalid/unsupported serial line speed %d.\n", speed);
	sfaccess->exit(SF_EXIT_OTHER);
	return 0;
      }
      MEMSET(&newtio, 0, sizeof(newtio));
      newtio.c_cflag = CS8 | CLOCAL | CREAD;
      cfsetispeed(&newtio, speed);
      cfsetospeed(&newtio, speed);
      newtio.c_iflag = IGNPAR | ICRNL | ISTRIP;
      newtio.c_oflag = OPOST | ONLCR;
      newtio.c_lflag = ICANON;
#ifdef _POSIX_VDISABLE
      {
	int _i, _n;
	_i = sizeof(newtio.c_cc);
	_i /= sizeof(newtio.c_cc[0]);
	for (_n = 0; _n < _i; _n++) {
	  newtio.c_cc[_n] = _POSIX_VDISABLE;
	}
      }
#endif    
      if (tcflush(fd, TCIFLUSH) == -1) {
	fprintf(stderr, "CLI: tcflush failed: %s.\n", strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
	fprintf(stderr, "CLI: tcsetattr failed: %s.\n", strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      if ((stream = fdopen(fd, "r+")) == NULL) {
	fprintf(stderr, "CLI: fdopen failed: %s.\n", strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      setvbuf(stream, NULL, _IOLBF, 0);
      WRITE_TO_SYNCH_FD;
      
      stream_loop(event_fd, fd, stream, stream);
        
    } else if (port != -1 && port2 != -1) {
      /* pipe interface */
      if ((instream = fdopen(port, "r")) == NULL) {
	fprintf(stderr, "CLI: fdopen 'r' on fd %d failed: %s.\n", port, strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      setvbuf(instream, NULL, _IOLBF, 0);
      if ((outstream = fdopen(port2, "w")) == NULL) {
	fprintf(stderr, "CLI: fdopen 'w' on fd %d failed: %s.\n", port2, strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      setvbuf(outstream, NULL, _IOLBF, 0);
      WRITE_TO_SYNCH_FD;
      
      stream_loop(event_fd, port, instream, outstream);
      
    } else if (port != -1) {
      /* TCP interface */
      MEMSET(&s_in, 0, sizeof(s_in));
      s_in.sin_family = AF_INET;
      s_in.sin_addr.s_addr = INADDR_ANY;
      s_in.sin_port = htons(port);
      
      if ((lsock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
	fprintf(stderr, "CLI: Failed to create socket: %s.\n", strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      opt = 1;
      if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
	fprintf(stderr, "CLI: Failed to set socket options: %s.\n", strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }    
      if (bind(lsock, (struct sockaddr *)&s_in, sizeof(struct sockaddr_in)) == -1) {
	fprintf(stderr, "CLI: Failed to bind name to socket: %s.\n", strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      if (listen(lsock, 1) != 0) {
	fprintf(stderr, "CLI: Failed to listen on port %d: %s.\n", port, strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      WRITE_TO_SYNCH_FD;
      
      socket_loop(event_fd, lsock);
      
    } else if (lport != NULL) {
      /* local socket interface */
      remove(lport);
      MEMSET(&s_un, 0, sizeof(s_un));
      s_un.sun_family = AF_UNIX;
      strncpy(s_un.sun_path, lport, sizeof(s_un.sun_path));
      s_un.sun_path[sizeof(s_un.sun_path) - 1] = '\0';
      
      if ((lsock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
	fprintf(stderr, "CLI: Failed to create socket: %s.\n", strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }
      
      if (bind(lsock, (struct sockaddr *)&s_un, sizeof(struct sockaddr_un)) == -1) {
	if (errno == EADDRINUSE) {
	  fprintf(stderr, "CLI: Failed to create local socket: path \"%s\" already exists.\n", s_un.sun_path);
	} else {
	  fprintf(stderr, "CLI: Failed to bind name to socket: %s.\n", strerror(errno));
	}
	sfaccess->exit(SF_EXIT_OTHER);
      }
      if (listen(lsock, 1) != 0) {
	fprintf(stderr, "CLI: Failed to listen on local socket \"%s\": %s.\n", s_un.sun_path, strerror(errno));
	sfaccess->exit(SF_EXIT_OTHER);
      }	
      free(lport);
      WRITE_TO_SYNCH_FD;
      
      socket_loop(event_fd, lsock);
    
    } else {
      fprintf(stderr, "CLI: No port specified.\n");
      sfaccess->exit(SF_EXIT_OTHER);
    }
    
    return 0;
}
