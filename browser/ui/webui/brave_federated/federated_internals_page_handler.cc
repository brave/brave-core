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
#include "brave/components/brave_federated/data_stores/async_data_store.h"
#include "brave/components/brave_federated/notification_ad_task_constants.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_federated {

FederatedInternalsPageHandler::FederatedInternalsPageHandler(
    mojo::PendingReceiver<federated_internals::mojom::PageHandler> receiver,
    mojo::PendingRemote<federated_internals::mojom::Page> page,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      data_store_service_(
          BraveFederatedServiceFactory::GetForBrowserContext(profile)
              ->GetDataStoreService()) {}

FederatedInternalsPageHandler::~FederatedInternalsPageHandler() = default;

void FederatedInternalsPageHandler::UpdateDataStoresInfo() {
  if (!data_store_service_) {
    return;
  }
  AsyncDataStore* notification_ad_data_store =
      data_store_service_->GetDataStore(kNotificationAdTaskName);
  if (!notification_ad_data_store) {
    return;
  }

  notification_ad_data_store->LoadTrainingData(
      base::BindOnce(&FederatedInternalsPageHandler::OnUpdateDataStoresInfo,
                     weak_ptr_factory_.GetWeakPtr()));
}

void FederatedInternalsPageHandler::OnUpdateDataStoresInfo(
    TrainingData training_data) {
  std::vector<federated_internals::mojom::TrainingInstancePtr>
      training_instances;
  for (auto& item : training_data) {
    auto training_instance =
        federated_internals::mojom::TrainingInstance::New();
    training_instance->id = item.first;
    std::vector<mojom::CovariatePtr>& covariates = item.second;
    for (auto& covariate : covariates) {
      training_instance->covariates.push_back(std::move(covariate));
    }

    training_instances.push_back(std::move(training_instance));
  }
  page_->OnUpdateDataStoresInfo(std::move(training_instances));
}

}  // namespace brave_federated
