/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/state/state_migration.h"

namespace {

constexpr int kCurrentVersionNumber = 14;

}  // namespace

namespace brave_rewards::internal::state {

StateMigration::StateMigration(RewardsEngine& engine)
    : engine_(engine),
      v1_(engine),
      v2_(engine),
      v4_(engine),
      v5_(engine),
      v6_(engine),
      v7_(engine),
      v8_(engine),
      v10_(engine),
      v11_(engine),
      v12_(engine),
      v13_(engine),
      v14_(engine) {}

StateMigration::~StateMigration() = default;

void StateMigration::Start(ResultCallback callback) {
  Migrate(std::move(callback));
}

void StateMigration::FreshInstall(ResultCallback callback) {
  engine_->Log(FROM_HERE) << "Fresh install, state version set to "
                          << kCurrentVersionNumber;
  engine_->state()->SetVersion(kCurrentVersionNumber);
  std::move(callback).Run(mojom::Result::OK);
}

void StateMigration::Migrate(ResultCallback callback) {
  int current_version = engine_->state()->GetVersion();

  if (current_version < 0) {
    engine_->state()->SetVersion(0);
    current_version = 0;
  }

  auto& options = engine_->options();

  if (options.is_testing &&
      current_version == options.state_migration_target_version_for_testing) {
    return std::move(callback).Run(mojom::Result::OK);
  }

  if (current_version == kCurrentVersionNumber) {
    std::move(callback).Run(mojom::Result::OK);
    return;
  }

  const int new_version = current_version + 1;

  auto migrate_callback =
      base::BindOnce(&StateMigration::OnMigration, base::Unretained(this),
                     std::move(callback), new_version);

  switch (new_version) {
    case 1: {
      v1_.Migrate(std::move(migrate_callback));
      return;
    }
    case 2: {
      v2_.Migrate(std::move(migrate_callback));
      return;
    }
    case 3: {
      v3_.Migrate(std::move(migrate_callback));
      return;
    }
    case 4: {
      v4_.Migrate(std::move(migrate_callback));
      return;
    }
    case 5: {
      v5_.Migrate(std::move(migrate_callback));
      return;
    }
    case 6: {
      v6_.Migrate(std::move(migrate_callback));
      return;
    }
    case 7: {
      v7_.Migrate(std::move(migrate_callback));
      return;
    }
    case 8: {
      v8_.Migrate(std::move(migrate_callback));
      return;
    }
    case 9: {
      v9_.Migrate(std::move(migrate_callback));
      return;
    }
    case 10: {
      v10_.Migrate(std::move(migrate_callback));
      return;
    }
    case 11: {
      v11_.Migrate(std::move(migrate_callback));
      return;
    }
    case 12: {
      v12_.Migrate(std::move(migrate_callback));
      return;
    }
    case 13: {
      v13_.Migrate(std::move(migrate_callback));
      return;
    }
    case 14: {
      v14_.Migrate(std::move(migrate_callback));
      return;
    }
  }

  engine_->LogError(FROM_HERE)
      << "Migration version is not handled " << new_version;
}

void StateMigration::OnMigration(ResultCallback callback,
                                 int version,
                                 mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "State: Error with migration from "
                                 << (version - 1) << " to " << version;
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  engine_->Log(FROM_HERE) << "State: Migrated to version " << version;
  engine_->state()->SetVersion(version);

  // If the user did not previously have a state version and the initial
  // migration did not find any rewards data stored in JSON files, assume that
  // this is a "fresh" Rewards profile and skip the remaining migrations.
  if (version == 1 && !v1_.legacy_data_migrated()) {
    FreshInstall(std::move(callback));
    return;
  }

  Migrate(std::move(callback));
}

}  // namespace brave_rewards::internal::state
