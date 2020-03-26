/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_

#include "net/cookies/canonical_cookie.h"

struct BraveLedger;
struct BraveStats;
struct ImportedWindowState;

#define BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H \
 public: \
  void OnCookiesImportStart(uint32_t total_cookies_count) override {} \
  void OnCookiesImportGroup( \
      const std::vector<net::CanonicalCookie>& cookies_group) override {} \
  void OnStatsImportReady(const BraveStats& stats) override {} \
  void OnLedgerImportReady(const BraveLedger& ledger) override {} \
  void OnReferralImportReady(const BraveReferral& referral) override {} \
  void OnWindowsImportReady( \
      const ImportedWindowState& window_state) override {} \
  void OnSettingsImportReady( \
      const SessionStoreSettings& settings) override {}

#include "../../../../../chrome/browser/importer/external_process_importer_client.h"
#undef BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_
