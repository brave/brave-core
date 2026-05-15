/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/email_aliases_metrics.h"

#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "brave/components/email_aliases/email_aliases_notes.h"
#include "brave/components/email_aliases/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace email_aliases {

namespace {
constexpr int kCountBuckets[] = {0, 5, 15};
}  // namespace

EmailAliasesMetrics::EmailAliasesMetrics(PrefService& pref_service)
    : pref_service_(pref_service),
      clipboard_copy_storage_(&pref_service,
                              prefs::kClipboardCopyCountStorage) {
  pref_change_registrar_.Init(&pref_service);
  pref_change_registrar_.Add(
      prefs::kEmailAliasesNotes,
      base::BindRepeating(&EmailAliasesMetrics::ReportNotesCount,
                          base::Unretained(this)));
  ReportAllMetrics();
}

EmailAliasesMetrics::~EmailAliasesMetrics() = default;

// static
void EmailAliasesMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAliasesPresent, false);
  registry->RegisterListPref(prefs::kClipboardCopyCountStorage);
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

void EmailAliasesMetrics::BindInterface(
    mojo::PendingReceiver<mojom::EmailAliasesMetrics> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void EmailAliasesMetrics::ReportEmailAliasPresence(bool is_present) {
  pref_service_->SetBoolean(prefs::kAliasesPresent, is_present);
  if (!is_present) {
    return;
  }
  // Suppress the settings page navigation metric for existing users who already
  // have aliases, to avoid attributing prior opt-ins.
  if (!pref_service_->GetBoolean(prefs::kSettingsPageMethodReported)) {
    pref_service_->SetBoolean(prefs::kSettingsPageMethodReported, true);
  }
  ReportAllMetrics();
}

void EmailAliasesMetrics::OnAliasCopied() {
  clipboard_copy_storage_.AddDelta(1u);
  ReportCopyCount();
}

void EmailAliasesMetrics::ReportAllMetrics() {
  report_timer_.Start(FROM_HERE, base::Time::Now() + base::Days(1),
                      base::BindOnce(&EmailAliasesMetrics::ReportAllMetrics,
                                     base::Unretained(this)));
  ReportCopyCount();
  ReportNotesCount();
}

void EmailAliasesMetrics::ReportCopyCount() {
  uint64_t total = clipboard_copy_storage_.GetWeeklySum();
  if (total == 0) {
    return;
  }
  p3a_utils::RecordToHistogramBucket(kClipboardCopyCountHistogramName,
                                     kCountBuckets, static_cast<int>(total));
}

void EmailAliasesMetrics::ReportNotesCount() {
  if (!pref_service_->GetBoolean(prefs::kAliasesPresent)) {
    return;
  }
  size_t count = EmailAliasesNotes::GetTotalCount(*pref_service_);
  p3a_utils::RecordToHistogramBucket(kNotesCountHistogramName, kCountBuckets,
                                     static_cast<int>(count));
}

}  // namespace email_aliases
