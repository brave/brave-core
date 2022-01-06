/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/bat_ledger_context.h"

#include <utility>

#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/option_keys.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

namespace {

BATLedgerContext::Environment GetEnvironment() {
  switch (ledger::_environment) {
    case mojom::Environment::DEVELOPMENT:
      return BATLedgerContext::Environment::kDevelopment;
    case mojom::Environment::STAGING:
      return BATLedgerContext::Environment::kStaging;
    case mojom::Environment::PRODUCTION:
      return BATLedgerContext::Environment::kProduction;
  }
}

BATLedgerContext::Options GetOptions(LedgerClient* ledger_client) {
  DCHECK(ledger_client);
  return {.environment = GetEnvironment(),
          .auto_contribute_allowed =
              !ledger_client->GetBooleanOption(option::kIsBitflyerRegion),
          .enable_experimental_features = ledger_client->GetBooleanOption(
              option::kEnableExperimentalFeatures)};
}

}  // namespace

BATLedgerContext::BATLedgerContext(LedgerImpl* ledger_impl)
    : ledger_client_(ledger_impl->ledger_client()),
      ledger_impl_(ledger_impl),
      options_(GetOptions(ledger_client_)) {
  DCHECK(ledger_client_);
}

BATLedgerContext::BATLedgerContext(LedgerClient* ledger_client)
    : ledger_client_(ledger_client), options_(GetOptions(ledger_client_)) {
  DCHECK(ledger_client_);
}

BATLedgerContext::~BATLedgerContext() = default;

BATLedgerContext::Object::Object() = default;

BATLedgerContext::Object::~Object() = default;

using LogStream = BATLedgerContext::LogStream;

LogStream::LogStream(base::WeakPtr<BATLedgerContext> context,
                     base::Location location,
                     LogLevel log_level)
    : context_(context), location_(location), log_level_(log_level) {
  DCHECK(context);
}

LogStream::LogStream(LogStream&& other)
    : context_(std::move(other.context_)),
      location_(other.location_),
      log_level_(other.log_level_),
      stream_(std::move(other.stream_)) {}

LogStream& LogStream::operator=(LogStream&& other) {
  context_ = std::move(other.context_);
  location_ = other.location_;
  log_level_ = other.log_level_;
  stream_ = std::move(other.stream_);
  return *this;
}

LogStream::~LogStream() {
  if (context_) {
    context_->GetLedgerClient()->Log(
        location_.file_name(), location_.line_number(),
        static_cast<int>(log_level_), stream_.str());
  }
}

LogStream BATLedgerContext::Log(base::Location location, LogLevel log_level) {
  return LogStream(GetWeakPtr(), location, log_level);
}

LogStream BATLedgerContext::LogError(base::Location location) {
  return LogStream(GetWeakPtr(), location, LogLevel::kError);
}

LogStream BATLedgerContext::LogInfo(base::Location location) {
  return LogStream(GetWeakPtr(), location, LogLevel::kInfo);
}

LogStream BATLedgerContext::LogVerbose(base::Location location) {
  return LogStream(GetWeakPtr(), location, LogLevel::kVerbose);
}

LogStream BATLedgerContext::LogFull(base::Location location) {
  return LogStream(GetWeakPtr(), location, LogLevel::kFull);
}

}  // namespace ledger
