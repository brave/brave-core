/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/psst_internals_handler.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"

namespace psst {

namespace {

constexpr base::TimeDelta kFlushDelay = base::Milliseconds(250);

}  // namespace

PsstInternalsHandler::PsstInternalsHandler(PsstLogService* log_service)
    : log_service_(log_service) {
  if (log_service_) {
    log_observation_.Observe(log_service_);
  }
}

PsstInternalsHandler::~PsstInternalsHandler() = default;

void PsstInternalsHandler::BindInterface(
    mojo::PendingReceiver<mojom::PsstInternalsHandler> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void PsstInternalsHandler::BindPage(
    mojo::PendingRemote<mojom::PsstInternalsPage> page,
    BindPageCallback callback) {
  page_.reset();
  page_.Bind(std::move(page));
  std::move(callback).Run(log_service_ ? log_service_->GetEntries()
                                      : std::vector<mojom::PsstLogEntryPtr>());
}

void PsstInternalsHandler::Clear() {
  if (log_service_) {
    log_service_->Clear();
  }
}

void PsstInternalsHandler::OnPsstLogEntryAdded(
    const mojom::PsstLogEntry& entry) {
  if (!page_) {
    return;
  }
  pending_entries_.push_back(entry.Clone());
  if (!flush_timer_.IsRunning()) {
    flush_timer_.Start(
        FROM_HERE, kFlushDelay,
        base::BindOnce(&PsstInternalsHandler::FlushPendingEntries,
                       base::Unretained(this)));
  }
}

void PsstInternalsHandler::OnPsstLogCleared() {
  pending_entries_.clear();
  flush_timer_.Stop();
  if (page_) {
    page_->OnCleared();
  }
}

void PsstInternalsHandler::FlushPendingEntries() {
  if (pending_entries_.empty()) {
    return;
  }
  if (page_) {
    page_->OnEntriesAdded(std::move(pending_entries_));
  }
  pending_entries_.clear();
}

}  // namespace psst
