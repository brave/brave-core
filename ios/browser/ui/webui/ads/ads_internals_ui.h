// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_ADS_ADS_INTERNALS_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_ADS_ADS_INTERNALS_UI_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"

// MARK: - BASED ON: brave/browser/ui/webui/ads_internals/ads_internals_ui.h

namespace brave_ads {
class AdsServiceImplIOS;
}  // namespace brave_ads

// TODO(https://github.com/brave/brave-browser/issues/42303): Unify this
// implementation with the original AdsInternalsUI class for Desktop and
// Android.
class AdsInternalsUI : public web::WebUIIOSController,
                       bat_ads::mojom::AdsInternals {
 public:
  AdsInternalsUI(web::WebUIIOS* const web_ui, const GURL& url);

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
  void ClearAdsData(ClearAdsDataCallback callback) override;
  void CreateAdsInternalsPageHandler(
      mojo::PendingRemote<bat_ads::mojom::AdsInternalsPage> page) override;

  void GetInternalsCallback(GetAdsInternalsCallback callback,
                            std::optional<base::Value::List> value);

  void OnPrefChanged(const std::string& path);
  void UpdateBraveRewardsEnabled();

  const raw_ptr<brave_ads::AdsServiceImplIOS> ads_service_ =
      nullptr;  // Not owned.

  const raw_ptr<PrefService> prefs_ = nullptr;  // Not owned.

  mojo::Receiver<bat_ads::mojom::AdsInternals> receiver_{this};

  mojo::Remote<bat_ads::mojom::AdsInternalsPage> page_;

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<AdsInternalsUI> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_ADS_ADS_INTERNALS_UI_H_
