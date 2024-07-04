/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/star_randomness_points.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/network_annotations.h"
#include "brave/components/p3a/p3a_config.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace p3a {

namespace {

constexpr std::size_t kMaxRandomnessResponseSize = 131072;

std::unique_ptr<rust::Vec<constellation::VecU8>> DecodeBase64List(
    const base::Value::List& list) {
  std::unique_ptr<rust::Vec<constellation::VecU8>> result =
      std::make_unique<rust::Vec<constellation::VecU8>>();
  for (const base::Value& list_entry : list) {
    const std::string* entry_str = list_entry.GetIfString();
    if (entry_str == nullptr) {
      LOG(ERROR) << "StarRandomnessPoints: list value is not string";
      return nullptr;
    }
    constellation::VecU8 entry_dec_vec;
    std::optional<std::vector<uint8_t>> entry_dec =
        base::Base64Decode(*entry_str);
    if (!entry_dec.has_value()) {
      LOG(ERROR) << "StarRandomnessPoints: failed to decode base64 value";
      return nullptr;
    }
    std::copy(entry_dec->cbegin(), entry_dec->cend(),
              std::back_inserter(entry_dec_vec.data));
    result->push_back(entry_dec_vec);
  }
  return result;
}

}  // namespace

StarRandomnessPoints::StarRandomnessPoints(
    scoped_refptr<network::SharedURLLoaderFactory> url_loaders_factory,
    const P3AConfig* config)
    : url_loader_factory_(url_loaders_factory),
      config_(config) {}

StarRandomnessPoints::~StarRandomnessPoints() = default;

void StarRandomnessPoints::SendRandomnessRequest(
    MetricLogType log_type,
    uint8_t epoch,
    StarRandomnessMeta* randomness_meta,
    const rust::Vec<constellation::VecU8>& rand_req_points,
    RandomnessDataCallback callback) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url =
      GURL(base::StrCat({config_->star_randomness_host, "/instances/",
                         MetricLogTypeToString(log_type), "/randomness"}));
  resource_request->method = "POST";

  url_loaders_[log_type] = network::SimpleURLLoader::Create(
      std::move(resource_request), GetRandomnessServerInfoAnnotation());

  base::Value::Dict payload_dict;
  base::Value::List points_list;
  for (const auto& point_data : rand_req_points) {
    points_list.Append(base::Base64Encode(point_data.data));
  }
  payload_dict.Set("points", std::move(points_list));
  payload_dict.Set("epoch", epoch);

  std::string payload_str;
  if (!base::JSONWriter::Write(payload_dict, &payload_str)) {
    LOG(ERROR) << "StarRandomnessPoints: failed to serialize "
                  "randomness req payload";
    std::move(callback).Run(nullptr, nullptr);
    return;
  }

  url_loaders_[log_type]->AttachStringForUpload(payload_str,
                                                "application/json");
  url_loaders_[log_type]->SetURLLoaderFactoryOptions(
      network::mojom::kURLLoadOptionSendSSLInfoWithResponse);

  url_loaders_[log_type]->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&StarRandomnessPoints::HandleRandomnessResponse,
                     base::Unretained(this), log_type, randomness_meta,
                     std::move(callback)),
      kMaxRandomnessResponseSize);
}

void StarRandomnessPoints::HandleRandomnessResponse(
    MetricLogType log_type,
    StarRandomnessMeta* randomness_meta,
    RandomnessDataCallback callback,
    std::unique_ptr<std::string> response_body) {
  if (!response_body || response_body->empty()) {
    std::string error_str =
        net::ErrorToShortString(url_loaders_[log_type]->NetError());
    url_loaders_[log_type] = nullptr;
    LOG(ERROR) << "StarRandomnessPoints: no response body for "
                  "randomness request, "
               << "net error: " << error_str;
    std::move(callback).Run(nullptr, nullptr);
    return;
  }
  if (!randomness_meta->VerifyRandomnessCert(url_loaders_[log_type].get())) {
    std::move(callback).Run(nullptr, nullptr);
    url_loaders_[log_type] = nullptr;
    return;
  }
  url_loaders_[log_type] = nullptr;
  base::JSONReader::Result parsed_body =
      base::JSONReader::ReadAndReturnValueWithError(*response_body);
  if (!parsed_body.has_value() || !parsed_body.value().is_dict()) {
    LOG(ERROR) << "StarRandomnessPoints: failed to parse randomness "
                  "response json: "
               << parsed_body.error().message;
    std::move(callback).Run(nullptr, nullptr);
    return;
  }
  const base::Value::Dict& root = parsed_body->GetDict();
  const base::Value::List* points = root.FindList("points");
  const base::Value::List* proofs = root.FindList("proofs");
  if (points == nullptr) {
    LOG(ERROR) << "StarRandomnessPoints: failed to find points list in "
                  "randomness response";
    std::move(callback).Run(nullptr, nullptr);
    return;
  }
  std::unique_ptr<rust::Vec<constellation::VecU8>> points_vec =
      DecodeBase64List(*points);
  if (points_vec == nullptr) {
    std::move(callback).Run(nullptr, nullptr);
    return;
  }
  std::unique_ptr<rust::Vec<constellation::VecU8>> proofs_vec;
  if (proofs != nullptr) {
    proofs_vec = DecodeBase64List(*proofs);
    if (!proofs_vec) {
      std::move(callback).Run(nullptr, nullptr);
      return;
    }
  } else {
    proofs_vec = std::make_unique<rust::Vec<constellation::VecU8>>();
  }
  std::move(callback).Run(std::move(points_vec), std::move(proofs_vec));
}

}  // namespace p3a
