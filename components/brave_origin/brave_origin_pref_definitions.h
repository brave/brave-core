/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREF_DEFINITIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREF_DEFINITIONS_H_

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_origin/brave_origin_pref_info.h"

namespace base {
template <typename T>
class NoDestructor;
}
namespace brave_origin {

// Singleton that holds BraveOrigin preference definitions.
// This is initialized from the browser layer during startup with data that
// includes component dependencies, allowing both the factory (browser layer)
// and policy provider (components layer) to access the same definitions
// without layering violations.
class BraveOriginPrefDefinitions {
 public:
  static BraveOriginPrefDefinitions* GetInstance();

  // Initialize with pref definitions mappings from browser layer
  void Init(BraveOriginPrefMap pref_definitions);

  // Get all pref definitions (for factory and policy provider)
  const BraveOriginPrefMap& GetAll() const;

  // Get policy key to pref name mappings (for policy provider)
  const base::flat_map<std::string, std::string>& GetPolicyMappings() const;

  // Check if the singleton has been initialized
  bool IsInitialized() const;

  // Helper function to get pref info from pref definitions
  const BraveOriginPrefInfo* GetPrefInfo(const std::string& pref_name);

  BraveOriginPrefDefinitions(const BraveOriginPrefDefinitions&) = delete;
  BraveOriginPrefDefinitions& operator=(const BraveOriginPrefDefinitions&) =
      delete;

 private:
  friend base::NoDestructor<BraveOriginPrefDefinitions>;

  BraveOriginPrefDefinitions();
  ~BraveOriginPrefDefinitions();

  bool initialized_;
  BraveOriginPrefMap pref_definitions_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_PREF_DEFINITIONS_H_
