/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_MOJOM_TRAITS_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_MOJOM_TRAITS_H_

#define BRAVE_STRUCT_TRAITS_HOST_PORT_PAIR       \
  static const std::string& username(            \
      const net::HostPortPair& host_port_pair) { \
    return host_port_pair.username();            \
  }                                              \
                                                 \
  static const std::string& password(            \
      const net::HostPortPair& host_port_pair) { \
    return host_port_pair.password();            \
  }

#include "src/services/network/public/cpp/network_param_mojom_traits.h"  // IWYU pragma: export

#undef BRAVE_STRUCT_TRAITS_HOST_PORT_PAIR

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_MOJOM_TRAITS_H_
