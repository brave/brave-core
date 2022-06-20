/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/prefs/pref_manager.h"

#include "base/check_op.h"
#include "bat/ads/internal/base/logging_util.h"

namespace ads {

namespace {
PrefManager* g_pref_manager_instance = nullptr;
}  // namespace

PrefManager::PrefManager() {
  DCHECK(!g_pref_manager_instance);
  g_pref_manager_instance = this;
}

PrefManager::~PrefManager() {
  DCHECK_EQ(this, g_pref_manager_instance);
  g_pref_manager_instance = nullptr;
}

// static
PrefManager* PrefManager::Get() {
  DCHECK(g_pref_manager_instance);
  return g_pref_manager_instance;
}

// static
bool PrefManager::HasInstance() {
  return !!g_pref_manager_instance;
}

void PrefManager::AddObserver(PrefManagerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void PrefManager::RemoveObserver(PrefManagerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void PrefManager::OnPrefChanged(const std::string& path) const {
  NotifyPrefChanged(path);
}

///////////////////////////////////////////////////////////////////////////////

void PrefManager::NotifyPrefChanged(const std::string& path) const {
  for (PrefManagerObserver& observer : observers_) {
    observer.OnPrefChanged(path);
  }
}

}  // namespace ads
