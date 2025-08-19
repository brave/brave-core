/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREFS_H_

#include "base/containers/flat_map.h"
#include "brave/components/brave_origin/brave_origin_pref_info.h"

namespace base {
template <typename T>
class NoDestructor;
}

namespace brave_origin {

// Singleton that holds BraveOrigin preference definitions and policy mappings.
// This is initialized from the browser layer during startup with data that
// includes component dependencies, allowing both the factory (browser layer)
// and policy provider (components layer) to access the same definitions
// without layering violations.
class BraveOriginPrefs {
 public:
  static BraveOriginPrefs* GetInstance();

  // Initialize with pref definitions and policy mappings from browser layer
  void Init(BraveOriginPrefMap pref_definitions,
            base::flat_map<std::string, std::string> policy_mappings);

  // Get the pref definitions (for factory and policy provider)
  const BraveOriginPrefMap& GetPrefDefinitions() const;

  // Get policy key to pref name mappings (for policy provider)
  const base::flat_map<std::string, std::string>& GetPolicyMappings() const;

  // Check if the singleton has been initialized
  bool IsInitialized() const;

  BraveOriginPrefs(const BraveOriginPrefs&) = delete;
  BraveOriginPrefs& operator=(const BraveOriginPrefs&) = delete;

 private:
  friend base::NoDestructor<BraveOriginPrefs>;

  BraveOriginPrefs();
  ~BraveOriginPrefs();

  bool initialized_;
  BraveOriginPrefMap pref_definitions_;
  base::flat_map<std::string, std::string> policy_mappings_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREFS_H_
