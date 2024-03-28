/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_shields/cookie_list_opt_in_page_handler.h"

#include <utility>

#include "base/metrics/histogram_functions.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"

CookieListOptInPageHandler::CookieListOptInPageHandler(
    mojo::PendingReceiver<brave_shields::mojom::CookieListOptInPageHandler>
        receiver,
    base::WeakPtr<TopChromeWebUIController::Embedder> embedder,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      embedder_(embedder),
      profile_(profile) {
  DCHECK(embedder_);
  DCHECK(profile_);
}

CookieListOptInPageHandler::~CookieListOptInPageHandler() = default;

void CookieListOptInPageHandler::ShowUI() {
  base::UmaHistogramExactLinear(brave_shields::kCookieListPromptHistogram, 1,
                                4);
  if (embedder_) {
    embedder_->ShowUI();
  }
}

void CookieListOptInPageHandler::CloseUI() {
  if (embedder_) {
    embedder_->CloseUI();
  }
}

void CookieListOptInPageHandler::EnableFilter() {
  g_brave_browser_process->ad_block_service()
      ->component_service_manager()
      ->EnableFilterList(brave_shields::kCookieListUuid, true);
}

void CookieListOptInPageHandler::OnUINoClicked() {
  base::UmaHistogramExactLinear(brave_shields::kCookieListPromptHistogram, 2,
                                4);
}

void CookieListOptInPageHandler::OnUIYesClicked() {
  base::UmaHistogramExactLinear(brave_shields::kCookieListPromptHistogram, 3,
                                4);
}
