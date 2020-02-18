/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_COMMON_IMPORTER_IMPORTER_BRIDGE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_COMMON_IMPORTER_IMPORTER_BRIDGE_H_

#include "net/cookies/canonical_cookie.h"

struct BraveLedger;
struct BraveStats;
struct BraveReferral;
struct ImportedWindowState;
struct SessionStoreSettings;

#define BRAVE_IMPORTER_BRIDGE_H \
 public: \
  virtual void SetCookies( \
      const std::vector<net::CanonicalCookie>& cookies) {} \
  virtual void UpdateStats(const BraveStats& stats) {} \
  virtual void UpdateLedger(const BraveLedger& ledger) {} \
  virtual void UpdateReferral(const BraveReferral& referral) {} \
  virtual void UpdateWindows(const ImportedWindowState& windowState) {} \
  virtual void UpdateSettings(const SessionStoreSettings& settings) {}

#include "../../../../../chrome/common/importer/importer_bridge.h"

#undef BRAVE_IMPORTER_BRIDGE_H

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_IMPORTER_IMPORTER_BRIDGE_H_
