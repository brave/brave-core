// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#include "BraveEmbeddedTor.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <tor/feature/api/tor_api.h>

struct brave_tor_host {
  tor_main_configuration_t* configuration;
  char** arguments;
  int argument_count;
  int control_descriptor;
  pthread_t thread;
};

// C Tor permits only one active instance in a process.
static pthread_mutex_t process_lock = PTHREAD_MUTEX_INITIALIZER;
static bool process_active = false;

static void release_process_slot(void) {
  pthread_mutex_lock(&process_lock);
  process_active = false;
  pthread_mutex_unlock(&process_lock);
}

static void free_host(brave_tor_host* host) {
  for (int i = 0; i < host->argument_count; ++i)
    free(host->arguments[i]);
  free(host->arguments);
  free(host);
}

static void* run_tor(void* context) {
  brave_tor_host* host = context;
  tor_run_main(host->configuration);
  tor_main_configuration_free(host->configuration);
  return NULL;
}

static void* reap_tor(void* context) {
  brave_tor_host* host = context;
  pthread_join(host->thread, NULL);
  close(host->control_descriptor);
  free_host(host);
  release_process_slot();
  return NULL;
}

brave_tor_host* brave_tor_host_start(int argc,
                                    const char* const* argv,
                                    int* control_descriptor) {
  *control_descriptor = -1;
  if (argc < 1)
    return NULL;

  pthread_mutex_lock(&process_lock);
  if (process_active) {
    pthread_mutex_unlock(&process_lock);
    return NULL;
  }
  process_active = true;
  pthread_mutex_unlock(&process_lock);

  brave_tor_host* host = calloc(1, sizeof(*host));
  if (!host) {
    release_process_slot();
    return NULL;
  }
  host->control_descriptor = -1;
  host->argument_count = argc;
  host->arguments = calloc((size_t)argc + 1, sizeof(char*));
  if (!host->arguments)
    goto fail;
  for (int i = 0; i < argc; ++i) {
    host->arguments[i] = strdup(argv[i]);
    if (!host->arguments[i])
      goto fail;
  }
  host->configuration = tor_main_configuration_new();
  if (!host->configuration)
    goto fail;
  if (tor_main_configuration_set_command_line(
          host->configuration, argc, host->arguments) < 0)
    goto fail_configuration;
  host->control_descriptor =
      tor_main_configuration_setup_control_socket(host->configuration);
  if (host->control_descriptor < 0)
    goto fail_configuration;
  int no_sigpipe = 1;
  setsockopt(host->control_descriptor, SOL_SOCKET, SO_NOSIGPIPE,
             &no_sigpipe, sizeof(no_sigpipe));
  if (pthread_create(&host->thread, NULL, run_tor, host) != 0) {
    close(host->control_descriptor);
    goto fail_configuration;
  }
  *control_descriptor = host->control_descriptor;
  return host;

fail_configuration:
  tor_main_configuration_free(host->configuration);
fail:
  if (host->arguments)
    free_host(host);
  else
    free(host);
  release_process_slot();
  return NULL;
}

void brave_tor_host_release(brave_tor_host* host) {
  if (!host)
    return;
  // Closing Tor's owning connection requests shutdown, even during bootstrap.
  shutdown(host->control_descriptor, SHUT_RDWR);
  pthread_t reaper;
  if (pthread_create(&reaper, NULL, reap_tor, host) == 0)
    pthread_detach(reaper);
  else
    reap_tor(host);
}
