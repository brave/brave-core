/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_origin_startup/brave_origin_startup_ui.h"

#include <utility>

#include "brave/browser/resources/brave_origin_startup/grit/brave_origin_startup_generated_map.h"
#include "brave/browser/resources/brave_origin_startup/grit/brave_origin_startup_static_resources.h"
#include "brave/browser/resources/brave_origin_startup/grit/brave_origin_startup_static_resources_map.h"
#include "brave/browser/skus/skus_service_factory.h"
#include "brave/browser/ui/webui/brave_origin_startup/brave_origin_startup_handler.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_webui_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

namespace {

void CreateAndAddDataSource(content::WebUI* web_ui, Profile* profile) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kBraveOriginStartupHost);
  webui::SetupWebUIDataSource(
      source, kBraveOriginStartupGenerated,
      IDR_BRAVE_ORIGIN_STARTUP_STATIC_BRAVE_ORIGIN_STARTUP_HTML);
  source->AddResourcePaths(kBraveOriginStartupStaticResources);

  source->AddLocalizedStrings(webui::kBraveOriginStartupStrings);
}

}  // namespace

BraveOriginStartupUI::BraveOriginStartupUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  auto* profile = Profile::FromWebUI(web_ui);
  CreateAndAddDataSource(web_ui, profile);
}

BraveOriginStartupUI::~BraveOriginStartupUI() = default;

void BraveOriginStartupUI::BindInterface(
    mojo::PendingReceiver<brave_origin::mojom::BraveOriginStartupHandler>
        receiver) {
  if (handler_) {
    handler_->BindInterface(std::move(receiver));
  } else {
    // Store the receiver to bind later when the handler is created.
    pending_receiver_ = std::move(receiver);
  }
}

void BraveOriginStartupUI::SetCallbacks(
    content::BrowserContext* sku_browser_context,
    OpenBuyWindowCallback open_buy_window_callback,
    CloseDialogCallback close_dialog_callback) {
  CreateHandler(sku_browser_context, std::move(open_buy_window_callback),
                std::move(close_dialog_callback));
}

void BraveOriginStartupUI::CreateHandler(
    content::BrowserContext* sku_browser_context,
    OpenBuyWindowCallback open_buy_window_callback,
    CloseDialogCallback close_dialog_callback) {
  // Use the regular profile's browser context for SKU service if available,
  // otherwise fall back to the WebUI's context (system profile).
  auto* browser_context = sku_browser_context
                              ? sku_browser_context
                              : web_ui()->GetWebContents()->GetBrowserContext();
  auto* local_state = g_browser_process->local_state();

  auto skus_getter = base::BindRepeating(
      &skus::SkusServiceFactory::GetForContext, browser_context);
  handler_ = std::make_unique<BraveOriginStartupHandler>(
      std::move(skus_getter), local_state, std::move(open_buy_window_callback),
      std::move(close_dialog_callback));

  if (pending_receiver_.is_valid()) {
    handler_->BindInterface(std::move(pending_receiver_));
  }
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveOriginStartupUI)
