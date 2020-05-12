/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_migration.h"
#include "bat/ledger/internal/state/state_util.h"

using std::placeholders::_1;

namespace {

const int kCurrentVersionNumber = 1;

}  // namespace

namespace braveledger_state {

StateMigration::StateMigration(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

StateMigration::~StateMigration() = default;

void StateMigration::Migrate(ledger::ResultCallback callback) {
  const int current_version = GetVersion(ledger_);
  const int new_version = current_version + 1;

  if (current_version == kCurrentVersionNumber) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  auto migrate_callback = std::bind(&StateMigration::OnMigration,
      this,
      _1,
      new_version,
      callback);

  switch (new_version) {
    case 1: {
      MigrateToV1(migrate_callback);
      return;
    }
  }

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Migration version is not handled " << new_version;
  NOTREACHED();
}

void StateMigration::OnMigration(
    ledger::Result result,
    const int version,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
        "State: Error with migration from " <<
        (version - 1) <<
        " to " << version;
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO) <<
      "State: Migrated to version " << version;

  SetVersion(ledger_, version);
  Migrate(callback);
}

void StateMigration::MigrateToV1(ledger::ResultCallback callback) {
  legacy_publisher_ =
      std::make_unique<braveledger_publisher::LegacyPublisherState>(ledger_);

  auto load_callback = std::bind(&StateMigration::OnLoadState,
      this,
      _1,
      callback);

  legacy_publisher_->Load(load_callback);
}

void StateMigration::OnLoadState(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result == ledger::Result::NO_PUBLISHER_STATE) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
        "Failed to load publisher state file, setting default values";
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  ledger_->SetIntegerState(
      ledger::kStateMinVisitTime,
      static_cast<int>(legacy_publisher_->GetPublisherMinVisitTime()));

  ledger_->SetIntegerState(
      ledger::kStateMinVisits,
      static_cast<int>(legacy_publisher_->GetPublisherMinVisits()));

  ledger_->SetBooleanState(
      ledger::kStateAllowNonVerified,
      legacy_publisher_->GetPublisherAllowNonVerified());

  ledger_->SetBooleanState(
      ledger::kStateAllowVideoContribution,
      legacy_publisher_->GetPublisherAllowVideos());

  // TODO(nejc): migrate processed publishers in db

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_state
