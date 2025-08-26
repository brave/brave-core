/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ios/browser/ai_chat_tab_helper.h"

#include <array>
#include <cstdint>
#include <functional>
#include <ios>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/clamped_math.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_ostream_operators.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/ai_chat/ios/browser/associated_content_driver_ios.h"
#include "brave/components/ai_chat/ios/browser/page_content_extractor.h"
#include "brave/components/ai_chat/ios/browser/page_content_fetcher.h"
#include "components/strings/grit/components_strings.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

AIChatTabHelper::AIChatTabHelper(web::WebState* web_state)
    : web::WebStateObserver(),
      AssociatedContentDriverIOS(
          web_state->GetBrowserState()->GetSharedURLLoaderFactory(),
          nil),
      web_state_(web_state),
      page_content_fetcher_delegate_(
          std::make_unique<PageContentFetcher>(web_state)) {
  previous_page_title_ = web_state->GetTitle();

  web_state->AddObserver(this);

  new PageContentExtractor(web_state);
}

AIChatTabHelper::~AIChatTabHelper() {
  web_state_->RemoveObserver(this);
}

void AIChatTabHelper::GetOpenAIChatButtonNonce(
    mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback) {
  page_content_fetcher_delegate_->GetOpenAIChatButtonNonce(std::move(callback));
}

void AIChatTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (!navigation_context->HasCommitted()) {
    return;
  }

  pending_navigation_id_ = navigation_context->GetNavigationId();

  is_same_document_navigation_ = navigation_context->IsSameDocument();

  // Page loaded is only considered changing when full document changes
  if (!is_same_document_navigation_) {
    is_page_loaded_ = false;
  }

  if (!is_same_document_navigation_ ||
      previous_page_title_ != web_state->GetTitle()) {
    OnNewPage(pending_navigation_id_);
  }

  previous_page_title_ = web_state->GetTitle();
}

void AIChatTabHelper::PageLoaded(
    web::WebState* web_state,
    web::PageLoadCompletionStatus load_completion_status) {
  if (web_state->GetLastCommittedURL().is_valid()) {
    is_page_loaded_ = true;
    if (pending_get_page_content_callback_) {
      GetPageContent(std::move(pending_get_page_content_callback_), "");
    }
  }
}

void AIChatTabHelper::TitleWasSet(web::WebState* web_state) {
  MaybeSameDocumentIsNewPage();
  previous_page_title_ = web_state->GetTitle();
  SetTitle(web_state->GetTitle());
}

void AIChatTabHelper::GetPageContent(FetchPageContentCallback callback,
                                     std::string_view invalidation_token) {
  page_content_fetcher_delegate_->FetchPageContent(
      invalidation_token,
      base::BindOnce(&AIChatTabHelper::OnFetchPageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatTabHelper::GetSearchSummarizerKey(
    GetSearchSummarizerKeyCallback callback) {
  //  if (!IsBraveSearchSERP(web_state()->GetLastCommittedURL())) {
  //    std::move(callback).Run(std::nullopt);
  //    return;
  //  }

  page_content_fetcher_delegate_->GetSearchSummarizerKey(std::move(callback));
}

bool AIChatTabHelper::HasOpenAIChatPermission() const {
  return false;
}

void AIChatTabHelper::GetScreenshots(
    mojom::ConversationHandler::GetScreenshotsCallback callback) {
  std::move(callback).Run(std::nullopt);
}

void AIChatTabHelper::OnScreenshotsCaptured(
    mojom::ConversationHandler::GetScreenshotsCallback callback,
    base::expected<std::vector<std::vector<uint8_t>>, std::string>) {
  std::move(callback).Run(std::nullopt);
}

void AIChatTabHelper::OnFetchPageContentComplete(
    FetchPageContentCallback callback,
    std::string content,
    bool is_video,
    std::string invalidation_token) {
  base::TrimWhitespaceASCII(content, base::TRIM_ALL, &content);
  if (content.empty() && !is_video && !is_page_loaded_) {
    SetPendingGetContentCallback(std::move(callback));
    return;
  }
  std::move(callback).Run(std::move(content), is_video,
                          std::move(invalidation_token));
}

void AIChatTabHelper::SetPendingGetContentCallback(
    FetchPageContentCallback callback) {
  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_).Run("", false, "");
  }
  pending_get_page_content_callback_ = std::move(callback);
}

void AIChatTabHelper::OnNewPage(int64_t navigation_id) {
  AssociatedContentDriver::OnNewPage(navigation_id);
  set_url(web_state()->GetLastCommittedURL());
  SetTitle(web_state()->GetTitle());

  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_).Run("", false, "");
  }
}

void AIChatTabHelper::MaybeSameDocumentIsNewPage() {
  if (is_same_document_navigation_) {
    OnNewPage(pending_navigation_id_);
    is_same_document_navigation_ = false;
  }
}

}  // namespace ai_chat
