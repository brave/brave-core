/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_P3A_H_
#define BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_P3A_H_

#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace sidebar {
namespace p3a {

inline constexpr char kSidebarEnabledHistogramName[] = "Brave.Sidebar.Enabled";

class SidebarP3A {
 public:
  explicit SidebarP3A(PrefService* profile_prefs);
  ~SidebarP3A();

  SidebarP3A(const SidebarP3A&) = delete;
  SidebarP3A& operator=(const SidebarP3A&) = delete;

  void RecordEnabledSetting();

 private:
  raw_ptr<PrefService> profile_prefs_;
  PrefChangeRegistrar profile_pref_change_registrar_;
};

}  // namespace p3a
}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_P3A_H_
