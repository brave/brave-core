/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSER_PROCESS_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSER_PROCESS_IMPL_H_

// Note: Init method name is quite common. To re-define only Init in
// browser_process_impl.h, all other headers are added.
#include "base/debug/stack_trace.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/buildflags.h"
#include "components/keep_alive_registry/keep_alive_state_observer.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/nacl/common/buildflags.h"
#include "components/prefs/persistent_pref_store.h"
#include "components/prefs/pref_change_registrar.h"
#include "extensions/buildflags/buildflags.h"
#include "media/media_buildflags.h"
#include "ppapi/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "services/network/public/cpp/network_quality_tracker.h"
#include "services/network/public/mojom/network_service.mojom-forward.h"

#define Init virtual Init
#include "src/chrome/browser/browser_process_impl.h"
#undef Init

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSER_PROCESS_IMPL_H_
