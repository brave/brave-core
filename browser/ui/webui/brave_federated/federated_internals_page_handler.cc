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
#include "brave/components/brave_federated/data_stores/data_store.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"
#include "brave/components/brave_federated/tasks.h"
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

FederatedInternalsPageHandler::~FederatedInternalsPageHandler() = default;

void FederatedInternalsPageHandler::GetDataStoreInfo() {
  if (!data_store_service_)
    return;
  const auto* ad_notification_data_store =
      data_store_service_->GetDataStore(kAdNotificationTaskName);

  if (!ad_notification_data_store) {
    return;
  }

  ad_notification_data_store->LoadTrainingData(
      base::BindOnce(&FederatedInternalsPageHandler::OnDataStoreInfoAvailable,
                     weak_ptr_factory_.GetWeakPtr()));
}

void FederatedInternalsPageHandler::OnDataStoreInfoAvailable(
    brave_federated::DataStore::TrainingData training_data) {
  std::vector<federated_internals::mojom::InternalsTrainingInstancePtr>
      internals_training_instances;
  for (const auto& training_instance : training_data) {
    auto internals_training_instance =
        federated_internals::mojom::TrainingInstance::New();
    for (const auto& covariate : training_instance.second) {
      auto internals_covariate = federated_internals::mojom::InternalsCovariate::New();
      internals_covariate->training_instance_id = training_instance.first;
      internals_covariate->covariate = covariate;
    }

    internals_training_instances.push_back(std::move(internals_training_instance));
  }
  page_->OnDataStoreInfoAvailable(std::move(internals_training_instances));
}
