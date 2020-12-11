/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/restricted_cookie_manager.h"

#define BRAVE_DELETIONFILTERTOINFO       \
  delete_info.ephemeral_storage_domain = \
      std::move(filter->ephemeral_storage_domain);

#include "../../../../../services/network/cookie_manager.cc"
