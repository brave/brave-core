/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "components/autofill/core/browser/sync_utils.h"

class PrefService;
namespace syncer {
class SyncService;
}  // namespace syncer

namespace autofill {
class LogManager;
bool IsCreditCardUploadEnabled(const PrefService* pref_service,
                               const syncer::SyncService* sync_service,
                               const std::string& user_email,
                               const AutofillSyncSigninState sync_state,
                               LogManager* log_manager) {
  return false;
}
}  // namespace autofill

#define IsCreditCardUploadEnabled IsCreditCardUploadEnabled_ChromiumImpl
#include "../../../../../../components/autofill/core/browser/autofill_experiments.cc"
#undef IsCreditCardUploadEnabled
