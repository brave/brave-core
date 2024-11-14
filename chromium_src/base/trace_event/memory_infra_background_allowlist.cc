/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/trace_event/memory_infra_background_allowlist.h"

#define IsMemoryAllocatorDumpNameInAllowlist \
  IsMemoryAllocatorDumpNameInAllowlist_ChromiumImpl
#include "src/base/trace_event/memory_infra_background_allowlist.cc"
#undef IsMemoryAllocatorDumpNameInAllowlist

namespace base::trace_event {

bool IsMemoryAllocatorDumpNameInAllowlist(const std::string& name) {
  if (name.starts_with(
          "extensions/value_store/Extensions.Database.Open.BraveWallet/")) {
    return true;
  }
  if (name.starts_with("extensions/value_store/"
                       "Extensions.Database.Open.AdBlock Custom Resources/")) {
    return true;
  }
  return IsMemoryAllocatorDumpNameInAllowlist_ChromiumImpl(name);
}

}  // namespace base::trace_event
