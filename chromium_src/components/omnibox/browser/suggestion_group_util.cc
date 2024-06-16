// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/omnibox/browser/suggestion_group_util.h"

#include "brave/components/commander/common/buildflags/buildflags.h"
#include "components/grit/brave_components_strings.h"

#define BuildDefaultGroups BuildDefaultGroups_ChromiumImpl
#include "src/components/omnibox/browser/suggestion_group_util.cc"
#undef BuildDefaultGroups

namespace omnibox {
const GroupConfigMap& BuildDefaultGroups() {
  if (g_default_groups.Get().empty()) {
    BuildDefaultGroups_ChromiumImpl();

#if BUILDFLAG(ENABLE_COMMANDER)
    g_default_groups.Get()[GROUP_OTHER_NAVS] = CreateGroup(
        SECTION_OTHER_NAVS,
        GroupConfig::RenderType::GroupConfig_RenderType_DEFAULT_VERTICAL,
        IDS_IDC_COMMANDER);
#endif
  }

  return g_default_groups.Get();
}
}  // namespace omnibox
