// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_AI_REWRITER_AI_REWRITER_DIALOG_DELEGATE_H_
#define BRAVE_BROWSER_UI_AI_REWRITER_AI_REWRITER_DIALOG_DELEGATE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace content {
class WebContents;
class NavigationHandle;
}  // namespace content

namespace ai_rewriter {

class AIRewriterDialogDelegate : public ui::WebDialogDelegate,
                                 public content::WebContentsObserver {
 public:
  AIRewriterDialogDelegate(const AIRewriterDialogDelegate&) = delete;
  AIRewriterDialogDelegate& operator=(const AIRewriterDialogDelegate&) = delete;
  ~AIRewriterDialogDelegate() override;

  static AIRewriterDialogDelegate* Show(content::WebContents* contents);

  void CloseDialog();
  content::WebContents* GetDialogWebContents();

  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* handle) override;

 private:
  class DialogContentsObserver;

  explicit AIRewriterDialogDelegate(content::WebContents* contents);

  void ResetDialogObserver();
  void ShowDialog();

  base::WeakPtr<content::WebContents> target_contents_;
  std::unique_ptr<DialogContentsObserver> dialog_observer_;
};

}  // namespace ai_rewriter

#endif  // BRAVE_BROWSER_UI_AI_REWRITER_AI_REWRITER_DIALOG_DELEGATE_H_
