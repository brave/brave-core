/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/attestation/attestation_iosx.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace attestation {

AttestationIOS::AttestationIOS(LedgerImpl* ledger) :
    Attestation(ledger),
    promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {
}

AttestationIOS::~AttestationIOS() = default;

std::string AttestationIOS::ParseStartPayload(
    const std::string& response) {
  absl::optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return "";
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return "";
  }

  const auto* key = dictionary->FindStringKey("publicKey");
  if (!key) {
    BLOG(0, "Public key is wrong");
    return "";
  }

  return *key;
}

type::Result AttestationIOS::ParseClaimSolution(
    const std::string& response,
    std::string* nonce,
    std::string* blob,
    std::string* signature) {
  absl::optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return type::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return type::Result::LEDGER_ERROR;
  }

  const auto* nonce_parsed = dictionary->FindStringKey("nonce");
  if (!nonce_parsed) {
    BLOG(0, "Nonce is wrong");
    return type::Result::LEDGER_ERROR;
  }

  const auto* blob_parsed = dictionary->FindStringKey("blob");
  if (!blob_parsed) {
    BLOG(0, "Blob is wrong");
    return type::Result::LEDGER_ERROR;
  }

  const auto* signature_parsed = dictionary->FindStringKey("signature");
  if (!signature_parsed) {
    BLOG(0, "Signature is wrong");
    return type::Result::LEDGER_ERROR;
  }

  *nonce = *nonce_parsed;
  *blob = *blob_parsed;
  *signature = *signature_parsed;
  return type::Result::LEDGER_OK;
}

void AttestationIOS::Start(
    const std::string& payload,
    StartCallback callback) {
  const std::string key = ParseStartPayload(payload);

  if (key.empty()) {
    BLOG(0, "Key is empty");
    std::move(callback).Run(type::Result::LEDGER_ERROR, "");
    return;
  }
  auto url_callback = base::BindOnce(
      &AttestationIOS::OnStart, base::Unretained(this), std::move(callback));

  promotion_server_->post_devicecheck()->Request(key, std::move(url_callback));
}

void AttestationIOS::OnStart(StartCallback callback,
                             type::Result result,
                             const std::string& nonce) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to start attestation");
    std::move(callback).Run(type::Result::LEDGER_ERROR, "");
    return;
  }

  std::move(callback).Run(type::Result::LEDGER_OK, nonce);
}

void AttestationIOS::Confirm(
    const std::string& solution,
    ConfirmCallback callback) {
  std::string nonce;
  std::string blob;
  std::string signature;
  const type::Result result =
      ParseClaimSolution(solution, &nonce, &blob, &signature);

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to parse solution");
    std::move(callback).Run(type::Result::LEDGER_ERROR);
    return;
  }

  auto url_callback = base::BindOnce(
      &AttestationIOS::OnConfirm, base::Unretained(this), std::move(callback));

  promotion_server_->put_devicecheck()->Request(blob, signature, nonce,
                                                std::move(url_callback));
}

void AttestationIOS::OnConfirm(ConfirmCallback callback, type::Result result) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to confirm attestation");
    std::move(callback).Run(type::Result::LEDGER_ERROR);
    return;
  }

  std::move(callback).Run(type::Result::LEDGER_OK);
}

}  // namespace attestation
}  // namespace ledger
