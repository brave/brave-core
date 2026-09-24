/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/history/brave_history_ui.h"

#include <utility>

#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/page_not_available_for_guest/page_not_available_for_guest_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/web_ui_data_source.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/browser/ui/webui/history/brave_history_embeddings_page_handler.h"
#endif

// Matches HistoryUIConfig::CreateWebUIController, which this config replaces.
std::unique_ptr<content::WebUIController>
BraveHistoryUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                            const GURL& url) {
  if (Profile::FromWebUI(web_ui)->IsGuestSession()) {
    return std::make_unique<PageNotAvailableForGuestUI>(
        web_ui, chrome::kChromeUIHistoryHost);
  }
  return std::make_unique<BraveHistoryUI>(web_ui);
}

BraveHistoryUI::BraveHistoryUI(content::WebUI* web_ui) : HistoryUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  base::DictValue update;
#if BUILDFLAG(ENABLE_LOCAL_AI)
  // Matches what the page handler pushes on a pref change, so a page loading
  // after the toggle moved agrees with one already open.
  auto state = BraveHistoryEmbeddingsPageHandler::GetState(profile);
  update.Set("braveHistoryEmbeddingsEnabled", state.enabled);
  // Upstream gates its calls into the embeddings service on this.
  update.Set("enableHistoryEmbeddings", state.search_enabled());
  update.Set("braveHistoryEmbeddingsNeedsRestart", state.needs_restart);
#else
  update.Set("braveHistoryEmbeddingsEnabled", false);
  update.Set("braveHistoryEmbeddingsNeedsRestart", false);
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)
  content::WebUIDataSource::Update(profile, chrome::kChromeUIHistoryHost,
                                   std::move(update));
}

BraveHistoryUI::~BraveHistoryUI() = default;

#if BUILDFLAG(ENABLE_LOCAL_AI)
void BraveHistoryUI::BindInterface(
    mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandlerFactory>
        receiver) {
  page_handler_factory_receiver_.reset();
  page_handler_factory_receiver_.Bind(std::move(receiver));
}

void BraveHistoryUI::CreateInterfacePageHandler(
    mojo::PendingRemote<brave_history_embeddings::mojom::Page> page,
    mojo::PendingReceiver<brave_history_embeddings::mojom::PageHandler>
        receiver) {
  page_handler_ = std::make_unique<BraveHistoryEmbeddingsPageHandler>(
      std::move(receiver), std::move(page), Profile::FromWebUI(web_ui()),
      g_browser_process->local_state());
}
#endif
