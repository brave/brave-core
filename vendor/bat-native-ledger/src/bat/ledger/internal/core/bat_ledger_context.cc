/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/bat_ledger_context.h"

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/ledger_client.h"

namespace ledger {

namespace {

size_t g_next_component_key = 0;

}  // namespace

BATLedgerContext::BATLedgerContext(LedgerImpl* ledger_impl)
    : ledger_client_(ledger_impl->ledger_client()), ledger_impl_(ledger_impl) {
  DCHECK(ledger_client_);
}

BATLedgerContext::BATLedgerContext(LedgerClient* ledger_client)
    : ledger_client_(ledger_client) {
  DCHECK(ledger_client_);
}

BATLedgerContext::~BATLedgerContext() = default;

using Component = BATLedgerContext::Component;
using ComponentKey = BATLedgerContext::ComponentKey;

ComponentKey::ComponentKey() : value_(g_next_component_key++) {}

Component::Component(BATLedgerContext* context) : context_(context) {}

Component::~Component() = default;

using LogStream = BATLedgerContext::LogStream;

LogStream::LogStream(BATLedgerContext* context,
                     base::Location location,
                     LogLevel log_level)
    : context_(context), location_(location), log_level_(log_level) {}

LogStream::LogStream(LogStream&& other)
    : context_(other.context_),
      location_(other.location_),
      log_level_(other.log_level_),
      stream_(std::move(other.stream_)) {
  other.moved_ = true;
}

LogStream& LogStream::operator=(LogStream&& other) {
  context_ = other.context_;
  location_ = other.location_;
  log_level_ = other.log_level_;
  stream_ = std::move(other.stream_);

  other.moved_ = true;
  return *this;
}

LogStream::~LogStream() {
  if (!moved_) {
    context_->GetLedgerClient()->Log(
        location_.file_name(), location_.line_number(),
        static_cast<int>(log_level_), stream_.str());
  }
}

LogStream BATLedgerContext::Log(base::Location location, LogLevel log_level) {
  return LogStream(this, location, log_level);
}

LogStream BATLedgerContext::LogError(base::Location location) {
  return LogStream(this, location, LogLevel::kError);
}

LogStream BATLedgerContext::LogInfo(base::Location location) {
  return LogStream(this, location, LogLevel::kInfo);
}

LogStream BATLedgerContext::LogVerbose(base::Location location) {
  return LogStream(this, location, LogLevel::kVerbose);
}

LogStream BATLedgerContext::LogFull(base::Location location) {
  return LogStream(this, location, LogLevel::kFull);
}

}  // namespace ledger
