/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "components/autofill/core/browser/metrics/autofill_metrics.h"

class PrefService;
namespace syncer {
class SyncService;
}  // namespace syncer

namespace autofill {
class LogManager;
class PersonalDataManager;
bool IsCreditCardUploadEnabled(
    const syncer::SyncService* sync_service,
    const std::string& user_country,
    AutofillMetrics::PaymentsSigninState signin_state_for_metrics,
    LogManager* log_manager) {
  return false;
}
bool IsCreditCardMigrationEnabled(PersonalDataManager* personal_data_manager,
                                  syncer::SyncService* sync_service,
                                  bool is_test_mode,
                                  LogManager* log_manager) {
  return false;
}
}  // namespace autofill

#define IsCreditCardUploadEnabled IsCreditCardUploadEnabled_ChromiumImpl
#define IsCreditCardMigrationEnabled IsCreditCardMigrationEnabled_ChromiumImpl
#include "src/components/autofill/core/browser/autofill_experiments.cc"
#undef IsCreditCardUploadEnabled
#undef IsCreditCardMigrationEnabled
