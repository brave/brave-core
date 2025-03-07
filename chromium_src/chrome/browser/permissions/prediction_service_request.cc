/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/prediction_service_request.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "components/permissions/prediction_service/prediction_service.h"

PredictionServiceRequest::PredictionServiceRequest(
    permissions::PredictionService* service,
    const permissions::PredictionRequestFeatures& entity,
    permissions::PredictionServiceBase::LookupResponseCallback callback)
    : callback_(std::move(callback)) {
  // Fail the prediction service request
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&PredictionServiceRequest::LookupReponseReceived,
                     weak_factory_.GetWeakPtr(), false, false, std::nullopt));
}

PredictionServiceRequest::~PredictionServiceRequest() = default;

void PredictionServiceRequest::LookupReponseReceived(
    bool lookup_succesful,
    bool response_from_cache,
    const std::optional<permissions::GeneratePredictionsResponse>& response) {
  std::move(callback_).Run(lookup_succesful, response_from_cache, response);
}
