// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_ADS_INTERNALS_ADS_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_ADS_INTERNALS_ADS_INTERNALS_UI_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/core/browser/internals/ads_internals_handler.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/ui/webui/ads_internals/ads_internals_logs_handler.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

namespace brave_rewards {
class RewardsService;
}  // namespace brave_rewards

class AdsInternalsUI;

class AdsInternalsUIConfig
    : public content::DefaultWebUIConfig<AdsInternalsUI> {
 public:
  AdsInternalsUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kAdsInternalsHost) {}

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

class AdsInternalsUI : public content::WebUIController {
 public:
  explicit AdsInternalsUI(content::WebUI* web_ui);

  AdsInternalsUI(const AdsInternalsUI&) = delete;
  AdsInternalsUI& operator=(const AdsInternalsUI&) = delete;

  ~AdsInternalsUI() override;

  void BindInterface(
      mojo::PendingReceiver<bat_ads::mojom::AdsInternals> pending_receiver);
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  void BindInterface(
      mojo::PendingReceiver<bat_ads::mojom::AdsInternalsLogs> pending_receiver);
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

 private:
  AdsInternalsHandler handler_;
  const raw_ptr<brave_rewards::RewardsService> rewards_service_;  // Not owned.
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  AdsInternalsLogsHandler logs_handler_;
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_ADS_INTERNALS_ADS_INTERNALS_UI_H_
