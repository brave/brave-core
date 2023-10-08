// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/open_here_action.h"

#include <numeric>
#include <utility>

#include "build/build_config.h"
#include "components/grit/brave_components_strings.h"
#include "components/omnibox/browser/buildflags.h"
#include "ui/base/window_open_disposition.h"

#if defined(SUPPORT_PEDALS_VECTOR_ICONS)
#include "brave/components/vector_icons/vector_icons.h"
#endif

OpenHereAction::OpenHereAction(GURL url)
    : OmniboxAction(LabelStrings(IDS_OMNIBOX_OPEN_HERE_HINT,
                                 IDS_OMNIBOX_OPEN_HERE_HINT,
                                 IDS_ACC_OPEN_HERE_SUFFIX,
                                 IDS_ACC_OPEN_HERE_BUTTON),
                    std::move(url)) {}

OpenHereAction::~OpenHereAction() = default;

void OpenHereAction::Execute(ExecutionContext& context) const {
  context.disposition_ = WindowOpenDisposition::CURRENT_TAB;
  OpenURL(context, url_);
}

#if defined(SUPPORT_PEDALS_VECTOR_ICONS)
const gfx::VectorIcon& OpenHereAction::GetVectorIcon() const {
  return kLeoWindowTabNewIcon;
}
#endif

OmniboxActionId OpenHereAction::ActionId() const {
  return OmniboxActionId::UNKNOWN;
}
