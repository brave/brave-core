// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_ADS_INTERNALS_ADS_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_ADS_INTERNALS_ADS_INTERNALS_UI_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/public/service/ads_service_callback.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

class AdsInternalsUI : public content::WebUIController,
                       bat_ads::mojom::AdsInternals {
 public:
  AdsInternalsUI(content::WebUI* const web_ui,
                 const std::string& name,
                 brave_ads::AdsService* ads_service,
                 PrefService* prefs);

  AdsInternalsUI(const AdsInternalsUI&) = delete;
  AdsInternalsUI& operator=(const AdsInternalsUI&) = delete;

  AdsInternalsUI(AdsInternalsUI&&) noexcept = delete;
  AdsInternalsUI& operator=(AdsInternalsUI&&) noexcept = delete;

  ~AdsInternalsUI() override;

  void BindInterface(
      mojo::PendingReceiver<bat_ads::mojom::AdsInternals> pending_receiver);

 private:
  // bat_ads::mojom::AdsInternals:
  void GetAdsInternals(GetAdsInternalsCallback callback) override;
  void ClearAdsData(brave_ads::ClearDataCallback callback) override;
  void CreateAdsInternalsPageHandler(
      mojo::PendingRemote<bat_ads::mojom::AdsInternalsPage> page) override;

  void GetInternalsCallback(GetAdsInternalsCallback callback,
                            std::optional<base::Value::List> value);

  void OnPrefChanged(const std::string& path);
  void UpdateBraveRewardsEnabled();

  const raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;  // Not owned.

  const raw_ptr<PrefService> prefs_ = nullptr;  // Not owned.

  mojo::Receiver<bat_ads::mojom::AdsInternals> receiver_{this};

  mojo::Remote<bat_ads::mojom::AdsInternalsPage> page_;

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<AdsInternalsUI> weak_ptr_factory_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_ADS_INTERNALS_ADS_INTERNALS_UI_H_
