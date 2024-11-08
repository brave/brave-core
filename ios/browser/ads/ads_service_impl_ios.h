/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_ADS_ADS_SERVICE_IMPL_IOS_H_
#define BRAVE_IOS_BROWSER_ADS_ADS_SERVICE_IMPL_IOS_H_

#include <memory>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_ads {

class Ads;
class AdsClient;
class Database;

class AdsServiceImplIOS : public KeyedService {
 public:
  explicit AdsServiceImplIOS(PrefService* prefs);

  AdsServiceImplIOS(const AdsServiceImplIOS&) = delete;
  AdsServiceImplIOS& operator=(const AdsServiceImplIOS&) = delete;

  AdsServiceImplIOS(AdsServiceImplIOS&&) noexcept = delete;
  AdsServiceImplIOS& operator=(AdsServiceImplIOS&&) noexcept = delete;

  ~AdsServiceImplIOS() override;

  bool IsRunning() const;

  void InitializeAds(const std::string& storage_path,
                     AdsClient& ads_client,
                     mojom::SysInfoPtr mojom_sys_info,
                     mojom::BuildChannelInfoPtr mojom_build_channel,
                     mojom::WalletInfoPtr mojom_wallet,
                     InitializeCallback callback);
  void ShutdownAds(ShutdownCallback callback);

  void ClearData(base::OnceClosure callback);

  void RunDBTransaction(mojom::DBTransactionInfoPtr mojom_db_transaction,
                        RunDBTransactionCallback callback);

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback);

  void MaybeServeInlineContentAd(const std::string& dimensions,
                                 MaybeServeInlineContentAdCallback callback);
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  void MaybeGetNotificationAd(const std::string& placement_id,
                              MaybeGetNotificationAdCallback callback);
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      mojom::NotificationAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  void MaybeGetSearchResultAd(const std::string& placement_id,
                              MaybeGetSearchResultAdCallback callback);
  void TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  void PurgeOrphanedAdEventsForType(
      mojom::AdType mojom_ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback);

 private:
  // KeyedService:
  void Shutdown() override;

  void InitializeDatabase(const std::string& storage_path);
  void Cleanup();

  void InitializeAdsCallback(InitializeCallback callback, bool success);
  void ShutdownAdsCallback(ShutdownCallback callback, bool success);

  const raw_ptr<PrefService> prefs_ = nullptr;  // Not owned.

  const scoped_refptr<base::SequencedTaskRunner> database_queue_;

  base::FilePath storage_path_;

  base::SequenceBound<Database> database_;

  std::unique_ptr<Ads> ads_;

  base::WeakPtrFactory<AdsServiceImplIOS> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_ADS_ADS_SERVICE_IMPL_IOS_H_
