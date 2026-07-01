/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/policy_pref_interceptor_list.h"

namespace brave_policy {

// static
PolicyPrefInterceptorList* PolicyPrefInterceptorList::GetInstance() {
  static base::NoDestructor<PolicyPrefInterceptorList> instance;
  return instance.get();
}

PolicyPrefInterceptorList::PolicyPrefInterceptorList() = default;

PolicyPrefInterceptorList::~PolicyPrefInterceptorList() = default;

void PolicyPrefInterceptorList::SetPrefs(
    base::span<const std::string_view> prefs) {
  for (const auto& pref : prefs) {
    if (!pref.empty()) {
      prefs_.push_back(pref);
    }
  }
}

base::span<const std::string_view> PolicyPrefInterceptorList::GetPrefs() const {
  return prefs_;
}

}  // namespace brave_policy
