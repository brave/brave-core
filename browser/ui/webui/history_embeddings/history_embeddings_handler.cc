// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/webui/history_embeddings/history_embeddings_handler.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/i18n/time_formatting.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/history_embeddings/history_embeddings_service_factory.h"
#include "brave/components/history_embeddings/content/history_embeddings_service.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/history/history_ui.h"
#include "components/history_clusters/core/history_clusters_util.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/webui/web_ui_util.h"
#include "url/gurl.h"

namespace brave_history_embeddings {

namespace {

// The number of best matching items to take from the search.
constexpr size_t kSearchResultItemCount = 3;

}  // namespace

HistoryEmbeddingsHandler::HistoryEmbeddingsHandler(
    mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandler>
        pending_page_handler,
    mojo::PendingRemote<brave_history_embeddings::mojom::Page> pending_page,
    base::WeakPtr<Profile> profile)
    : page_handler_(this, std::move(pending_page_handler)),
      page_(std::move(pending_page)),
      profile_(std::move(profile)) {}

HistoryEmbeddingsHandler::~HistoryEmbeddingsHandler() = default;

void HistoryEmbeddingsHandler::Search(
    brave_history_embeddings::mojom::SearchQueryPtr query) {
  if (!profile_) {
    OnReceivedSearchResult({});
    return;
  }

  brave_history_embeddings::HistoryEmbeddingsService* service =
      HistoryEmbeddingsServiceFactory::GetForProfile(profile_.get());
  // The page handler factory only binds when the service was built.
  CHECK(service);
  last_result_ = service->Search(
      &last_result_, query->query, /*time_range_start=*/std::nullopt,
      kSearchResultItemCount, /*url_id_filter=*/{},
      base::BindRepeating(&HistoryEmbeddingsHandler::OnReceivedSearchResult,
                          weak_ptr_factory_.GetWeakPtr()));
  VLOG(3) << "HistoryEmbeddingsHandler::Search started for '"
          << last_result_.query << "'";
}

void HistoryEmbeddingsHandler::PublishResultToPage(
    const brave_history_embeddings::SearchResult& native_search_result) {
  auto mojom_search_result =
      brave_history_embeddings::mojom::SearchResult::New();
  mojom_search_result->query = native_search_result.query;

  for (const brave_history_embeddings::ScoredUrlRow& scored_url_row :
       native_search_result.scored_url_rows) {
    auto item = brave_history_embeddings::mojom::SearchResultItem::New();
    item->title = base::UTF16ToUTF8(scored_url_row.row.title());
    item->url = scored_url_row.row.url();
    item->short_date_time = base::UTF16ToUTF8(
        base::TimeFormatShortDate(scored_url_row.row.last_visit()));
    item->last_url_visit_timestamp =
        scored_url_row.row.last_visit().InMillisecondsFSinceUnixEpoch();
    item->url_for_display =
        base::UTF16ToUTF8(history_clusters::ComputeURLForDisplay(
            scored_url_row.row.url(),
            brave_history_embeddings::GetFeatureParameters()
                .trim_after_host_in_results));
    mojom_search_result->items.push_back(std::move(item));
  }
  page_->SearchResultChanged(std::move(mojom_search_result));
}

void HistoryEmbeddingsHandler::OnReceivedSearchResult(
    brave_history_embeddings::SearchResult native_search_result) {
  // Ignore results for outdated queries; only update results for current query.
  if (!native_search_result.IsContinuationOf(last_result_)) {
    VLOG(3) << "HistoryEmbeddingsHandler::OnReceivedSearchResult for '"
            << native_search_result.query
            << "' (dropped as not a continuation of '" << last_result_.query
            << "')";
    return;
  }
  VLOG(3) << "HistoryEmbeddingsHandler::OnReceivedSearchResult for '"
          << native_search_result.query << "' (published as continuation)";
  last_result_ = std::move(native_search_result);
  PublishResultToPage(last_result_);
}

HistoryEmbeddingsPageHandlerFactory::HistoryEmbeddingsPageHandlerFactory(
    content::RenderFrameHost* render_frame_host)
    : content::DocumentUserData<HistoryEmbeddingsPageHandlerFactory>(
          render_frame_host) {}

HistoryEmbeddingsPageHandlerFactory::~HistoryEmbeddingsPageHandlerFactory() =
    default;

// static
void HistoryEmbeddingsPageHandlerFactory::BindForFrame(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingReceiver<mojom::PageHandlerFactory> receiver) {
  content::WebUI* web_ui = render_frame_host->GetWebUI();
  if (!web_ui || !web_ui->GetController()->GetAs<HistoryUI>()) {
    return;
  }
  if (!HistoryEmbeddingsServiceFactory::GetForProfile(
          Profile::FromWebUI(web_ui))) {
    return;
  }
  auto* factory = GetOrCreateForCurrentDocument(render_frame_host);
  factory->receiver_.reset();
  factory->receiver_.Bind(std::move(receiver));
}

void HistoryEmbeddingsPageHandlerFactory::CreatePageHandler(
    mojo::PendingRemote<mojom::Page> page,
    mojo::PendingReceiver<mojom::PageHandler> handler) {
  Profile* profile =
      Profile::FromBrowserContext(render_frame_host().GetBrowserContext());
  handler_ = std::make_unique<HistoryEmbeddingsHandler>(
      std::move(handler), std::move(page), profile->GetWeakPtr());
}

DOCUMENT_USER_DATA_KEY_IMPL(HistoryEmbeddingsPageHandlerFactory);

}  // namespace brave_history_embeddings

// Forward declared by the chromium_src override of
// history_ui_platform_handlers_desktop.cc, which builds brave://history's data
// source.
void BravePopulateHistoryEmbeddingsSource(content::WebUI* web_ui,
                                          content::WebUIDataSource* source) {
  source->AddBoolean(
      "enableBraveHistoryEmbeddings",
      !!brave_history_embeddings::HistoryEmbeddingsServiceFactory::
           GetForProfile(Profile::FromWebUI(web_ui)));
  static constexpr webui::LocalizedString kStrings[] = {
      {"braveHistoryEmbeddingsDisclaimer",
       IDS_BRAVE_HISTORY_EMBEDDINGS_DISCLAIMER},
      {"braveHistoryEmbeddingsFooter", IDS_BRAVE_HISTORY_EMBEDDINGS_FOOTER},
      {"braveHistoryEmbeddingsHeading", IDS_BRAVE_HISTORY_EMBEDDINGS_HEADING},
      {"braveHistoryEmbeddingsHeadingLoading",
       IDS_BRAVE_HISTORY_EMBEDDINGS_HEADING_LOADING},
      {"braveHistoryEmbeddingsMatch",
       IDS_BRAVE_HISTORY_EMBEDDINGS_MATCH_RESULT},
      {"braveHistoryEmbeddingsMatches",
       IDS_BRAVE_HISTORY_EMBEDDINGS_MATCH_RESULTS},
  };
  source->AddLocalizedStrings(kStrings);
  source->AddInteger("braveHistoryEmbeddingsSearchMinimumWordCount",
                     brave_history_embeddings::GetFeatureParameters()
                         .search_query_minimum_word_count);
}
