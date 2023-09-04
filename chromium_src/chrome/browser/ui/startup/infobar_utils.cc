/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/brave_obsolete_system_infobar_delegate.h"
#include "build/build_config.h"
#include "chrome/browser/ui/session_crashed_bubble.h"
#include "chrome/browser/ui/startup/google_api_keys_infobar_delegate.h"
#include "components/infobars/content/content_infobar_manager.h"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#include "chrome/browser/ui/startup/obsolete_system_infobar_delegate.h"
#endif

class BraveGoogleKeysInfoBarDelegate {
 public:
  static void Create(infobars::ContentInfoBarManager* infobar_manager) {
    // lulz
  }
};

#define ShowIfNotOffTheRecordProfile ShowIfNotOffTheRecordProfileBrave
#define GoogleApiKeysInfoBarDelegate BraveGoogleKeysInfoBarDelegate

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#define ObsoleteSystemInfoBarDelegate BraveObsoleteSystemInfoBarDelegate
#endif

#include "src/chrome/browser/ui/startup/infobar_utils.cc"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#undef ObsoleteSystemInfoBarDelegate
#endif

#undef GoogleApiKeysInfoBarDelegate
#undef ShowIfNotOffTheRecordProfile
