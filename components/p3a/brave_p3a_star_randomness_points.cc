/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_star_randomness_points.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/network_annotations.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave {

namespace {

constexpr std::size_t kMaxRandomnessResponseSize = 131072;

std::unique_ptr<rust::Vec<nested_star::VecU8>> DecodeBase64List(
    const base::Value* list) {
  std::unique_ptr<rust::Vec<nested_star::VecU8>> result(
      new rust::Vec<nested_star::VecU8>());
  for (const base::Value& list_entry : list->GetList()) {
    const std::string* entry_str = list_entry.GetIfString();
    if (entry_str == nullptr) {
      LOG(ERROR) << "BraveP3AStarRandomnessPoints: list value is not string";
      return nullptr;
    }
    nested_star::VecU8 entry_dec_vec;
    absl::optional<std::vector<uint8_t>> entry_dec =
        base::Base64Decode(*entry_str);
    if (!entry_dec.has_value()) {
      LOG(ERROR)
          << "BraveP3AStarRandomnessPoints: failed to decode base64 value";
      return nullptr;
    }
    std::copy(entry_dec->cbegin(), entry_dec->cend(),
              std::back_inserter(entry_dec_vec.data));
    result->push_back(entry_dec_vec);
  }
  return result;
}

}  // namespace

BraveP3AStarRandomnessPoints::BraveP3AStarRandomnessPoints(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    RandomnessDataCallback data_callback,
    BraveP3AConfig* config)
    : url_loader_factory_(url_loader_factory),
      data_callback_(data_callback),
      config_(config) {}

BraveP3AStarRandomnessPoints::~BraveP3AStarRandomnessPoints() {}

void BraveP3AStarRandomnessPoints::SendRandomnessRequest(
    std::string histogram_name,
    BraveP3AStarRandomnessMeta* randomness_meta,
    uint8_t epoch,
    rust::Box<nested_star::RandomnessRequestStateWrapper>
        randomness_request_state,
    const rust::Vec<nested_star::VecU8>& rand_req_points) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(config_->star_randomness_host + "/randomness");
  resource_request->method = "POST";

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetRandomnessServerInfoAnnotation());

  base::Value payload_dict(base::Value::Type::DICT);
  base::Value points_list(base::Value::Type::LIST);
  for (const auto& point_data : rand_req_points) {
    points_list.Append(base::Base64Encode(point_data.data));
  }
  payload_dict.SetKey("points", std::move(points_list));
  payload_dict.SetIntKey("epoch", epoch);

  std::string payload_str;
  if (!base::JSONWriter::Write(payload_dict, &payload_str)) {
    LOG(ERROR) << "BraveP3AStarRandomnessPoints: failed to serialize "
                  "randomness req payload";
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }

  url_loader_->AttachStringForUpload(payload_str, "application/json");
  url_loader_->SetURLLoaderFactoryOptions(
      network::mojom::kURLLoadOptionSendSSLInfoWithResponse);

  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP3AStarRandomnessPoints::HandleRandomnessResponse,
                     base::Unretained(this), histogram_name, randomness_meta,
                     epoch, std::move(randomness_request_state)),
      kMaxRandomnessResponseSize);
}

void BraveP3AStarRandomnessPoints::HandleRandomnessResponse(
    std::string histogram_name,
    BraveP3AStarRandomnessMeta* randomness_meta,
    uint8_t epoch,
    rust::Box<nested_star::RandomnessRequestStateWrapper>
        randomness_request_state,
    std::unique_ptr<std::string> response_body) {
  if (!response_body || response_body->empty()) {
    std::string error_str = net::ErrorToShortString(url_loader_->NetError());
    url_loader_.reset();
    LOG(ERROR) << "BraveP3AStarRandomnessPoints: no response body for "
                  "randomness request, "
               << "net error: " << error_str;
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }
  if (!randomness_meta->VerifyRandomnessCert(url_loader_.get())) {
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    url_loader_.reset();
    return;
  }
  url_loader_.reset();
  base::JSONReader::ValueWithError parsed_body =
      base::JSONReader::ReadAndReturnValueWithError(*response_body);
  if (!parsed_body.value.has_value() || !parsed_body.value->is_dict()) {
    LOG(ERROR) << "BraveP3AStarRandomnessPoints: failed to parse randomness "
                  "response json: "
               << parsed_body.error_message;
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }
  const base::Value* points_value = parsed_body.value->FindListKey("points");
  const base::Value* proofs_value = parsed_body.value->FindListKey("proofs");
  if (points_value == nullptr) {
    LOG(ERROR) << "BraveP3AStarRandomnessPoints: failed to find points list in "
                  "randomness response";
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }
  std::unique_ptr<rust::Vec<nested_star::VecU8>> points_vec =
      DecodeBase64List(points_value);
  if (points_vec == nullptr) {
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }
  std::unique_ptr<rust::Vec<nested_star::VecU8>> proofs_vec;
  if (proofs_value != nullptr) {
    proofs_vec = DecodeBase64List(proofs_value);
    if (!proofs_vec) {
      data_callback_.Run(histogram_name, epoch,
                         std::move(randomness_request_state), nullptr, nullptr);
      return;
    }
  } else {
    proofs_vec.reset(new rust::Vec<nested_star::VecU8>());
  }
  data_callback_.Run(histogram_name, epoch, std::move(randomness_request_state),
                     std::move(points_vec), std::move(proofs_vec));
}

}  // namespace brave
