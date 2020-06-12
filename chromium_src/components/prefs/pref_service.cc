/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../components/prefs/pref_service.cc"

namespace prefs {

BravePrefService* BravePrefService::GetInstance() {
  return base::Singleton<BravePrefService>::get();
}

void BravePrefService::RegisterGetPrefsCallback(const GetPrefsCallback& cb) {
  get_prefs_callback_ = cb;
}

PrefService* BravePrefService::GetPrefs() {
  if (!get_prefs_callback_) {
    NOTREACHED();
    return nullptr;
  }
  return get_prefs_callback_.Run();
}

BravePrefService::BravePrefService() {}

BravePrefService::~BravePrefService() {}

}  // namespace prefs
