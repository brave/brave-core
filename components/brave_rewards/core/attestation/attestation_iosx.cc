/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/attestation/attestation_iosx.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace attestation {

AttestationIOS::AttestationIOS(RewardsEngineImpl& engine)
    : Attestation(engine), promotion_server_(engine) {}

AttestationIOS::~AttestationIOS() = default;

std::string AttestationIOS::ParseStartPayload(const std::string& response) {
  std::optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return "";
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* key = dict.FindString("publicKey");
  if (!key) {
    engine_->LogError(FROM_HERE) << "Public key is wrong";
    return "";
  }

  return *key;
}

mojom::Result AttestationIOS::ParseClaimSolution(const std::string& response,
                                                 std::string* nonce,
                                                 std::string* blob,
                                                 std::string* signature) {
  std::optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* nonce_parsed = dict.FindString("nonce");
  if (!nonce_parsed) {
    engine_->LogError(FROM_HERE) << "Nonce is wrong";
    return mojom::Result::FAILED;
  }

  const auto* blob_parsed = dict.FindString("blob");
  if (!blob_parsed) {
    engine_->LogError(FROM_HERE) << "Blob is wrong";
    return mojom::Result::FAILED;
  }

  const auto* signature_parsed = dict.FindString("signature");
  if (!signature_parsed) {
    engine_->LogError(FROM_HERE) << "Signature is wrong";
    return mojom::Result::FAILED;
  }

  *nonce = *nonce_parsed;
  *blob = *blob_parsed;
  *signature = *signature_parsed;
  return mojom::Result::OK;
}

void AttestationIOS::Start(const std::string& payload, StartCallback callback) {
  const std::string key = ParseStartPayload(payload);

  if (key.empty()) {
    engine_->LogError(FROM_HERE) << "Key is empty";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }
  auto url_callback = base::BindOnce(
      &AttestationIOS::OnStart, base::Unretained(this), std::move(callback));

  promotion_server_.post_devicecheck().Request(key, std::move(url_callback));
}

void AttestationIOS::OnStart(StartCallback callback,
                             mojom::Result result,
                             const std::string& nonce) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to start attestation";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  std::move(callback).Run(mojom::Result::OK, nonce);
}

void AttestationIOS::Confirm(const std::string& solution,
                             ConfirmCallback callback) {
  std::string nonce;
  std::string blob;
  std::string signature;
  const mojom::Result result =
      ParseClaimSolution(solution, &nonce, &blob, &signature);

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to parse solution";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto url_callback = base::BindOnce(
      &AttestationIOS::OnConfirm, base::Unretained(this), std::move(callback));

  promotion_server_.put_devicecheck().Request(blob, signature, nonce,
                                              std::move(url_callback));
}

void AttestationIOS::OnConfirm(ConfirmCallback callback, mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to confirm attestation";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace attestation
}  // namespace brave_rewards::internal
