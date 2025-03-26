/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/aggregation_service/aggregation_service_impl.h"

#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/ptr_util.h"
#include "base/task/task_traits.h"
#include "base/time/time.h"
#include "base/values.h"
#include "content/browser/aggregation_service/aggregatable_report.h"
#include "content/browser/aggregation_service/aggregatable_report_assembler.h"
#include "content/browser/aggregation_service/aggregatable_report_scheduler.h"
#include "content/browser/aggregation_service/aggregatable_report_sender.h"
#include "content/browser/aggregation_service/aggregation_service_observer.h"
#include "content/browser/aggregation_service/aggregation_service_storage.h"
#include "content/browser/aggregation_service/aggregation_service_storage_sql.h"
#include "content/browser/aggregation_service/public_key.h"
#include "content/browser/storage_partition_impl.h"
#include "content/public/browser/storage_partition.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

AggregationServiceImpl::AggregationServiceImpl(
    bool run_in_memory,
    const base::FilePath& user_data_directory,
    StoragePartitionImpl* storage_partition) {}

AggregationServiceImpl::~AggregationServiceImpl() = default;

// static
std::unique_ptr<AggregationServiceImpl>
AggregationServiceImpl::CreateForTesting(
    bool run_in_memory,
    const base::FilePath& user_data_directory,
    const base::Clock* clock,
    std::unique_ptr<AggregatableReportScheduler> scheduler,
    std::unique_ptr<AggregatableReportAssembler> assembler,
    std::unique_ptr<AggregatableReportSender> sender) {
  return base::WrapUnique<AggregationServiceImpl>(new AggregationServiceImpl(
      run_in_memory, user_data_directory, clock, std::move(scheduler),
      std::move(assembler), std::move(sender)));
}

AggregationServiceImpl::AggregationServiceImpl(
    bool run_in_memory,
    const base::FilePath& user_data_directory,
    const base::Clock* clock,
    std::unique_ptr<AggregatableReportScheduler> scheduler,
    std::unique_ptr<AggregatableReportAssembler> assembler,
    std::unique_ptr<AggregatableReportSender> sender) {}

void AggregationServiceImpl::AssembleReport(
    AggregatableReportRequest report_request,
    AssemblyCallback callback) {}

void AggregationServiceImpl::SendReport(
    const GURL url,
    const AggregatableReport& report,
    std::optional<AggregatableReportRequest::DelayType> delay_type,
    SendCallback callback) {
  std::move(callback).Run(AggregatableReportSender::RequestStatus::kOk);
}

void AggregationServiceImpl::SendReport(
    const GURL url,
    const base::Value& contents,
    std::optional<AggregatableReportRequest::DelayType> delay_type,
    SendCallback callback) {
  std::move(callback).Run(AggregatableReportSender::RequestStatus::kOk);
}

const base::SequenceBound<AggregationServiceStorage>&
AggregationServiceImpl::GetStorage() {
  return storage_;
}

void AggregationServiceImpl::ClearData(
    base::Time delete_begin,
    base::Time delete_end,
    StoragePartition::StorageKeyMatcherFunction filter,
    base::OnceClosure done) {
  std::move(done).Run();
}

void AggregationServiceImpl::ScheduleReport(
    AggregatableReportRequest report_request) {}

void AggregationServiceImpl::AssembleAndSendReport(
    AggregatableReportRequest report_request) {}

void AggregationServiceImpl::SetPublicKeysForTesting(
    const GURL& url,
    const PublicKeyset& keyset) {}

void AggregationServiceImpl::GetPendingReportRequestsForWebUI(
    base::OnceCallback<
        void(std::vector<AggregationServiceStorage::RequestAndId>)> callback) {
  std::move(callback).Run({});
}

void AggregationServiceImpl::SendReportsForWebUI(
    const std::vector<AggregationServiceStorage::RequestId>& ids,
    base::OnceClosure reports_sent_callback) {
  std::move(reports_sent_callback).Run();
}

void AggregationServiceImpl::GetPendingReportReportingOrigins(
    base::OnceCallback<void(std::set<url::Origin>)> callback) {
  std::move(callback).Run({});
}

void AggregationServiceImpl::AddObserver(AggregationServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AggregationServiceImpl::RemoveObserver(
    AggregationServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// For tests to compile.
void AggregationServiceImpl::OnScheduledReportTimeReached(
    std::vector<AggregationServiceStorage::RequestAndId> requests_and_ids) {}

}  // namespace content
