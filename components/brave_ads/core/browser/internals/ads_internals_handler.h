// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_INTERNALS_ADS_INTERNALS_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_INTERNALS_ADS_INTERNALS_HANDLER_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_callback.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

class PrefService;

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace variations {
class VariationsService;
}  // namespace variations

class AdsInternalsHandler final : public bat_ads::mojom::AdsInternals {
 public:
  // Avoids a direct dependency on `NTPBackgroundImagesService`, which would
  // introduce a build dependency cycle back to this component (see
  // `//brave/components/ntp_background_images/browser` -> ... ->
  // `//brave/components/brave_ads/browser`).
  using GetComponentIdCallback =
      base::RepeatingCallback<std::optional<std::string>()>;

  // Same rationale as `GetComponentIdCallback` above.
  using GetIsSponsoredImagesLoadedCallback = base::RepeatingCallback<bool()>;

  AdsInternalsHandler(
      brave_ads::AdsService* ads_service,
      PrefService& prefs,
      variations::VariationsService* variations_service,
      GetComponentIdCallback get_ntp_sponsored_images_component_id_callback,
      GetComponentIdCallback get_country_resource_component_id_callback,
      GetComponentIdCallback get_language_resource_component_id_callback,
      GetIsSponsoredImagesLoadedCallback
          get_is_sponsored_images_loaded_callback,
      GetComponentIdCallback
          get_ntp_sponsored_images_manifest_version_callback);

  AdsInternalsHandler(const AdsInternalsHandler&) = delete;
  AdsInternalsHandler& operator=(const AdsInternalsHandler&) = delete;

  ~AdsInternalsHandler() override;

  void BindInterface(mojo::PendingReceiver<bat_ads::mojom::AdsInternals>
                         ads_internals_pending_receiver);

 private:
  // bat_ads::mojom::AdsInternals:
  void CreateAdsInternalsPageHandler(
      mojo::PendingRemote<bat_ads::mojom::AdsInternalsPage>
          ads_internals_page_pending_remote) override;
  void GetAdsInternals(GetAdsInternalsCallback callback) override;
  void ClearAdsData(brave_ads::ResultCallback callback) override;
  void GetDiagnostics(GetDiagnosticsCallback callback) override;
  void SetDiagnosticId(const std::string& diagnostic_id) override;

  void GetInternalsCallback(GetAdsInternalsCallback callback,
                            std::optional<base::DictValue> dict);

  base::DictValue BuildDiagnosticsDict() const;
  void OnGetDiagnostics(GetDiagnosticsCallback callback,
                        base::DictValue dict,
                        std::optional<base::ListValue> diagnostic_entries);

  void OnBraveRewardsEnabledPrefChanged(const std::string& path);
  void UpdateBraveRewardsEnabled();

  const raw_ptr<brave_ads::AdsService> ads_service_;  // Not owned.

  const raw_ref<PrefService> prefs_;

  const raw_ptr<variations::VariationsService>
      variations_service_;  // Not owned.

  const GetComponentIdCallback get_ntp_sponsored_images_component_id_callback_;
  const GetComponentIdCallback get_country_resource_component_id_callback_;
  const GetComponentIdCallback get_language_resource_component_id_callback_;
  const GetIsSponsoredImagesLoadedCallback
      get_is_sponsored_images_loaded_callback_;
  const GetComponentIdCallback
      get_ntp_sponsored_images_manifest_version_callback_;

  mojo::Receiver<bat_ads::mojom::AdsInternals> ads_internals_receiver_{this};

  mojo::Remote<bat_ads::mojom::AdsInternalsPage> ads_internals_page_remote_;

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<AdsInternalsHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_INTERNALS_ADS_INTERNALS_HANDLER_H_
