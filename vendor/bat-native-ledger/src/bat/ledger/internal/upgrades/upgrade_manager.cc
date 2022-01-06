/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/upgrades/upgrade_manager.h"

#include <algorithm>
#include <string>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/sql_store.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/upgrades/upgrade_1.h"
#include "bat/ledger/internal/upgrades/upgrade_10.h"
#include "bat/ledger/internal/upgrades/upgrade_11.h"
#include "bat/ledger/internal/upgrades/upgrade_12.h"
#include "bat/ledger/internal/upgrades/upgrade_13.h"
#include "bat/ledger/internal/upgrades/upgrade_14.h"
#include "bat/ledger/internal/upgrades/upgrade_15.h"
#include "bat/ledger/internal/upgrades/upgrade_16.h"
#include "bat/ledger/internal/upgrades/upgrade_17.h"
#include "bat/ledger/internal/upgrades/upgrade_18.h"
#include "bat/ledger/internal/upgrades/upgrade_19.h"
#include "bat/ledger/internal/upgrades/upgrade_2.h"
#include "bat/ledger/internal/upgrades/upgrade_20.h"
#include "bat/ledger/internal/upgrades/upgrade_21.h"
#include "bat/ledger/internal/upgrades/upgrade_22.h"
#include "bat/ledger/internal/upgrades/upgrade_23.h"
#include "bat/ledger/internal/upgrades/upgrade_24.h"
#include "bat/ledger/internal/upgrades/upgrade_25.h"
#include "bat/ledger/internal/upgrades/upgrade_26.h"
#include "bat/ledger/internal/upgrades/upgrade_27.h"
#include "bat/ledger/internal/upgrades/upgrade_28.h"
#include "bat/ledger/internal/upgrades/upgrade_29.h"
#include "bat/ledger/internal/upgrades/upgrade_3.h"
#include "bat/ledger/internal/upgrades/upgrade_30.h"
#include "bat/ledger/internal/upgrades/upgrade_31.h"
#include "bat/ledger/internal/upgrades/upgrade_32.h"
#include "bat/ledger/internal/upgrades/upgrade_33.h"
#include "bat/ledger/internal/upgrades/upgrade_34.h"
#include "bat/ledger/internal/upgrades/upgrade_35.h"
#include "bat/ledger/internal/upgrades/upgrade_4.h"
#include "bat/ledger/internal/upgrades/upgrade_5.h"
#include "bat/ledger/internal/upgrades/upgrade_6.h"
#include "bat/ledger/internal/upgrades/upgrade_7.h"
#include "bat/ledger/internal/upgrades/upgrade_8.h"
#include "bat/ledger/internal/upgrades/upgrade_9.h"
#include "bat/ledger/internal/upgrades/upgrade_new.h"

namespace ledger {

namespace {

using UpgradeHandler = Future<bool> (*)(BATLedgerContext* context);

struct UpgradeEntry {
  int version;
  UpgradeHandler handler;
};

template <typename T>
static Future<bool> UpgradeHandlerFor(BATLedgerContext* context) {
  return context->StartJob<T>();
}

template <typename... Ts>
struct UpgradeSequence {
  static std::vector<UpgradeEntry> MakeList() {
    return {{.version = Ts::kVersion, .handler = UpgradeHandlerFor<Ts>}...};
  }
};

using AllUpgrades = UpgradeSequence<Upgrade1,
                                    Upgrade2,
                                    Upgrade3,
                                    Upgrade4,
                                    Upgrade5,
                                    Upgrade6,
                                    Upgrade7,
                                    Upgrade8,
                                    Upgrade9,
                                    Upgrade10,
                                    Upgrade11,
                                    Upgrade12,
                                    Upgrade13,
                                    Upgrade14,
                                    Upgrade15,
                                    Upgrade16,
                                    Upgrade17,
                                    Upgrade18,
                                    Upgrade19,
                                    Upgrade20,
                                    Upgrade21,
                                    Upgrade22,
                                    Upgrade23,
                                    Upgrade24,
                                    Upgrade25,
                                    Upgrade26,
                                    Upgrade27,
                                    Upgrade28,
                                    Upgrade29,
                                    Upgrade30,
                                    Upgrade31,
                                    Upgrade32,
                                    Upgrade33,
                                    Upgrade34,
                                    Upgrade35>;

class UpgradeJob : public BATLedgerJob<bool> {
 public:
  UpgradeJob() { DCHECK(!upgrades_.empty()); }

  void Start(int target_version = 0) {
    target_version_ = target_version > 0 ? target_version : current_version();

    context().Get<SQLStore>().Open().Then(
        ContinueWith(this, &UpgradeJob::OnDatabaseOpened));
  }

 private:
  int current_version() const { return upgrades_.back().version; }

  void OnDatabaseOpened(SQLReader reader) {
    if (!reader.Step()) {
      context().LogError(FROM_HERE) << "Unable to open database";
      Complete(false);
    }

    // Read the current database version.
    starting_version_ = db_version_ = static_cast<int>(reader.ColumnInt64(0));

    // If we are performing a clean install, skip the individual upgrades and
    // initialize at the current version.
    if (starting_version_ == 0 && target_version_ == current_version()) {
      context().LogVerbose(FROM_HERE)
          << "Installing version " << target_version_;
      context()
          .StartJob<UpgradeNew>(target_version_)
          .Then(ContinueWith(this, &UpgradeJob::OnNewInstallComplete));
      return;
    }

    // Advance to the first upgrade past the current DB version.
    while (iter_ != upgrades_.end() && iter_->version <= db_version_) {
      ++iter_;
    }

    RunNextUpgrade();
  }

  void OnNewInstallComplete(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "New installation failed";
    }
    Complete(success);
  }

  void RunNextUpgrade() {
    if (iter_ == upgrades_.end() || db_version_ == target_version_) {
      return MaybeVacuumDatabase();
    }

    DCHECK_EQ(iter_->version, db_version_ + 1);

    context().LogVerbose(FROM_HERE)
        << "Upgrading to version " << iter_->version;

    iter_->handler(&context())
        .Then(ContinueWith(this, &UpgradeJob::OnUpgradeHandlerComplete));
  }

  void OnUpgradeHandlerComplete(bool success) {
    DCHECK(iter_ != upgrades_.end());

    if (!success) {
      context().LogError(FROM_HERE)
          << "Upgrade " << iter_->version << " failed";
      return Complete(false);
    }

    db_version_ = iter_->version;
    ++iter_;

    RunNextUpgrade();
  }

  void MaybeVacuumDatabase() {
    if (starting_version_ < db_version_) {
      context().LogVerbose(FROM_HERE) << "Freeing unused space in database";
      context().Get<SQLStore>().Vacuum().Then(
          ContinueWith(this, &UpgradeJob::OnDatabaseVacuumComplete));
    } else {
      Complete(true);
    }
  }

  void OnDatabaseVacuumComplete(SQLReader reader) {
    if (!reader.Succeeded()) {
      context().LogError(FROM_HERE) << "Database vacuum failed";
    }
    Complete(true);
  }

  int starting_version_ = 0;
  int db_version_ = 0;
  int target_version_ = 0;
  std::vector<UpgradeEntry> upgrades_ = AllUpgrades::MakeList();
  std::vector<UpgradeEntry>::iterator iter_ = upgrades_.begin();
};

}  // namespace

Future<bool> UpgradeManager::Initialize() {
  return context().StartJob<UpgradeJob>();
}

Future<bool> UpgradeManager::UpgradeToVersionForTesting(int version) {
  return context().StartJob<UpgradeJob>(version);
}

}  // namespace ledger
