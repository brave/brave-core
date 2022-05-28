/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define CanAddURLToHistory CanAddURLToHistory_ChromiumImpl
#include "src/chrome/browser/history/history_utils.cc"
#undef CanAddURLToHistory

#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/components/constants/url_constants.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#endif

bool CanAddURLToHistory(const GURL& url) {
  if (!CanAddURLToHistory_ChromiumImpl(url))
    return false;

  bool is_brave_scheme = url.SchemeIs(content::kBraveUIScheme);
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  bool is_wallet_host =
      url.SchemeIs(kChromeExtensionScheme) &&
      url.host() == ethereum_remote_client_extension_id;
  return !is_brave_scheme && !is_wallet_host;
#else
  return !is_brave_scheme;
#endif
}
