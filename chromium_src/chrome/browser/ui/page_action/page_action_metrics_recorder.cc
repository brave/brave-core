// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/page_action/page_action_metrics_recorder.h"

#include "chrome/browser/ui/page_action/page_action_properties_provider.h"

// Overrides the upstream class to be a no-op.

namespace page_actions {

PageActionMetricsRecorder::PageActionMetricsRecorder(
    tabs::TabInterface& tab_interface,
    VisibleEphemeralPageActionsCountCallback
        visible_ephemeral_page_actions_count_callback) {}

PageActionMetricsRecorder::~PageActionMetricsRecorder() = default;

void PageActionMetricsRecorder::RecordClick(actions::ActionId action_id,
                                            PageActionTrigger trigger_source) {}

void PageActionMetricsRecorder::Observe(
    PageActionModelInterface& model,
    const PageActionProperties& properties) {}

void PageActionMetricsRecorder::OnPageActionModelChanged(
    const PageActionModelInterface& model) {}

void PageActionMetricsRecorder::OnPageActionModelWillBeDeleted(
    const PageActionModelInterface& model) {}

}  // namespace page_actions
