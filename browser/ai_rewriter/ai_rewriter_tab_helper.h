// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_TAB_HELPER_H_
#define BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_TAB_HELPER_H_

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"

namespace ai_rewriter {
class AIRewriterButton;

// This class exists to direct all the RenderFrames for a WebContents to a
// single AIRewriterButton and handle positioning the button relative to the
// MainFrame.
class AIRewriterTabHelper
    : public content::WebContentsUserData<AIRewriterTabHelper>,
      public mojom::AIRewriterButtonController {
 public:
  AIRewriterTabHelper(const AIRewriterTabHelper&) = delete;
  AIRewriterTabHelper& operator=(const AIRewriterTabHelper&) = delete;
  ~AIRewriterTabHelper() override;

  static void Bind(
      content::RenderFrameHost* host,
      mojo::PendingAssociatedReceiver<mojom::AIRewriterButtonController>
          receiver);

  // mojom::AIRewriterButtonController:
  void Hide() override;
  void Show(const gfx::Rect& rect) override;

  base::WeakPtr<AIRewriterButton> button_for_testing() { return button_; }

  void SetOnVisibilityChangeForTesting(
      base::RepeatingClosure visibility_change_callback) {
    on_visibility_change_for_testing_ = visibility_change_callback;
  }

 private:
  explicit AIRewriterTabHelper(content::WebContents* contents);

  ai_rewriter::AIRewriterButton* GetButton();

  base::WeakPtr<ai_rewriter::AIRewriterButton> button_;
  mojo::AssociatedReceiverSet<mojom::AIRewriterButtonController,
                              content::GlobalRenderFrameHostToken>
      receivers_;

  base::RepeatingClosure on_visibility_change_for_testing_;

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};
}  // namespace ai_rewriter

#endif  // BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_TAB_HELPER_H_
