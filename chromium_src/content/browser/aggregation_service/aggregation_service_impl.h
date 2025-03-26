/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_AGGREGATION_SERVICE_AGGREGATION_SERVICE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_AGGREGATION_SERVICE_AGGREGATION_SERVICE_IMPL_H_

#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/observer_list.h"
#include "base/threading/sequence_bound.h"
#include "content/browser/aggregation_service/aggregatable_report_assembler.h"
#include "content/browser/aggregation_service/aggregatable_report_scheduler.h"
#include "content/browser/aggregation_service/aggregatable_report_sender.h"
#include "content/browser/aggregation_service/aggregation_service.h"
#include "content/browser/aggregation_service/aggregation_service_observer.h"
#include "content/browser/aggregation_service/aggregation_service_storage.h"
#include "content/browser/aggregation_service/aggregation_service_storage_context.h"
#include "content/common/content_export.h"
#include "content/public/browser/storage_partition.h"

class GURL;

namespace base {
class Clock;
class FilePath;
}  // namespace base

namespace url {
class Origin;
}  // namespace url

namespace content {

struct PublicKeyset;
class AggregatableReport;
class AggregatableReportRequest;
class AggregationServiceStorage;
class AggregatableReportScheduler;
class StoragePartitionImpl;

// UI thread class that manages the lifetime of the underlying storage. Owned by
// the StoragePartitionImpl. Lifetime is bound to lifetime of the
// StoragePartitionImpl.
class CONTENT_EXPORT AggregationServiceImpl
    : public AggregationService,
      public AggregationServiceStorageContext {
 public:
  static std::unique_ptr<AggregationServiceImpl> CreateForTesting(
      bool run_in_memory,
      const base::FilePath& user_data_directory,
      const base::Clock* clock,
      std::unique_ptr<AggregatableReportScheduler> scheduler,
      std::unique_ptr<AggregatableReportAssembler> assembler,
      std::unique_ptr<AggregatableReportSender> sender);

  AggregationServiceImpl(bool run_in_memory,
                         const base::FilePath& user_data_directory,
                         StoragePartitionImpl* storage_partition);
  AggregationServiceImpl(const AggregationServiceImpl& other) = delete;
  AggregationServiceImpl& operator=(const AggregationServiceImpl& other) =
      delete;
  AggregationServiceImpl(AggregationServiceImpl&& other) = delete;
  AggregationServiceImpl& operator=(AggregationServiceImpl&& other) = delete;
  ~AggregationServiceImpl() override;

  // AggregationService:
  void AssembleReport(AggregatableReportRequest report_request,
                      AssemblyCallback callback) override;
  void SendReport(
      const GURL url,
      const AggregatableReport& report,
      std::optional<AggregatableReportRequest::DelayType> delay_type,
      SendCallback callback) override;
  void SendReport(
      const GURL url,
      const base::Value& contents,
      std::optional<AggregatableReportRequest::DelayType> delay_type,
      SendCallback callback) override;
  void ClearData(base::Time delete_begin,
                 base::Time delete_end,
                 StoragePartition::StorageKeyMatcherFunction filter,
                 base::OnceClosure done) override;
  void ScheduleReport(AggregatableReportRequest report_request) override;
  void AssembleAndSendReport(AggregatableReportRequest report_request) override;
  void GetPendingReportRequestsForWebUI(
      base::OnceCallback<
          void(std::vector<AggregationServiceStorage::RequestAndId>)> callback)
      override;
  void SendReportsForWebUI(
      const std::vector<AggregationServiceStorage::RequestId>& ids,
      base::OnceClosure reports_sent_callback) override;
  void GetPendingReportReportingOrigins(
      base::OnceCallback<void(std::set<url::Origin>)> callback) override;
  void AddObserver(AggregationServiceObserver* observer) override;
  void RemoveObserver(AggregationServiceObserver* observer) override;

  // AggregationServiceStorageContext:
  const base::SequenceBound<AggregationServiceStorage>& GetStorage() override;

  // Sets the public keys for `url` in storage to allow testing without network.
  void SetPublicKeysForTesting(const GURL& url, const PublicKeyset& keyset);

 private:
  // Allows access to `OnScheduledReportTimeReached()`.
  friend class AggregationServiceImplTest;

  AggregationServiceImpl(bool run_in_memory,
                         const base::FilePath& user_data_directory,
                         const base::Clock* clock,
                         std::unique_ptr<AggregatableReportScheduler> scheduler,
                         std::unique_ptr<AggregatableReportAssembler> assembler,
                         std::unique_ptr<AggregatableReportSender> sender);

  void OnScheduledReportTimeReached(
      std::vector<AggregationServiceStorage::RequestAndId> requests_and_ids);

  base::SequenceBound<AggregationServiceStorage> storage_;
  base::ObserverList<AggregationServiceObserver> observers_;
};

}  // namespace content

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_AGGREGATION_SERVICE_AGGREGATION_SERVICE_IMPL_H_
