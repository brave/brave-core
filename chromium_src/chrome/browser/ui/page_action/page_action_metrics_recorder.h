// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_METRICS_RECORDER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_METRICS_RECORDER_H_

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "chrome/browser/ui/page_action/page_action_metrics_recorder_interface.h"
#include "chrome/browser/ui/page_action/page_action_model_observer.h"
#include "chrome/browser/ui/page_action/page_action_triggers.h"
#include "ui/actions/action_id.h"

// Overrides the upstream class to be a no-op.

namespace tabs {
class TabInterface;
}

namespace page_actions {

struct PageActionProperties;

class PageActionMetricsRecorder : public PageActionMetricsRecorderInterface,
                                  public PageActionModelObserver {
 public:
  PageActionMetricsRecorder(tabs::TabInterface& tab_interface,
                            VisibleEphemeralPageActionsCountCallback
                                visible_ephemeral_page_actions_count_callback);

  PageActionMetricsRecorder(const PageActionMetricsRecorder&) = delete;
  PageActionMetricsRecorder operator=(const PageActionMetricsRecorder&) =
      delete;

  ~PageActionMetricsRecorder() override;

  // PageActionMetricsRecorderInterface:
  void RecordClick(actions::ActionId action_id,
                   PageActionTrigger trigger_source) override;
  void Observe(PageActionModelInterface& model,
               const PageActionProperties& properties) override;

  // PageActionModelObserver:
  void OnPageActionModelChanged(const PageActionModelInterface& model) override;
  void OnPageActionModelWillBeDeleted(
      const PageActionModelInterface& model) override;
};

}  // namespace page_actions

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_METRICS_RECORDER_H_
