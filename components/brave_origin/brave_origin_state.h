/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_STATE_H_

#include "base/containers/flat_set.h"

namespace brave_origin {

class BraveOriginState {
 public:
  BraveOriginState();
  static BraveOriginState* GetInstance();

  // Initialize the Brave Origin state.
  // Should be called once during browser startup.
  void Initialize();

  // Returns true if the user is considered a Brave Origin user.
  // Must be called after Initialize().
  bool IsBraveOriginUser() const;

  // Add a preference to the set of preferences controlled by BraveOrigin
  void AddBraveOriginControlledPref(const std::string& pref_name);

  // Check if a preference is controlled by BraveOrigin
  bool IsPrefControlledByBraveOrigin(const std::string& pref_name) const;

  // Clear all tracked preferences (called when user is no longer BraveOrigin)
  void ClearBraveOriginControlledPrefs();

  // Set whether the browser was managed before BraveOrigin policies were
  // applied
  void SetWasManagedBeforeBraveOrigin(bool was_managed);

  // Check if the browser was managed before BraveOrigin policies
  bool WasManagedBeforeBraveOrigin() const;

 private:
  ~BraveOriginState();

  bool is_brave_origin_user_;
  bool initialized_;
  bool was_managed_before_brave_origin_;

  // Track which preferences are controlled by BraveOrigin
  base::flat_set<std::string> brave_origin_controlled_prefs_;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_STATE_H_
