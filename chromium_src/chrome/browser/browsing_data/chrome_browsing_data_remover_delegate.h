/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_DELEGATE_H_

#include "build/build_config.h"

class BraveBrowsingDataRemoverDelegate;

#define BRAVE_CHROME_BROWSING_DATA_REMOVER_DELEGATE_H \
  friend class BraveBrowsingDataRemoverDelegate;

#if !BUILDFLAG(IS_ANDROID)
class FalseBoolStub {
 public:
  bool operator=(bool) { return false; }
  explicit operator bool() const { return false; }
};

#define should_clear_sync_account_settings_          \
  unused1_ = false;                                  \
  FalseBoolStub should_clear_sync_account_settings_; \
  bool unused2_

#endif  // !BUILDFLAG(IS_ANDROID)

#include "src/chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"  // IWYU pragma: export

#if !BUILDFLAG(IS_ANDROID)
#undef should_clear_sync_account_settings_
#endif

#undef BRAVE_CHROME_BROWSING_DATA_REMOVER_DELEGATE_H

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BROWSING_DATA_CHROME_BROWSING_DATA_REMOVER_DELEGATE_H_
