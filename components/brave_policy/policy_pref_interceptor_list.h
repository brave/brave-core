/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_PREF_INTERCEPTOR_LIST_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_PREF_INTERCEPTOR_LIST_H_

#include <string_view>
#include <vector>

#include "base/component_export.h"
#include "base/containers/span.h"
#include "base/no_destructor.h"

namespace brave_policy {

// Singleton that holds the list of pref names which do not support dynamic
// policy refresh. Must be populated via SetPrefs() early in process startup
// before any PolicyPrefInterceptor is used.
class COMPONENT_EXPORT(POLICY_PREF_INTERCEPTOR) PolicyPrefInterceptorList {
 public:
  static PolicyPrefInterceptorList* GetInstance();

  PolicyPrefInterceptorList(const PolicyPrefInterceptorList&) = delete;
  PolicyPrefInterceptorList& operator=(const PolicyPrefInterceptorList&) =
      delete;

  // Sets the list of non-dynamic pref names. The string_view entries must
  // reference data with static storage duration for the lifetime of the
  // process.
  void SetPrefs(base::span<const std::string_view> prefs);

  base::span<const std::string_view> GetPrefs() const;

 private:
  friend class base::NoDestructor<PolicyPrefInterceptorList>;

  PolicyPrefInterceptorList();
  ~PolicyPrefInterceptorList();

  std::vector<std::string_view> prefs_;
};

}  // namespace brave_policy

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_POLICY_PREF_INTERCEPTOR_LIST_H_
