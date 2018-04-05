/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/browser/importer/brave_external_process_importer_client.h"

BraveExternalProcessImporterClient::BraveExternalProcessImporterClient(
    base::WeakPtr<ExternalProcessImporterHost> importer_host,
    const importer::SourceProfile& source_profile,
    uint16_t items,
    BraveInProcessImporterBridge* bridge)
    : ExternalProcessImporterClient(
          importer_host, source_profile, items, bridge),
      total_cookies_count_(0),
      bridge_(bridge),
      cancelled_(false) {}

void BraveExternalProcessImporterClient::Cancel() {
  if (cancelled_)
    return;

  cancelled_ = true;
  ExternalProcessImporterClient::Cancel();
}

void BraveExternalProcessImporterClient::OnCookiesImportStart(
    uint32_t total_cookies_count) {
  if (cancelled_)
    return;

  total_cookies_count_ = total_cookies_count;
  cookies_.reserve(total_cookies_count);
}

void BraveExternalProcessImporterClient::OnCookiesImportGroup(
    const std::vector<net::CanonicalCookie>& cookies_group) {
  if (cancelled_)
    return;

  cookies_.insert(cookies_.end(), cookies_group.begin(),
                  cookies_group.end());
  if (cookies_.size() >= total_cookies_count_)
    bridge_->SetCookies(cookies_);
}

BraveExternalProcessImporterClient::~BraveExternalProcessImporterClient() {}
