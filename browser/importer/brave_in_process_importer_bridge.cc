/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_in_process_importer_bridge.h"

BraveInProcessImporterBridge::BraveInProcessImporterBridge(
    ProfileWriter* writer,
    base::WeakPtr<ExternalProcessImporterHost> host) :
  InProcessImporterBridge(writer, host),
  writer_(static_cast<BraveProfileWriter*>(writer)) {
}

void BraveInProcessImporterBridge::SetCookies(
    const std::vector<net::CanonicalCookie>& cookies) {
  writer_->AddCookies(cookies);
}

BraveInProcessImporterBridge::~BraveInProcessImporterBridge() {}
