/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_state.h"

#define ShouldSSLErrorsBeFatal(host) \
  ShouldSSLErrorsBeFatal(ssl_config_.network_anonymization_key, host)

#include "src/net/socket/ssl_client_socket_impl.cc"

#undef ShouldSSLErrorsBeFatal
