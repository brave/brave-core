// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_page_content_extraction_tab_helper.h"

#include <utility>

#include "brave/browser/history_embeddings/brave_page_content_extraction_service.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "components/optimization_guide/content/browser/page_content_proto_util.h"
#include "components/page_content_annotations/content/page_content_extraction_service.h"
#include "content/public/browser/global_request_id.h"
#include "content/public/browser/page.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/tokens/tokens.h"

namespace {

std::optional<optimization_guide::RenderFrameInfo> GetRenderFrameInfo(
    int child_process_id,
    blink::FrameToken frame_token) {
  content::RenderFrameHost* frame = nullptr;
  if (frame_token.Is<blink::RemoteFrameToken>()) {
    frame = content::RenderFrameHost::FromPlaceholderToken(
        child_process_id, frame_token.GetAs<blink::RemoteFrameToken>());
  } else {
    frame = content::RenderFrameHost::FromFrameToken(
        content::GlobalRenderFrameHostToken(
            child_process_id, frame_token.GetAs<blink::LocalFrameToken>()));
  }
  if (!frame) {
    return std::nullopt;
  }
  optimization_guide::RenderFrameInfo info;
  info.global_frame_token = frame->GetGlobalFrameToken();
  info.source_origin = frame->GetLastCommittedOrigin();
  info.url = frame->GetLastCommittedURL();
  return info;
}

}  // namespace

BravePageContentExtractionTabHelper::BravePageContentExtractionTabHelper(
    tabs::TabInterface& tab,
    BravePageContentExtractionService* extraction_service)
    : tabs::ContentsObservingTabFeature(tab),
      extraction_service_(extraction_service) {}

BravePageContentExtractionTabHelper::~BravePageContentExtractionTabHelper() =
    default;

void BravePageContentExtractionTabHelper::DidStopLoading() {
  if (!extraction_service_) {
    return;
  }

  auto* rfh = web_contents()->GetPrimaryMainFrame();
  if (!rfh || !rfh->IsRenderFrameLive()) {
    return;
  }

  // Skip non-HTTP(S) pages.
  const GURL& url = web_contents()->GetLastCommittedURL();
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  // Reset any in-flight extraction (new page load supersedes).
  ai_page_content_agent_.reset();
  rfh->GetRemoteInterfaces()->GetInterface(
      ai_page_content_agent_.BindNewPipeAndPassReceiver());
  ai_page_content_agent_.reset_on_disconnect();

  auto options = blink::mojom::AIPageContentOptions::New();
  ai_page_content_agent_->GetAIPageContent(
      std::move(options),
      base::BindOnce(
          &BravePageContentExtractionTabHelper::OnAIPageContentReceived,
          weak_ptr_factory_.GetWeakPtr(),
          web_contents()->GetPrimaryPage().GetWeakPtr()));
}

void BravePageContentExtractionTabHelper::OnAIPageContentReceived(
    base::WeakPtr<content::Page> page,
    blink::mojom::AIPageContentPtr content) {
  ai_page_content_agent_.reset();

  if (!page || !content || !extraction_service_) {
    return;
  }

  auto* rfh = web_contents()->GetPrimaryMainFrame();
  if (!rfh) {
    return;
  }

  // Convert the mojo AIPageContent tree to the AnnotatedPageContent protobuf
  // using upstream's conversion utility.
  auto main_frame_token = rfh->GetGlobalFrameToken();
  auto options = blink::mojom::AIPageContentOptions::New();

  optimization_guide::AIPageContentMap page_content_map;
  page_content_map[main_frame_token] = std::move(content);

  optimization_guide::FrameTokenSet frame_token_set;
  optimization_guide::AIPageContentResult result;

  auto get_render_frame_info = base::BindRepeating(&GetRenderFrameInfo);

  auto conversion_result = optimization_guide::ConvertAIPageContentToProto(
      std::move(options), main_frame_token, page_content_map,
      std::move(get_render_frame_info), frame_token_set, result);

  if (!conversion_result.has_value()) {
    DVLOG(1) << "ConvertAIPageContentToProto failed: "
             << conversion_result.error();
    return;
  }

  auto apc = base::MakeRefCounted<
      page_content_annotations::RefCountedAnnotatedPageContent>(
      std::move(result.proto));

  extraction_service_->NotifyPageContentExtracted(*page, std::move(apc));
}
