/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/history/core/browser/history_service.h"

#include "components/history/core/common/pref_names.h"
#include "components/prefs/pref_service.h"

#include <components/history/core/browser/history_service.cc>

namespace history {

namespace {

constexpr int kDefaultHistoryRetentionDays = 90;
constexpr int kKeepHistoryForever = -1;

base::TimeDelta HistoryRetentionDaysToTimeDelta(int days) {
  if (days == kKeepHistoryForever) {
    return base::TimeDelta::Max();
  }
  return base::Days(days > 0 ? days : kDefaultHistoryRetentionDays);
}

}  // namespace

void HistoryService::GetKnownToSyncCount(
    base::OnceCallback<void(HistoryCountResult)> callback) {
  backend_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&HistoryBackend::GetKnownToSyncCount, history_backend_),
      std::move(callback));
}

void HistoryService::InitHistoryRetentionPref(PrefService* prefs) {
  DCHECK(prefs);

  history_retention_days_.Init(
      prefs::kBraveHistoryRetentionDays, prefs,
      base::BindRepeating(&HistoryService::OnHistoryRetentionDaysChanged,
                          base::Unretained(this)));
  OnHistoryRetentionDaysChanged();
}

void HistoryService::OnHistoryRetentionDaysChanged() {
  const base::TimeDelta threshold =
      HistoryRetentionDaysToTimeDelta(history_retention_days_.GetValue());
  backend_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&HistoryBackend::UpdateExpirationThreshold,
                                history_backend_, threshold));
}

}  // namespace history
