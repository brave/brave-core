/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/payment_service.h"

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/internal/core/url_fetcher.h"
#include "bat/ledger/internal/payments/get_credentials_endpoint.h"
#include "bat/ledger/internal/payments/get_order_endpoint.h"
#include "bat/ledger/internal/payments/post_credentials_endpoint.h"
#include "bat/ledger/internal/payments/post_external_transaction_endpoint.h"
#include "bat/ledger/internal/payments/post_order_endpoint.h"
#include "bat/ledger/internal/payments/post_votes_endpoint.h"

namespace ledger {

Future<absl::optional<PaymentOrder>> PaymentService::CreateOrder(
    const std::map<std::string, int>& items) {
  return context().Get<URLFetcher>().FetchEndpoint<PostOrderEndpoint>(items);
}

Future<absl::optional<PaymentOrder>> PaymentService::GetOrder(
    const std::string& order_id) {
  return context().Get<URLFetcher>().FetchEndpoint<GetOrderEndpoint>(order_id);
}

Future<bool> PaymentService::PostExternalTransaction(
    const std::string& order_id,
    const std::string& transaction_id,
    ExternalWalletProvider provider) {
  return context()
      .Get<URLFetcher>()
      .FetchEndpoint<PostExternalTransactionEndpoint>(order_id, transaction_id,
                                                      provider);
}

Future<bool> PaymentService::PostCredentials(
    const std::string& order_id,
    const std::string& item_id,
    PaymentCredentialType type,
    const std::vector<std::string>& blinded_tokens) {
  return context().Get<URLFetcher>().FetchEndpoint<PostCredentialsEndpoint>(
      order_id, item_id, type, blinded_tokens);
}

Future<absl::optional<PaymentCredentials>> PaymentService::GetCredentials(
    const std::string& order_id,
    const std::string& item_id) {
  return context().Get<URLFetcher>().FetchEndpoint<GetCredentialsEndpoint>(
      order_id, item_id);
}

Future<bool> PaymentService::PostPublisherVotes(
    const std::string& publisher_id,
    PaymentVoteType vote_type,
    const std::vector<PaymentVote>& votes) {
  return context().Get<URLFetcher>().FetchEndpoint<PostVotesEndpoint>(
      publisher_id, vote_type, votes);
}

}  // namespace ledger
