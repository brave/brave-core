// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/page_action_test_observer.h"

#include "chrome/browser/ui/page_action/page_action_model.h"

namespace page_actions {

PageActionTestObserver::PageActionTestObserver() = default;
PageActionTestObserver::~PageActionTestObserver() = default;

void PageActionTestObserver::OnPageActionModelChanged(
    const PageActionModelInterface& model) {
  visible_ = model.GetVisible();
  text_ = model.GetText();
  tooltip_text_ = model.GetTooltipText();
  ++model_change_count_;
}

}  // namespace page_actions
