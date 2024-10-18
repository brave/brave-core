// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_rewriter/ai_rewriter_tab_helper.h"

#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ai_rewriter/ai_rewriter_button.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "chrome/browser/ui/browser_finder.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "ui/gfx/geometry/rect.h"

namespace ai_rewriter {

AIRewriterTabHelper::AIRewriterTabHelper(content::WebContents* contents)
    : content::WebContentsUserData<AIRewriterTabHelper>(*contents) {}

AIRewriterTabHelper::~AIRewriterTabHelper() = default;

// static
void AIRewriterTabHelper::Bind(
    content::RenderFrameHost* rfh,
    mojo::PendingAssociatedReceiver<mojom::AIRewriterButtonController>
        receiver) {
  auto* contents = content::WebContents::FromRenderFrameHost(rfh);
  CHECK(contents);

  // Don't create this TabHelper for non-tabbed UI.
  if (!chrome::FindBrowserWithTab(contents)) {
    return;
  }

  AIRewriterTabHelper::CreateForWebContents(contents);
  auto* tab_helper = AIRewriterTabHelper::FromWebContents(contents);
  CHECK(tab_helper);

  tab_helper->receivers_.Add(tab_helper, std::move(receiver),
                             rfh->GetGlobalFrameToken());
}

void AIRewriterTabHelper::Hide() {
  if (button_) {
    button_->Hide();
  }

  if (on_visibility_change_for_testing_) {
    on_visibility_change_for_testing_.Run();
  }
}

void AIRewriterTabHelper::Show(const gfx::Rect& rect) {
  auto* rfh =
      content::RenderFrameHost::FromFrameToken(receivers_.current_context());
  if (!rfh) {
    return;
  }

  auto* view = rfh->GetView();
  if (!view) {
    return;
  }

  if (auto* button = GetButton()) {
    auto transformed_origin =
        view->TransformPointToRootCoordSpace(rect.origin());
    button->Show(gfx::Rect(transformed_origin, rect.size()));
  }

  if (on_visibility_change_for_testing_) {
    on_visibility_change_for_testing_.Run();
  }
}

ai_rewriter::AIRewriterButton* AIRewriterTabHelper::GetButton() {
  if (!button_) {
    button_ = ai_rewriter::CreateRewriterButton(&GetWebContents());
  }

  return button_.get();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIRewriterTabHelper);

}  // namespace ai_rewriter
