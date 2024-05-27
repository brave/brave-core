// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_rewriter/ai_rewriter_ui.h"

#include <string>
#include <utility>

#include "base/notimplemented.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ai_rewriter/common/features.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "brave/components/ai_rewriter/resources/page/grit/ai_rewriter_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#endif

namespace ai_rewriter {

AIRewriterUI::AIRewriterUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui), profile_(Profile::FromWebUI(web_ui)) {
  DCHECK(profile_);
  DCHECK(brave::IsRegularProfile(profile_));
  DCHECK(features::IsAIRewriterEnabled());

  auto* source = CreateAndAddWebUIDataSource(
      web_ui, kRewriterUIHost, kAiRewriterUiGenerated,
      kAiRewriterUiGeneratedSize, IDR_REWRITER_UI_HTML);
  DCHECK(source);
}

AIRewriterUI::~AIRewriterUI() = default;

void AIRewriterUI::BindInterface(
    mojo::PendingReceiver<mojom::AIRewriterPageHandler> service) {
  receiver_.Bind(std::move(service));
}

void AIRewriterUI::Close() {
  if (dialog_closed_) {
    return;
  }

  dialog_closed_ = true;

  ConstrainedWebDialogDelegate* delegate = GetConstrainedDelegate();
  if (!delegate) {
    return;
  }

  delegate->GetWebDialogDelegate()->OnDialogClosed(std::string());
  delegate->OnDialogCloseFromWebUI();
}

void AIRewriterUI::OpenSettings() {
  NOTIMPLEMENTED();
}

void AIRewriterUI::GetInitialText(GetInitialTextCallback callback) {
  std::move(callback).Run("Message from WebUI");
}

WEB_UI_CONTROLLER_TYPE_IMPL(AIRewriterUI)

}  // namespace ai_rewriter
