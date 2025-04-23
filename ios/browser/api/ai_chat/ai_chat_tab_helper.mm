/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/ai_chat/ai_chat_tab_helper.h"

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
#include "brave/ios/browser/api/ai_chat/associated_content_driver_ios.h"
#include "brave/ios/browser/api/ai_chat/page_content_fetcher.h"
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
}

AIChatTabHelper::~AIChatTabHelper() = default;

void AIChatTabHelper::OnInterceptedPageContentChanged() {
  MaybeSameDocumentIsNewPage();
}

void AIChatTabHelper::GetOpenAIChatButtonNonce(
    mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback) {
  page_content_fetcher_delegate_->GetOpenAIChatButtonNonce(std::move(callback));
}

void AIChatTabHelper::DidStartNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  pending_navigation_id_ = navigation_context->GetNavigationId();

  is_same_document_navigation_ = navigation_context->IsSameDocument();

  // Page loaded is only considered changing when full document changes
  if (!is_same_document_navigation_) {
    is_page_loaded_ = false;
  }

  if (!is_same_document_navigation_ || previous_page_title_ != GetPageTitle()) {
    OnNewPage(pending_navigation_id_);
  }

  previous_page_title_ = GetPageTitle();
}

void AIChatTabHelper::DidRedirectNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {}

void AIChatTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (navigation_context->GetUrl() == GetPageURL()) {
    is_page_loaded_ = true;
    if (pending_get_page_content_callback_) {
      GetPageContent(std::move(pending_get_page_content_callback_), "");
    }
  }
}

void AIChatTabHelper::TitleWasSet(web::WebState* web_state) {
  MaybeSameDocumentIsNewPage();
  previous_page_title_ = GetPageTitle();
  OnTitleChanged();
}

GURL AIChatTabHelper::GetPageURL() const {
  return web_state_->GetLastCommittedURL();
}

void AIChatTabHelper::GetPageContent(GetPageContentCallback callback,
                                     std::string_view invalidation_token) {
  page_content_fetcher_delegate_->FetchPageContent(
      invalidation_token,
      base::BindOnce(&AIChatTabHelper::OnFetchPageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

std::u16string AIChatTabHelper::GetPageTitle() const {
  return web_state_->GetTitle();
}

void AIChatTabHelper::OnNewPage(int64_t navigation_id) {
  AssociatedContentDriver::OnNewPage(navigation_id);
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

void AIChatTabHelper::GetSearchSummarizerKey(
    GetSearchSummarizerKeyCallback callback) {
  //  if (!IsBraveSearchSERP(GetPageURL())) {
  //    std::move(callback).Run(std::nullopt);
  //    return;
  //  }

  page_content_fetcher_delegate_->GetSearchSummarizerKey(std::move(callback));
}

bool AIChatTabHelper::HasOpenAIChatPermission() const {
  return false;  // No idea
}

void AIChatTabHelper::OnFetchPageContentComplete(
    GetPageContentCallback callback,
    std::string content,
    bool is_video,
    std::string invalidation_token) {
  base::TrimWhitespaceASCII(content, base::TRIM_ALL, &content);
  if (content.empty() && !is_video) {
    if (!is_page_loaded_) {
      SetPendingGetContentCallback(std::move(callback));
      return;
    }
  }
  std::move(callback).Run(std::move(content), is_video,
                          std::move(invalidation_token));
}

void AIChatTabHelper::BindPageContentExtractorReceiver(
    mojo::PendingAssociatedReceiver<mojom::PageContentExtractorHost> receiver) {
  page_content_extractor_receiver_.reset();
  page_content_extractor_receiver_.Bind(std::move(receiver));
}

void AIChatTabHelper::SetPendingGetContentCallback(
    GetPageContentCallback callback) {
  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_).Run("", false, "");
  }
  pending_get_page_content_callback_ = std::move(callback);
}

}  // namespace ai_chat
