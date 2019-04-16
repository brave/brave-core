/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_INTERNAL_SQL_DATA_STORE_ADAPTER_H_
#define BAT_LEDGER_INTERNAL_SQL_DATA_STORE_ADAPTER_H_

#include <string>

#include "bat/ledger/data_store_adapter.h"

namespace bat_ledger {
namespace mojom {
class DataStoreCommandResponse;
}  // namespace mojom
}  // namespace bat_ledger

namespace ledger {

class SqlDataStoreAdapter : public DataStoreAdapter {
 public:
  SqlDataStoreAdapter();

  // Not copyable, not assignable
  SqlDataStoreAdapter(const SqlDataStoreAdapter&) = delete;
  SqlDataStoreAdapter& operator=(const SqlDataStoreAdapter&) = delete;

  int GetCurrentVersion();
  void SetCurrentVersionForTesting(int value);

  // DataStoreAdapter implementation
  void Initialize(LedgerClient* ledger_client) override;

  void SaveContributionInfo(
      const std::string& probi,
      const int month,
      const int year,
      const uint32_t date,
      const std::string& publisher_key,
      const ledger::REWARDS_CATEGORY category,
      SaveContributionInfoCallback callback) override;

  void GetRecurringTips(ledger::PublisherInfoListCallback callback) override;
  void GetOneTimeTips(ledger::ACTIVITY_MONTH month,
                      int32_t year,
                      ledger::PublisherInfoListCallback callback) override;

  // virtual void SavePublisherInfo(std::unique_ptr<PublisherInfo> publisher_info,
  //                               PublisherInfoCallback callback) = 0;

  // virtual void LoadPublisherInfo(const std::string& publisher_key,
  //                                PublisherInfoCallback callback) = 0;

  // virtual void LoadPanelPublisherInfo(ActivityInfoFilter filter,
  //                                     PublisherInfoCallback callback) = 0;

  // virtual void RestorePublishers(ledger::OnRestoreCallback callback) = 0;

  // virtual void GetExcludedPublishersNumberDB(
  //     ledger::GetExcludedPublishersNumberDBCallback callback) = 0;

  // virtual void GetActivityInfoList(
  //     uint32_t start,
  //     uint32_t limit,
  //     ledger::ActivityInfoFilter filter,
  //     ledger::PublisherInfoListCallback callback) = 0;

  // virtual void SaveActivityInfo(std::unique_ptr<PublisherInfo> publisher_info,
  //                               PublisherInfoCallback callback) = 0;

  // virtual void LoadActivityInfo(ActivityInfoFilter filter,
  //                               PublisherInfoCallback callback) = 0;

  // // TODO(bridiver) - add callback
  // virtual void SaveMediaPublisherInfo(const std::string& media_key,
  //                                     const std::string& publisher_id) = 0;

  // virtual void LoadMediaPublisherInfo(const std::string& media_key,
  //                                     PublisherInfoCallback callback) = 0;

  // // TODO(bridiver) - add callback
  // virtual void SaveRecurringDonation(const std::string& publisher_key,
  //                                    const int amount);

  // virtual void GetRecurringDonations(
  //     ledger::PublisherInfoListCallback callback) = 0;

  // virtual void RemoveRecurringDonation(
  //     const std::string& publisher_key,
  //     ledger::RecurringRemoveCallback callback) = 0;

  // // TODO(bridiver) - add callback
  // virtual void SavePendingContribution(
  //     const ledger::PendingContributionList& list) = 0;

  // // virtual void GetPendingContributionsTotal(
  // //     const GetPendingContributionsTotalCallback& callback) override;

  // // TODO(bridiver) - add callback
  // virtual bool DeleteActivityInfo(const std::string& publisher_key,
  //                                 uint64_t reconcile_stamp) = 0;
 private:
  void OnSaveContributionInfo(
      SaveContributionInfoCallback callback,
      bat_ledger::mojom::DataStoreCommandResponse* response);
  void OnGetPublisherInfo(
      PublisherInfoListCallback callback,
      bat_ledger::mojom::DataStoreCommandResponse* response);

  LedgerClient* ledger_client_;
  int testing_current_version_;
};

}  // namespace ledger

#endif  // BAT_LEDGER_INTERNAL_SQL_DATA_STORE_ADAPTER_H_
