/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define CanAddURLToHistory CanAddURLToHistory_ChromiumImpl
#include <chrome/browser/history/history_utils.cc>
#undef CanAddURLToHistory

#include "brave/components/constants/url_constants.h"
#include "extensions/buildflags/buildflags.h"

bool CanAddURLToHistory(const GURL& url) {
  if (!CanAddURLToHistory_ChromiumImpl(url))
    return false;

  bool is_brave_scheme = url.SchemeIs(content::kBraveUIScheme);
  return !is_brave_scheme;
}
