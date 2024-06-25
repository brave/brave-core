// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_AI_REWRITER_AI_REWRITER_BUTTON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_AI_REWRITER_AI_REWRITER_BUTTON_VIEW_H_

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/ai_rewriter/ai_rewriter_dialog_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

class Browser;

namespace ai_rewriter {

// A button which shows up when text is selected. The button is owned by a
// widget which is parented to the browser window.
// It is created when:
// 1. A focused element has more than two characters of text selected

// The Widget will be closed (destroying this view) when:
// 1. The tab is destroyed
// 2. The tab is reparented
// 3. The tab navigates
class AIRewriterButtonView : public views::WidgetDelegateView,
                             public content::WebContentsObserver,
                             public TabStripModelObserver {
  METADATA_HEADER(AIRewriterButtonView, views::WidgetDelegateView)

 public:
  AIRewriterButtonView(const AIRewriterButtonView&) = delete;
  AIRewriterButtonView& operator=(const AIRewriterButtonView&) = delete;
  ~AIRewriterButtonView() override;

  // Creates the AIRewriterButtonView for |contents| if it exists in a tab.
  static base::WeakPtr<AIRewriterButtonView> MaybeCreateButton(
      content::WebContents* contents);

  void Show(const gfx::Rect& rect);
  void Hide();

  AIRewriterDialogDelegate* OpenDialog();

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;
  void PrimaryPageChanged(content::Page& page) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

 private:
  AIRewriterButtonView(Browser* browser, content::WebContents* contents);

  base::ScopedObservation<TabStripModel, AIRewriterButtonView>
      tab_strip_observation_{this};
  base::WeakPtrFactory<AIRewriterButtonView> weak_ptr_factory_{this};
};

BEGIN_VIEW_BUILDER(/*no export*/,
                   AIRewriterButtonView,
                   views::WidgetDelegateView)
END_VIEW_BUILDER

}  // namespace ai_rewriter

DEFINE_VIEW_BUILDER(/*no export*/, ai_rewriter::AIRewriterButtonView)

#endif  // BRAVE_BROWSER_UI_VIEWS_AI_REWRITER_AI_REWRITER_BUTTON_VIEW_H_
