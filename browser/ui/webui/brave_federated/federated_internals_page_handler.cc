/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_federated/federated_internals_page_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/browser/brave_federated/brave_federated_service_factory.h"
#include "brave/components/brave_federated/brave_federated_service.h"
#include "brave/components/brave_federated/data_store_service.h"
#include "brave/components/brave_federated/data_stores/ad_notification_timing_data_store.h"
#include "chrome/browser/profiles/profile.h"

FederatedInternalsPageHandler::FederatedInternalsPageHandler(
    mojo::PendingReceiver<federated_internals::mojom::PageHandler> receiver,
    mojo::PendingRemote<federated_internals::mojom::Page> page,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      data_store_service_(
          brave_federated::BraveFederatedServiceFactory::GetForBrowserContext(
              profile)
              ->GetDataStoreService()) {}

FederatedInternalsPageHandler::~FederatedInternalsPageHandler() {}

void FederatedInternalsPageHandler::GetAdStoreInfo() {
  if (!data_store_service_)
    return;
  auto* const ad_notification_data_store =
      data_store_service_->GetAdNotificationTimingDataStore();

  ad_notification_data_store->LoadLogs(
      base::BindOnce(&FederatedInternalsPageHandler::OnAdStoreInfoAvailable,
                     weak_ptr_factory_.GetWeakPtr()));
}

void FederatedInternalsPageHandler::OnAdStoreInfoAvailable(
    brave_federated::AdNotificationTimingDataStore::
        IdToAdNotificationTimingTaskLogMap ad_notification_timing_logs) {
  std::vector<federated_internals::mojom::AdStoreLogPtr> ad_logs;
  for (auto const& object : ad_notification_timing_logs) {
    auto ad_store_log = federated_internals::mojom::AdStoreLog::New();
    const auto& log = object.second;
    ad_store_log->log_id = log.id;
    ad_store_log->log_time = log.time.ToJsTime();
    ad_store_log->log_locale = log.locale;
    ad_store_log->log_number_of_tabs = log.number_of_tabs;
    ad_store_log->log_label = log.label;
    ad_logs.emplace_back(std::move(ad_store_log));
  }
  page_->OnAdStoreInfoAvailable(std::move(ad_logs));
}
