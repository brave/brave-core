// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_TEST_OBSERVER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_TEST_OBSERVER_H_

#include <string>

#include "chrome/browser/ui/page_action/page_action_model_observer.h"

namespace page_actions {

// Records the most recent state pushed into an action's PageActionModel.
class PageActionTestObserver : public PageActionModelObserver {
 public:
  PageActionTestObserver();
  ~PageActionTestObserver() override;

  // PageActionModelObserver:
  void OnPageActionModelChanged(const PageActionModelInterface& model) override;

  bool visible() const { return visible_; }
  const std::u16string& text() const { return text_; }
  const std::u16string& tooltip_text() const { return tooltip_text_; }
  int model_change_count() const { return model_change_count_; }

 private:
  bool visible_ = false;
  std::u16string text_;
  std::u16string tooltip_text_;
  int model_change_count_ = 0;
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_TEST_OBSERVER_H_
