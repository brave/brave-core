/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/email_aliases/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace email_aliases {

EmailAliasesMetrics::EmailAliasesMetrics(PrefService& pref_service)
    : pref_service_(pref_service) {
  // Suppress reporting for existing users to avoid attributing prior opt-ins.
  if (!pref_service_->GetBoolean(prefs::kSettingsPageMethodReported) &&
      pref_service_->GetBoolean(prefs::kEmailAliasesEnabled)) {
    pref_service_->SetBoolean(prefs::kSettingsPageMethodReported, true);
  }
}

EmailAliasesMetrics::~EmailAliasesMetrics() = default;

// static
void EmailAliasesMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kSettingsPageMethodReported, false);
}

void EmailAliasesMetrics::RecordSettingsPageNavigation(
    SettingsPageMethod method) {
  if (pref_service_->GetBoolean(prefs::kSettingsPageMethodReported)) {
    return;
  }
  UMA_HISTOGRAM_ENUMERATION(kSettingsPageMethodHistogramName, method);
  pref_service_->SetBoolean(prefs::kSettingsPageMethodReported, true);
}

}  // namespace email_aliases
