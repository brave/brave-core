/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/attestation/attestation_androidx.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace attestation {

AttestationAndroid::AttestationAndroid(LedgerImpl* ledger) :
    Attestation(ledger),
    promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {
}

AttestationAndroid::~AttestationAndroid() = default;

void AttestationAndroid::ParseClaimSolution(
    const std::string& response,
    std::string* token,
    std::string* nonce) {
  DCHECK(token && nonce);

  absl::optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    BLOG(0, "Parsing of solution failed");
    return;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* nonce_string = dict.FindString("nonce");
  if (!nonce_string) {
    BLOG(0, "Nonce is missing");
    return;
  }

  const auto* token_string = dict.FindString("token");
  if (!token_string) {
    BLOG(0, "Token is missing");
    return;
  }

  *nonce = *nonce_string;
  *token = *token_string;
}

void AttestationAndroid::Start(
    const std::string& payload,
    StartCallback callback) {
  auto url_callback =
      base::BindOnce(&AttestationAndroid::OnStart, base::Unretained(this),
                     std::move(callback));
  promotion_server_->post_safetynet()->Request(std::move(url_callback));
}

void AttestationAndroid::OnStart(StartCallback callback,
                                 mojom::Result result,
                                 const std::string& nonce) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to start attestation");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, "");
    return;
  }

  std::move(callback).Run(mojom::Result::LEDGER_OK, nonce);
}

void AttestationAndroid::Confirm(
    const std::string& solution,
    ConfirmCallback callback)  {
  std::string token;
  std::string nonce;
  ParseClaimSolution(solution, &token, &nonce);

  auto url_callback =
      base::BindOnce(&AttestationAndroid::OnConfirm, base::Unretained(this),
                     std::move(callback));

  promotion_server_->put_safetynet()->Request(token, nonce,
                                              std::move(url_callback));
}

void AttestationAndroid::OnConfirm(ConfirmCallback callback,
                                   mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to confirm attestation");
    std::move(callback).Run(result);
    return;
  }

  std::move(callback).Run(mojom::Result::LEDGER_OK);
}

}  // namespace attestation
}  // namespace ledger
