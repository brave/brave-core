/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_FORCED_REQUESTING_ORIGIN \
  !forced_requesting_origin_.is_empty() ? forced_requesting_origin_:

#include "src/components/permissions/permission_manager.cc"

#undef BRAVE_FORCED_REQUESTING_ORIGIN
