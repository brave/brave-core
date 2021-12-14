/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_INVALIDATION_IMPL_FCM_NETWORK_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_INVALIDATION_IMPL_FCM_NETWORK_HANDLER_H_

#include "components/invalidation/impl/fcm_sync_network_channel.h"

#define StartListening StartListening_ChromiumImpl(); \
  void StartListening
#include "src/components/invalidation/impl/fcm_network_handler.h"
#undef StartListening

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_INVALIDATION_IMPL_FCM_NETWORK_HANDLER_H_
