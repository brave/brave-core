// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_rewriter/ai_rewriter_ui.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/notimplemented.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/ai_rewriter/ai_rewriter_dialog_delegate.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_rewriter/common/features.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "brave/components/ai_rewriter/resources/page/grit/ai_rewriter_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#endif

namespace ai_rewriter {

AIRewriterUI::AIRewriterUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui), profile_(Profile::FromWebUI(web_ui)) {
  DCHECK(profile_);
  DCHECK(profile_->IsRegularProfile());
  DCHECK(features::IsAIRewriterEnabled());

  auto* source = CreateAndAddWebUIDataSource(
      web_ui, kRewriterUIHost, kAiRewriterUiGenerated,
      kAiRewriterUiGeneratedSize, IDR_REWRITER_UI_HTML);
  DCHECK(source);

  for (const auto& str : ai_chat::GetLocalizedStrings()) {
    source->AddString(str.name,
                      brave_l10n::GetLocalizedResourceUTF16String(str.id));
  }

  ai_engine_ = ai_chat::AIChatServiceFactory::GetForBrowserContext(profile_)
                   ->GetDefaultAIEngine();
}

AIRewriterUI::~AIRewriterUI() = default;

void AIRewriterUI::BindInterface(
    mojo::PendingReceiver<mojom::AIRewriterPageHandler> service) {
  receiver_.Bind(std::move(service));
}

void AIRewriterUI::SetPage(mojo::PendingRemote<mojom::AIRewriterPage> page) {
  page_.reset();
  page_.Bind(std::move(page));
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
  std::move(callback).Run(initial_text_);
}

void AIRewriterUI::RewriteText(const std::string& text,
                               ai_chat::mojom::ActionType action,
                               const std::string& instructions,
                               RewriteTextCallback callback) {
  // Stop any pending rewrite requests.
  weak_ptr_factory_.InvalidateWeakPtrs();

  auto* target = GetTargetContents();
  if (!target) {
    std::move(callback).Run();
    return;
  }

  // TODO(petemill): Pass |action| in addition to |instructions| when supported
  // by engine.
  ai_engine_->GenerateRewriteSuggestion(
      text, instructions,
      ai_chat::BindParseRewriteReceivedData(
          base::BindRepeating(&AIRewriterUI::OnRewriteSuggestionGenerated,
                              weak_ptr_factory_.GetWeakPtr())),
      base::BindOnce(
          [](RewriteTextCallback callback,
             ai_chat::EngineConsumer::GenerationResult result) {
            std::move(callback).Run();
          },
          std::move(callback)));
}

void AIRewriterUI::InsertTextAndClose(const std::string& text,
                                      InsertTextAndCloseCallback callback) {
  if (auto* contents = GetTargetContents()) {
    contents->Replace(base::UTF8ToUTF16(text));
  }
  std::move(callback).Run();
  Close();
}

AIRewriterDialogDelegate* AIRewriterUI::GetDialogDelegate() {
  auto* delegate = GetConstrainedDelegate();
  if (!delegate) {
    return nullptr;
  }
  auto* web_delegate = delegate->GetWebDialogDelegate();
  if (!web_delegate) {
    return nullptr;
  }

  return static_cast<AIRewriterDialogDelegate*>(
      GetConstrainedDelegate()->GetWebDialogDelegate());
}

content::WebContents* AIRewriterUI::GetTargetContents() {
  auto* delegate = GetDialogDelegate();

  // If we aren't being shown in a Dialog, then we're in a tab.
  if (!delegate) {
    return web_ui()->GetWebContents();
  }

  return delegate->web_contents();
}

void AIRewriterUI::GetActionMenuList(GetActionMenuListCallback callback) {
  std::move(callback).Run(ai_chat::GetActionMenuList());
}

void AIRewriterUI::OnRewriteSuggestionGenerated(const std::string& suggestion) {
  if (page_.is_bound()) {
    page_->OnUpdatedGeneratedText(suggestion);
  }
}

WEB_UI_CONTROLLER_TYPE_IMPL(AIRewriterUI)

}  // namespace ai_rewriter
