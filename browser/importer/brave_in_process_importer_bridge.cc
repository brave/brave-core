/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_in_process_importer_bridge.h"
#include "chrome/browser/importer/external_process_importer_host.h"

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

void BraveInProcessImporterBridge::UpdateStats(const BraveStats& stats) {
  writer_->UpdateStats(stats);
}

void BraveInProcessImporterBridge::UpdateLedger(
    const BraveLedger& ledger) {
  writer_->SetBridge(this);
  writer_->UpdateLedger(ledger);
}

void BraveInProcessImporterBridge::FinishLedgerImport() {
  NotifyItemEnded(importer::LEDGER);
  NotifyEnded();
}

void BraveInProcessImporterBridge::Cancel() {
  host_->Cancel();
}

void BraveInProcessImporterBridge::UpdateReferral(
    const BraveReferral& referral) {
  writer_->UpdateReferral(referral);
}

void BraveInProcessImporterBridge::UpdateWindows(
    const ImportedWindowState& windowState) {
  // TODO(grobinson): Can we just restore windows/tabs here?
  // Do we even need to do anything with the ProfileWriter?
  writer_->UpdateWindows(windowState);
}

void BraveInProcessImporterBridge::UpdateSettings(
      const SessionStoreSettings& settings) {
  writer_->UpdateSettings(settings);
}

BraveInProcessImporterBridge::~BraveInProcessImporterBridge() {}
