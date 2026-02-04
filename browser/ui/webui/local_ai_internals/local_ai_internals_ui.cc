// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/local_ai_internals/local_ai_internals_ui.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/browser/local_ai/local_ai_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/resources/grit/local_ai_internals_generated.h"
#include "brave/components/local_ai/resources/grit/local_ai_internals_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace local_ai {

LocalAIInternalsPageHandler::LocalAIInternalsPageHandler(
    mojo::PendingReceiver<local_ai_internals::mojom::PageHandler> receiver,
    mojo::PendingRemote<mojom::LocalAIService> local_ai_service)
    : receiver_(this, std::move(receiver)),
      local_ai_service_(std::move(local_ai_service)) {}

LocalAIInternalsPageHandler::~LocalAIInternalsPageHandler() {
  // Must drain pending callbacks before receiver_ is destroyed, otherwise
  // the mojo responders are destroyed while the pipe is still connected.
  CancelAllPending();
}

void LocalAIInternalsPageHandler::GenerateEmbedding(
    const std::string& text,
    GenerateEmbeddingCallback callback) {
  if (!local_ai_service_.is_bound()) {
    std::move(callback).Run({});
    return;
  }
  pending_callbacks_.push_back(std::move(callback));
  if (passage_embedder_.is_bound()) {
    passage_embedder_->GenerateEmbeddings(
        text, base::BindOnce(&LocalAIInternalsPageHandler::OnEmbeddingResult,
                             weak_ptr_factory_.GetWeakPtr()));
    return;
  }
  pending_texts_.push_back(text);
  if (pending_texts_.size() == 1) {
    local_ai_service_->GetPassageEmbedder(
        base::BindOnce(&LocalAIInternalsPageHandler::OnPassageEmbedderReady,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void LocalAIInternalsPageHandler::OnPassageEmbedderReady(
    mojo::PendingRemote<mojom::PassageEmbedder> remote) {
  if (!remote.is_valid()) {
    CancelAllPending();
    return;
  }
  if (!passage_embedder_.is_bound()) {
    passage_embedder_.Bind(std::move(remote));
    passage_embedder_.set_disconnect_handler(base::BindOnce(
        &LocalAIInternalsPageHandler::OnPassageEmbedderDisconnected,
        weak_ptr_factory_.GetWeakPtr()));
  }
  auto texts = std::move(pending_texts_);
  for (const auto& text : texts) {
    passage_embedder_->GenerateEmbeddings(
        text, base::BindOnce(&LocalAIInternalsPageHandler::OnEmbeddingResult,
                             weak_ptr_factory_.GetWeakPtr()));
  }
}

void LocalAIInternalsPageHandler::OnEmbeddingResult(
    const std::vector<double>& embedding) {
  if (pending_callbacks_.empty()) {
    return;
  }
  auto callback = std::move(pending_callbacks_.front());
  pending_callbacks_.erase(pending_callbacks_.begin());
  std::move(callback).Run(embedding);

  // Release the embedder when all requests are done so the JS side
  // can detect the disconnect and call notifyWorkerIdle().
  if (pending_callbacks_.empty()) {
    passage_embedder_.reset();
  }
}

void LocalAIInternalsPageHandler::OnPassageEmbedderDisconnected() {
  passage_embedder_.reset();
  CancelAllPending();
}

void LocalAIInternalsPageHandler::CancelAllPending() {
  pending_texts_.clear();
  auto callbacks = std::move(pending_callbacks_);
  for (auto& cb : callbacks) {
    std::move(cb).Run({});
  }
}

LocalAIInternalsUI::LocalAIInternalsUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  auto* profile = Profile::FromWebUI(web_ui);

  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kLocalAIInternalsHost);

  webui::SetupWebUIDataSource(source, kLocalAiInternalsGenerated,
                              IDR_LOCAL_AI_INTERNALS_HTML);
}

LocalAIInternalsUI::~LocalAIInternalsUI() = default;

void LocalAIInternalsUI::BindInterface(
    mojo::PendingReceiver<local_ai_internals::mojom::PageHandler> receiver) {
  mojo::PendingRemote<mojom::LocalAIService> service_remote;
  if (base::FeatureList::IsEnabled(features::kLocalAIModels)) {
    auto* profile = Profile::FromWebUI(web_ui());
    service_remote = LocalAIServiceFactory::GetForProfile(profile);
  }
  page_handler_ = std::make_unique<LocalAIInternalsPageHandler>(
      std::move(receiver), std::move(service_remote));
}

WEB_UI_CONTROLLER_TYPE_IMPL(LocalAIInternalsUI)

///////////////////////////////////////////////////////////////////////////////

LocalAIInternalsUIConfig::LocalAIInternalsUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kLocalAIInternalsHost) {}

}  // namespace local_ai
