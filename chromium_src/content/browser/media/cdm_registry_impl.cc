/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/browser/media/cdm_registry_impl.h"

#if defined(OS_LINUX)
#include <algorithm>
#endif

#define Init Init_ChromiumImpl
#include "../../../../../content/browser/media/cdm_registry_impl.cc"
#undef Init

namespace content {
void CdmRegistryImpl::Init() {
  Init_ChromiumImpl();

#if defined(OS_LINUX)
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
        return info.supported_key_system == "com.widevine.alpha";
      }),
      cdms_.end());
#endif
}
}  // namespace content
