// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>

#include "brave/components/containers/buildflags/buildflags.h"

#include <chrome/browser/ui/views/tabs/hovercard/hover_card_anchor_target.cc>

#if BUILDFLAG(ENABLE_CONTAINERS)
std::optional<ContainerCardData> HoverCardAnchorTarget::GetContainerCardData()
    const {
  return std::nullopt;
}
#endif
