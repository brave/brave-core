/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_THREADING_THREAD_RESTRICTIONS_H_
#define BRAVE_CHROMIUM_SRC_BASE_THREADING_THREAD_RESTRICTIONS_H_

class BraveBrowsingDataRemoverDelegate;
namespace ipfs {
class IpfsService;
}

#define BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_H  \
  friend class ::BraveBrowsingDataRemoverDelegate; \
  friend class ipfs::IpfsService;

#include "src/base/threading/thread_restrictions.h"
#undef BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_H

#endif  // BRAVE_CHROMIUM_SRC_BASE_THREADING_THREAD_RESTRICTIONS_H_
