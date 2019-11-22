/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/logging.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_bug_tracker.h"

#undef QUIC_BUG_IF
#define QUIC_BUG_IF DVLOG
#include "../../../../../../../../../net/third_party/quiche/src/quic/core/crypto/transport_parameters.cc"
#undef QUIC_BUG_IF
