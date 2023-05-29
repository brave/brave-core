/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/get_wallet/get_wallet.h"

#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace brave_rewards::internal::endpoint::promotion {

void GetWallet::Request(GetWalletCallback callback) const {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  ledger().LoadURL(std::move(request),
                   std::bind(&GetWallet::OnRequest, this, _1, callback));
}

std::string GetWallet::GetUrl() const {
  const auto rewards_wallet = ledger().wallet()->GetWallet();
  if (!rewards_wallet) {
    BLOG(0, "Rewards wallet is null!");
    return "";
  }

  return GetServerUrl("/v3/wallet/" + rewards_wallet->payment_id);
}

void GetWallet::OnRequest(mojom::UrlResponsePtr response,
                          GetWalletCallback callback) const {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::LEDGER_OK) {
    return callback(result, std::string{}, false);
  }

  std::string custodian = "";
  bool linked = false;
  result = ParseBody(response->body, &custodian, &linked);
  callback(result, custodian, linked);
}

mojom::Result GetWallet::CheckStatusCode(int status_code) const {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid payment id");
    return mojom::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized payment id");
    return mojom::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

mojom::Result GetWallet::ParseBody(const std::string& body,
                                   std::string* custodian,
                                   bool* linked) const {
  DCHECK(custodian);
  DCHECK(linked);
  *custodian = "";
  *linked = false;

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  if (const auto* deposit_account_provider =
          dict.FindDict("depositAccountProvider")) {
    const std::string* name = deposit_account_provider->FindString("name");
    const std::string* id = deposit_account_provider->FindString("id");
    const std::string* linking_id =
        deposit_account_provider->FindString("linkingId");

    if (!name || !id || !linking_id) {
      return mojom::Result::LEDGER_ERROR;
    }

    *custodian = *name;
    *linked = !id->empty() && !linking_id->empty();
  }

  return mojom::Result::LEDGER_OK;
}

}  // namespace brave_rewards::internal::endpoint::promotion
