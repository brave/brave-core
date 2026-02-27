/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/time_period_storage/pref_time_period_store.h"

#include <utility>

#include "base/check.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

PrefTimePeriodStore::PrefTimePeriodStore(PrefService* prefs,
                                         const char* pref_name)
    : prefs_(prefs), pref_name_(pref_name) {
  CHECK(prefs);
  CHECK(pref_name);
}

PrefTimePeriodStore::PrefTimePeriodStore(PrefService* prefs,
                                         const char* pref_name,
                                         const char* dict_key)
    : prefs_(prefs), pref_name_(pref_name), dict_key_(dict_key) {
  CHECK(prefs);
  CHECK(pref_name);
}

PrefTimePeriodStore::~PrefTimePeriodStore() = default;

void PrefTimePeriodStore::Set(base::ListValue data) {
  if (dict_key_) {
    ScopedDictPrefUpdate update(prefs_, pref_name_);
    update->Set(dict_key_, std::move(data));
  } else {
    prefs_->SetList(pref_name_, std::move(data));
  }
}

const base::ListValue* PrefTimePeriodStore::Get() {
  const base::Value& pref_value = prefs_->GetValue(pref_name_);

  const base::ListValue* list;
  if (dict_key_) {
    CHECK(pref_value.is_dict());
    list = pref_value.GetDict().FindList(dict_key_);
  } else {
    list = pref_value.GetIfList();
  }

  return list;
}

void PrefTimePeriodStore::Clear() {
  if (dict_key_) {
    ScopedDictPrefUpdate update(prefs_, pref_name_);
    update->Remove(dict_key_);
  } else {
    prefs_->ClearPref(pref_name_);
  }
}
