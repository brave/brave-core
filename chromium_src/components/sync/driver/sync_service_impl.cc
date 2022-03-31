/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/driver/brave_sync_auth_manager.h"
#include "brave/components/sync/driver/brave_sync_stopped_reporter.h"

#define SyncAuthManager BraveSyncAuthManager
#define SyncStoppedReporter BraveSyncStoppedReporter

#include "src/components/sync/driver/sync_service_impl.cc"

#undef SyncAuthManager
#undef SyncStoppedReporter
