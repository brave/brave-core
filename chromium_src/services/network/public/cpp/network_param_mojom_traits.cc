/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_STRUCT_TRAITS_HOST_PORT_PAIR_READ \
  std::string username;                         \
  if (!data.ReadUsername(&username)) {          \
    return false;                               \
  }                                             \
  out->set_username(username);                  \
                                                \
  std::string password;                         \
  if (!data.ReadPassword(&password)) {          \
    return false;                               \
  }                                             \
  out->set_password(password);

#include "src/services/network/public/cpp/network_param_mojom_traits.cc"

#undef BRAVE_STRUCT_TRAITS_HOST_PORT_PAIR_READ
