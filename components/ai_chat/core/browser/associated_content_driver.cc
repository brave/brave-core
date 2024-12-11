/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/associated_content_driver.h"

#include <ios>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/strings/strcat.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/ai_chat/core/browser/brave_search_responses.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/base/url_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

net::NetworkTrafficAnnotationTag
GetSearchQuerySummaryNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation(
      "ai_chat_associated_content_driver",
      R"(
      semantics {
        sender: "Brave Leo AI Chat"
        description:
          "This sender is used to get search query summary from Brave search."
        trigger:
          "Triggered by uses of Brave Leo AI Chat on Brave Search SERP."
        data:
          "User's search query and the corresponding summary."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

AssociatedContentDriver::AssociatedContentDriver(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {}

AssociatedContentDriver::~AssociatedContentDriver() {
  for (auto& conversation : associated_conversations_) {
    if (conversation) {
      conversation->OnAssociatedContentDestroyed(cached_text_content_,
                                                 is_video_);
    }
  }
}

void AssociatedContentDriver::AddRelatedConversation(
    ConversationHandler* conversation) {
  associated_conversations_.insert(conversation);
}

void AssociatedContentDriver::OnRelatedConversationDisassociated(
    ConversationHandler* conversation) {
  associated_conversations_.erase(conversation);
}

void AssociatedContentDriver::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AssociatedContentDriver::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

int AssociatedContentDriver::GetContentId() const {
  return current_navigation_id_;
}

GURL AssociatedContentDriver::GetURL() const {
  return GetPageURL();
}

std::u16string AssociatedContentDriver::GetTitle() const {
  return GetPageTitle();
}

void AssociatedContentDriver::GetContent(
    ConversationHandler::GetPageContentCallback callback) {
  // Determine whether we're adding our callback to the queue or the
  // we need to call GetPageContent.
  bool is_page_text_fetch_in_progress =
      (on_page_text_fetch_complete_ != nullptr);
  if (!is_page_text_fetch_in_progress) {
    on_page_text_fetch_complete_ = std::make_unique<base::OneShotEvent>();
  }
  // Register callback to fire when the event is complete
  auto handle_existing_fetch_complete = base::BindOnce(
      &AssociatedContentDriver::OnExistingGeneratePageContentComplete,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback),
      current_navigation_id_);
  on_page_text_fetch_complete_->Post(FROM_HERE,
                                     std::move(handle_existing_fetch_complete));

  if (is_page_text_fetch_in_progress) {
    DVLOG(1) << "A page content fetch is in progress, waiting for the existing "
                "operation to complete";
    return;
  }
  // No operation already in progress, so fetch the page content and signal the
  // event when done.
  GetPageContent(
      base::BindOnce(&AssociatedContentDriver::OnGeneratePageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), current_navigation_id_),
      content_invalidation_token_);
}

void AssociatedContentDriver::OnExistingGeneratePageContentComplete(
    ConversationHandler::GetPageContentCallback callback,
    int64_t navigation_id) {
  if (navigation_id != current_navigation_id_) {
    return;
  }
  std::move(callback).Run(cached_text_content_, is_video_,
                          content_invalidation_token_);
}

void AssociatedContentDriver::OnGeneratePageContentComplete(
    int64_t navigation_id,
    std::string contents_text,
    bool is_video,
    std::string invalidation_token) {
  DVLOG(1) << "OnGeneratePageContentComplete";
  DVLOG(4) << "Contents(is_video=" << is_video
           << ", invalidation_token=" << invalidation_token
           << "): " << contents_text;
  if (navigation_id != current_navigation_id_) {
    return;
  }

  // If invalidation token matches existing token, then
  // content was not re-fetched and we can use our existing cache.
  if (invalidation_token.empty() ||
      (invalidation_token != content_invalidation_token_)) {
    is_video_ = is_video;
    // Cache page content on instance so we don't always have to re-fetch
    // if the content fetcher knows the content won't have changed and the fetch
    // operation is expensive (e.g. network).
    cached_text_content_ = contents_text;
    content_invalidation_token_ = invalidation_token;
    if (contents_text.empty()) {
      DVLOG(1) << __func__ << ": No data";
    }
  }

  on_page_text_fetch_complete_->Signal();
  on_page_text_fetch_complete_ = nullptr;
}

std::string_view AssociatedContentDriver::GetCachedTextContent() {
  return cached_text_content_;
}

bool AssociatedContentDriver::GetCachedIsVideo() {
  return is_video_;
}

void AssociatedContentDriver::GetStagedEntriesFromContent(
    ConversationHandler::GetStagedEntriesCallback callback) {
  // At the moment we only know about staged entries from:
  // - Brave Search results page
  if (!IsBraveSearchSERP(GetPageURL())) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  GetSearchSummarizerKey(
      base::BindOnce(&AssociatedContentDriver::OnSearchSummarizerKeyFetched,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     current_navigation_id_));
}

void AssociatedContentDriver::OnSearchSummarizerKeyFetched(
    ConversationHandler::GetStagedEntriesCallback callback,
    int64_t navigation_id,
    const std::optional<std::string>& key) {
  if (!key || key->empty() || navigation_id != current_navigation_id_) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  if (!api_request_helper_) {
    api_request_helper_ =
        std::make_unique<api_request_helper::APIRequestHelper>(
            GetSearchQuerySummaryNetworkTrafficAnnotationTag(),
            url_loader_factory_);
  }

  // https://search.brave.com/api/chatllm/raw_data?key=<key>
  GURL base_url(
      base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                    brave_domains::GetServicesDomain(kBraveSearchURLPrefix),
                    "/api/chatllm/raw_data"}));
  CHECK(base_url.is_valid());
  GURL url = net::AppendQueryParameter(base_url, "key", *key);

  api_request_helper_->Request(
      "GET", url, "", "application/json",
      base::BindOnce(&AssociatedContentDriver::OnSearchQuerySummaryFetched,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     navigation_id),
      {}, {});
}

void AssociatedContentDriver::OnSearchQuerySummaryFetched(
    ConversationHandler::GetStagedEntriesCallback callback,
    int64_t navigation_id,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode() || navigation_id != current_navigation_id_) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  auto entries = ParseSearchQuerySummaryResponse(result.value_body());
  if (!entries) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  std::move(callback).Run(entries);
}

// static
std::optional<std::vector<SearchQuerySummary>>
AssociatedContentDriver::ParseSearchQuerySummaryResponse(
    const base::Value& value) {
  auto search_query_response =
      brave_search_responses::QuerySummaryResponse::FromValue(value);
  if (!search_query_response || search_query_response->conversation.empty()) {
    return std::nullopt;
  }

  std::vector<SearchQuerySummary> entries;
  for (const auto& entry : search_query_response->conversation) {
    if (entry.answer.empty()) {
      continue;
    }

    // Only support one answer for each query for now.
    entries.push_back(SearchQuerySummary(entry.query, entry.answer[0].text));
  }

  return entries;
}

void AssociatedContentDriver::OnFaviconImageDataChanged() {
  for (auto& conversation : associated_conversations_) {
    conversation->OnFaviconImageDataChanged();
  }
}

void AssociatedContentDriver::OnTitleChanged() {
  for (auto& conversation : associated_conversations_) {
    conversation->OnAssociatedContentTitleChanged();
  }
}

void AssociatedContentDriver::OnNewPage(int64_t navigation_id) {
  // Tell the associated_conversations_ that we're breaking up
  for (auto& conversation : associated_conversations_) {
    conversation->OnAssociatedContentDestroyed(cached_text_content_, is_video_);
  }
  // Tell the observer how to find the next conversation
  for (auto& observer : observers_) {
    observer.OnAssociatedContentNavigated(navigation_id);
  }

  // Reset state for next navigated Page
  associated_conversations_.clear();
  current_navigation_id_ = navigation_id;
  cached_text_content_.clear();
  content_invalidation_token_.clear();
  is_video_ = false;
  api_request_helper_.reset();
  ConversationHandler::AssociatedContentDelegate::OnNewPage(navigation_id);
}

}  // namespace ai_chat
