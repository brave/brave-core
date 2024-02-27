/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/worker_fetch_context.h"

#include "third_party/blink/public/platform/web_content_settings_client.h"

#define AllowScript AllowScript_Unused
#include "src/third_party/blink/renderer/core/loader/worker_fetch_context.cc"
#undef AllowScript

namespace blink {

bool WorkerFetchContext::AllowScript(const KURL& url) const {
  if (!global_scope_->ContentSettingsClient()) {
    return true;
  }
  // If we're on a worker, script should be enabled, so no need to plumb
  // Settings::GetScriptEnabled() here.
  return global_scope_->ContentSettingsClient()->AllowScriptFromSource(true,
                                                                       url);
}

}  // namespace blink
