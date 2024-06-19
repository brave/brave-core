// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_AI_REWRITER_AI_REWRITER_DIALOG_DELEGATE_H_
#define BRAVE_BROWSER_UI_AI_REWRITER_AI_REWRITER_DIALOG_DELEGATE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace content {
class WebContents;
class NavigationHandle;
struct FocusedNodeDetails;
}  // namespace content

namespace views {
class Widget;
}

namespace ai_rewriter {

class AIRewriterUI;

class AIRewriterDialogDelegate : public ui::WebDialogDelegate,
                                 public content::WebContentsObserver {
 public:
  AIRewriterDialogDelegate(const AIRewriterDialogDelegate&) = delete;
  AIRewriterDialogDelegate& operator=(const AIRewriterDialogDelegate&) = delete;
  ~AIRewriterDialogDelegate() override;

  static AIRewriterDialogDelegate* Show(content::WebContents* contents,
                                        std::string initial_text);

  void CloseDialog();
  content::WebContents* GetDialogWebContents();

  void UpdateBounds();

  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* handle) override;
  void OnFocusChangedInPage(content::FocusedNodeDetails* details) override;

  views::Widget* widget_for_testing() { return widget_for_testing_; }
  AIRewriterUI* GetRewriterUIForTesting();

 private:
  class DialogContentsObserver;
  class DialogPositioner;

  explicit AIRewriterDialogDelegate(content::WebContents* contents);

  void ResetDialogObserver();
  void ShowDialog();

  AIRewriterUI* GetRewriterUI();

  base::WeakPtr<content::WebContents> target_contents_;
  std::unique_ptr<DialogContentsObserver> dialog_observer_;
  std::unique_ptr<DialogPositioner> positioner_;

  raw_ptr<views::Widget> widget_for_testing_;
};

}  // namespace ai_rewriter

#endif  // BRAVE_BROWSER_UI_AI_REWRITER_AI_REWRITER_DIALOG_DELEGATE_H_
