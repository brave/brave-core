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
#include "brave/browser/history_embeddings/brave_history_embeddings_status.h"
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

  // The Semantic History Search toggle only takes effect on relaunch, so a
  // change made in an earlier page load is still pending.
  bool needs_restart = false;
#if BUILDFLAG(ENABLE_LOCAL_AI)
  needs_restart =
      history_embeddings::BraveHistoryEmbeddingsStatus::GetForProfile(profile)
          ->NeedsRestart();
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

  base::DictValue update;
  update.Set("braveHistoryEmbeddingsNeedsRestart", needs_restart);
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
