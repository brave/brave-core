/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"  // For OS_LINUX

#if defined(OS_LINUX)
#include "content/browser/media/cdm_registry_impl.h"

#include <algorithm>

#define Init Init_ChromiumImpl
#include "../../../../../content/browser/media/cdm_registry_impl.cc"  // NOLINT
#undef Init

namespace content {

namespace {
// This leaks but would be fine.
CdmInfo* g_widevine_info = nullptr;
}

void CdmRegistryImpl::Init() {
  Init_ChromiumImpl();

  // On linux, we want to register widevine cdm to CdmRegistry when users opt
  // in. Otherwise, widevine is initialized by default w/o user accept.
  // So, widevine cdm is erased from |cdms_| and it is registered when users opt
  // in. Also, we try to register it during the startup in
  // BraveBrowserMainExtraParts::PreMainMessageLoopRun() by checking opted in
  // prefs.
  cdms_.erase(
      std::remove_if(cdms_.begin(), cdms_.end(), [](const CdmInfo& info) {
        // It would be better to use |kWidevineKeySystem| constant instead of
        // using "com.widevine.alpha". To use constant, it requires adding
        // widevine dependency to content module.
        // However, using this string directly seems fine because it would not
        // be changed and can avoid addtional patching for this.
        if (info.supported_key_system == "com.widevine.alpha") {
          DCHECK(!g_widevine_info);
          // Cache upstream created info to reuse when brave wants to register
          // it later.
          g_widevine_info = new CdmInfo(info);
          return true;
        }
        return false;
      }),
      cdms_.end());
}

const CdmInfo& CdmRegistryImpl::cached_widevine_cdm_info() const {
  DCHECK(g_widevine_info);
  return *g_widevine_info;
}

}  // namespace content

#else  // OS_LINUX
// This overridden file is only used on linux.
#include "../../../../../content/browser/media/cdm_registry_impl.cc"  // NOLINT
#endif
