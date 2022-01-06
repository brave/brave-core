/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/post_votes_endpoint.h"

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/core/environment_config.h"
#include "bat/ledger/internal/core/privacy_pass.h"
#include "bat/ledger/internal/core/value_converters.h"

namespace ledger {

namespace {

struct RequestCredential {
  std::string t;
  std::string public_key;
  std::string signature;

  auto ToValue() const {
    ValueWriter w;
    w.Write("t", t);
    w.Write("publicKey", public_key);
    w.Write("signature", signature);
    return w.Finish();
  }
};

struct RequestData {
  std::vector<RequestCredential> credentials;
  std::string vote;

  auto ToValue() const {
    ValueWriter w;
    w.Write("credentials", credentials);
    w.Write("vote", vote);
    return w.Finish();
  }
};

struct RequestVote {
  std::string channel;
  PaymentVoteType type;

  auto ToValue() const {
    ValueWriter w;
    w.Write("channel", channel);
    w.Write("type", type);
    return w.Finish();
  }

  auto ToBase64() const {
    std::string json;
    bool ok = base::JSONWriter::Write(ToValue(), &json);
    DCHECK(ok);
    std::string encoded;
    base::Base64Encode(json, &encoded);
    return encoded;
  }
};

}  // namespace

URLRequest PostVotesEndpoint::MapRequest(
    const std::string& publisher_id,
    PaymentVoteType vote_type,
    const std::vector<PaymentVote>& votes) {
  RequestVote request_vote{.channel = publisher_id, .type = vote_type};
  RequestData data{.vote = request_vote.ToBase64()};

  auto& pp = context().Get<PrivacyPass>();

  for (auto& v : votes) {
    auto result = pp.SignMessage(v.unblinded_token, data.vote);
    if (!result) {
      // If for some reason we are unable to sign the message with the provided
      // unblinded token (e.g. if the token is corrupt) then log an error and
      // continue processing the remaining tokens. The token will be marked as
      // redeemed if the operation succeeds.
      context().LogError(FROM_HERE)
          << "Unable to sign message with unblined token";
    } else {
      data.credentials.push_back(
          RequestCredential{.t = result->preimage,
                            .public_key = v.public_key,
                            .signature = result->signature});
    }
  }

  std::string host = context().Get<EnvironmentConfig>().payment_service_host();
  auto request = URLRequest::Post("https://" + host + "/v1/votes");
  request.SetBody(data.ToValue());
  return request;
}

bool PostVotesEndpoint::MapResponse(const URLResponse& response) {
  if (!response.Succeeded()) {
    context().LogError(FROM_HERE) << "HTTP " << response.status_code();
    return false;
  }
  return true;
}

}  // namespace ledger
