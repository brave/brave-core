// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_INTERNALS_ADS_INTERNALS_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_INTERNALS_ADS_INTERNALS_HANDLER_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/public/service/ads_service_callback.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

class AdsInternalsHandler : public bat_ads::mojom::AdsInternals {
 public:
  AdsInternalsHandler(brave_ads::AdsService* ads_service, PrefService& prefs);

  AdsInternalsHandler(const AdsInternalsHandler&) = delete;
  AdsInternalsHandler& operator=(const AdsInternalsHandler&) = delete;

  AdsInternalsHandler(AdsInternalsHandler&&) noexcept = delete;
  AdsInternalsHandler& operator=(AdsInternalsHandler&&) noexcept = delete;

  ~AdsInternalsHandler() override;

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

  const raw_ptr<brave_ads::AdsService> ads_service_;  // Not owned.

  const raw_ref<PrefService> prefs_;

  mojo::Receiver<bat_ads::mojom::AdsInternals> receiver_{this};

  mojo::Remote<bat_ads::mojom::AdsInternalsPage> page_;

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<AdsInternalsHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_INTERNALS_ADS_INTERNALS_HANDLER_H_
