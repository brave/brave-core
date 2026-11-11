/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/offer_events/offer_events.h"

#include <algorithm>
#include <utility>

#include "base/check_op.h"
#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/rand_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/engine/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/engine/rewards_engine.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"
#include "brave/components/brave_rewards/core/engine/util/request_signer.h"
#include "brave/components/brave_rewards/core/engine/util/rewards_prefs.h"
#include "brave/components/brave_rewards/core/engine/util/url_loader.h"
#include "brave/components/brave_rewards/core/engine/wallet/wallet.h"
#include "brave/components/brave_rewards/core/features.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/challenge_bypass_ristretto/verification_signature.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {

namespace {

using challenge_bypass_ristretto::UnblindedToken;

constexpr int kTokenBatchSize = 50;
constexpr size_t kMaxPendingEvents = 100;
constexpr base::TimeDelta kRefillBackoff = base::Hours(1);

// Bounds for the Griffin params. Griffin can't set values outside them.
constexpr base::TimeDelta kMinSendDelayFloor = base::Seconds(30);
constexpr base::TimeDelta kMaxSendDelayCeiling = base::Hours(24);
constexpr size_t kMaxEventTypeLength = 32;
constexpr size_t kMaxOfferIdLength = 128;

constexpr char kPublicKey[] = "public_key";
constexpr char kTokens[] = "tokens";

std::string ToJson(const base::ValueView& value) {
  std::string json;
  base::JSONWriter::Write(value, &json);
  return json;
}

// Event types name a path segment and a token pool.
bool IsValidEventType(const std::string& type) {
  return !type.empty() && type.size() <= kMaxEventTypeLength &&
         std::ranges::all_of(type, [](char c) {
           return base::IsAsciiLower(c) || base::IsAsciiDigit(c) || c == '-';
         });
}

bool IsValidOfferId(const std::string& offer_id) {
  return !offer_id.empty() && offer_id.size() <= kMaxOfferIdLength &&
         std::ranges::all_of(offer_id, [](char c) {
           return base::IsAsciiAlphaNumeric(c) || c == '-' || c == '_' ||
                  c == '.' || c == ':';
         });
}

// Base64 keys never contain ':', ',' or ';'. Two keys for one type allow a key
// rotation.
bool IsPinnedKey(const std::string& event_type, const std::string& key) {
  auto entries =
      base::SplitStringPiece(features::kOfferEventsPinnedKeys.Get(), ";",
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (auto entry : entries) {
    auto colon = entry.find(':');
    if (colon == std::string_view::npos ||
        entry.substr(0, colon) != event_type) {
      continue;
    }
    auto keys = base::SplitStringPiece(entry.substr(colon + 1), ",",
                                       base::TRIM_WHITESPACE,
                                       base::SPLIT_WANT_NONEMPTY);
    if (base::Contains(keys, key)) {
      return true;
    }
  }
  return false;
}

std::string EventPath(const std::string& event_type) {
  return "/v1/offer-events/" + event_type;
}

}  // namespace

OfferEvents::TypeState::TypeState() = default;
OfferEvents::TypeState::~TypeState() = default;

OfferEvents::OfferEvents(RewardsEngine& engine) : RewardsEngineHelper(engine) {}

OfferEvents::~OfferEvents() = default;

base::TimeDelta OfferEvents::SendDelay() const {
  if (send_delay_for_testing_) {
    return *send_delay_for_testing_;
  }
  auto min = std::clamp(features::kOfferEventsMinSendDelay.Get(),
                        kMinSendDelayFloor, kMaxSendDelayCeiling);
  auto max = std::clamp(features::kOfferEventsMaxSendDelay.Get(), min,
                        kMaxSendDelayCeiling);
  return base::RandTimeDelta(min, max + base::Microseconds(1));
}

GURL OfferEvents::ServiceUrl() const {
  GURL url(features::kOfferEventsServiceUrl.Get());
  if (url.is_valid() &&
      (url.SchemeIs(url::kHttpsScheme) || net::IsLocalhost(url))) {
    return url;
  }
  return Get<EnvironmentConfig>().rewards_api_url();
}

size_t OfferEvents::PendingCount() const {
  size_t count = scheduled_count_;
  for (auto& [type, state] : states_) {
    count += state.waiting_payloads.size();
  }
  return count;
}

void OfferEvents::SetOffers(const std::vector<mojom::UICardPtr>& cards) {
  offers_.clear();
  auto add = [this](const std::string& offer_id,
                    const std::vector<mojom::OfferEventSpecPtr>& events) {
    for (auto& spec : events) {
      if (IsValidOfferId(offer_id) && IsValidEventType(spec->type)) {
        offers_.emplace(offer_id, spec->type);
      }
    }
  };
  for (auto& card : cards) {
    if (card->banner) {
      add(card->banner->offer_id, card->banner->events);
    }
    for (auto& item : card->items) {
      add(item->offer_id, item->events);
    }
  }
}

void OfferEvents::RecordEvent(const std::string& offer_id,
                              const std::string& event_type) {
  if (!offers_.contains({offer_id, event_type})) {
    Log(FROM_HERE) << "Offer event is not in the current cards";
    return;
  }
  if (PendingCount() >= kMaxPendingEvents) {
    return;
  }
  // The payload holds only the offer ID, so it can't carry a user identifier.
  base::DictValue payload;
  payload.Set("offer_id", offer_id);
  ScheduleSend(event_type, ToJson(payload));
}

void OfferEvents::ScheduleSend(const std::string& event_type,
                               std::string payload) {
  ++scheduled_count_;
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&OfferEvents::OnSendTimer, weak_factory_.GetWeakPtr(),
                     event_type, std::move(payload)),
      SendDelay());
}

void OfferEvents::OnSendTimer(const std::string& event_type,
                              std::string payload) {
  CHECK_GT(scheduled_count_, 0u);
  --scheduled_count_;
  if (auto token = TakeToken(event_type)) {
    SendEvent(event_type, payload, *token);
    return;
  }
  auto& state = states_[event_type];
  if (base::TimeTicks::Now() < state.refill_blocked_until) {
    return;
  }
  state.waiting_payloads.push_back(std::move(payload));
  RequestTokens(event_type);
}

std::optional<std::string> OfferEvents::TakeToken(
    const std::string& event_type) {
  auto& prefs = Get<RewardsPrefs>();
  auto pools = prefs.GetDict(prefs::kOfferEventTokens).Clone();
  auto* pool = pools.FindDict(event_type);
  if (!pool) {
    return std::nullopt;
  }

  // Tokens signed by a key that is no longer pinned can't be spent.
  auto* key = pool->FindString(kPublicKey);
  auto* tokens = pool->FindList(kTokens);
  if (!key || !tokens || !IsPinnedKey(event_type, *key)) {
    Log(FROM_HERE) << "Dropping offer event tokens for an unpinned key";
    pools.Remove(event_type);
    prefs.SetDict(prefs::kOfferEventTokens, std::move(pools));
    return std::nullopt;
  }
  if (tokens->empty()) {
    return std::nullopt;
  }

  // A token leaves the pool before its event is sent, so a crash can lose an
  // event but can never spend a token twice.
  const std::string* back = tokens->back().GetIfString();
  if (!back) {
    pools.Remove(event_type);
    prefs.SetDict(prefs::kOfferEventTokens, std::move(pools));
    return std::nullopt;
  }
  std::string token = *back;
  tokens->erase(tokens->end() - 1);
  prefs.SetDict(prefs::kOfferEventTokens, std::move(pools));
  return token;
}

void OfferEvents::SendEvent(const std::string& event_type,
                            const std::string& payload,
                            const std::string& unblinded) {
  auto token = UnblindedToken::DecodeBase64(unblinded);
  if (!token.has_value()) {
    LogError(FROM_HERE) << "Failed to decode stored token";
    return;
  }

  // Only RFC 9497 tokens are supported. Token::Blind() always uses the RFC
  // derivation, so the verification key must match it.
  auto signature = token->DeriveVerificationKeyRfc().Sign(payload);
  if (!signature.has_value()) {
    LogError(FROM_HERE) << "Failed to sign event";
    return;
  }

  base::DictValue body;
  body.Set("payload", payload);
  body.Set("t", token->Preimage().EncodeBase64());
  body.Set("signature", signature->EncodeBase64());

  auto request = mojom::UrlRequest::New();
  request->method = mojom::UrlMethod::POST;
  request->url = ServiceUrl().Resolve(EventPath(event_type)).spec();
  request->content_type = "application/json";
  request->content = ToJson(body);
  request->skip_log = true;

  Get<URLLoader>().Load(std::move(request), URLLoader::LogLevel::kNone,
                        base::BindOnce(&OfferEvents::OnEventResponse,
                                       weak_factory_.GetWeakPtr()));
}

void OfferEvents::OnEventResponse(mojom::UrlResponsePtr response) {
  if (!URLLoader::IsSuccessCode(response->status_code)) {
    Log(FROM_HERE) << "Event rejected: " << response->status_code;
  }
}

void OfferEvents::RequestTokens(const std::string& event_type) {
  auto& state = states_[event_type];
  if (state.requesting_tokens) {
    return;
  }

  // The server refills tokens only for Rewards users, so the request is signed
  // with the Rewards wallet key.
  auto wallet = engine().wallet()->GetWallet();
  if (!wallet) {
    return FailRefill(event_type, "No Rewards wallet");
  }
  auto signer = RequestSigner::FromRewardsWallet(*wallet);
  if (!signer) {
    return FailRefill(event_type, "Unable to sign token request");
  }

  auto creds = credential::GenerateCreds(kTokenBatchSize);
  auto blinded = credential::GenerateBlindCreds(creds);
  if (blinded.empty()) {
    return FailRefill(event_type, "Failed to blind tokens");
  }

  base::ListValue encoded;
  for (auto& item : blinded) {
    encoded.Append(item.EncodeBase64());
  }
  base::DictValue body;
  body.Set("blinded_tokens", encoded.Clone());

  std::string path = EventPath(event_type) + "/tokens";
  auto request = mojom::UrlRequest::New();
  request->method = mojom::UrlMethod::POST;
  request->url = ServiceUrl().Resolve(path).spec();
  request->content_type = "application/json";
  request->content = ToJson(body);
  request->headers = signer->GetSignedHeaders("post " + path, request->content);
  request->skip_log = true;

  state.requesting_tokens = true;
  Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kNone,
      base::BindOnce(&OfferEvents::OnTokensResponse, weak_factory_.GetWeakPtr(),
                     event_type, credential::GetCredsJSON(creds),
                     ToJson(encoded)));
}

void OfferEvents::OnTokensResponse(const std::string& event_type,
                                   std::string creds_json,
                                   std::string blinded_creds_json,
                                   mojom::UrlResponsePtr response) {
  states_[event_type].requesting_tokens = false;

  if (response->status_code == net::HTTP_TOO_MANY_REQUESTS) {
    return FailRefill(event_type, "Token limit reached");
  }
  if (!URLLoader::IsSuccessCode(response->status_code)) {
    return FailRefill(event_type, "Token request failed");
  }

  auto dict = base::JSONReader::ReadDict(response->body,
                                         base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!dict) {
    return FailRefill(event_type, "Invalid token response");
  }
  const auto* proof = dict->FindString("batch_proof");
  const auto* public_key = dict->FindString(kPublicKey);
  const auto* signed_tokens = dict->FindList("signed_tokens");
  if (!proof || !public_key || !signed_tokens) {
    return FailRefill(event_type, "Incomplete token response");
  }

  // An unpinned key could be issued to one browser only and so identify that
  // browser's events.
  if (!IsPinnedKey(event_type, *public_key)) {
    return FailRefill(event_type, "Issuer key is not pinned");
  }

  auto batch = mojom::CredsBatch::New();
  batch->creds = std::move(creds_json);
  batch->blinded_creds = std::move(blinded_creds_json);
  batch->signed_creds = ToJson(*signed_tokens);
  batch->batch_proof = *proof;
  batch->public_key = *public_key;

  auto unblinded = credential::UnBlindCreds(*batch);
  if (!unblinded.has_value()) {
    LogError(FROM_HERE) << unblinded.error();
    return FailRefill(event_type, "Token proof verification failed");
  }

  // Each pool holds tokens from one key. A new key replaces the old tokens.
  auto& prefs = Get<RewardsPrefs>();
  auto pools = prefs.GetDict(prefs::kOfferEventTokens).Clone();
  auto* pool = pools.FindDict(event_type);
  if (!pool || !pool->FindList(kTokens) ||
      pool->FindString(kPublicKey) == nullptr ||
      *pool->FindString(kPublicKey) != *public_key) {
    pool = pools.Set(event_type, base::DictValue())->GetIfDict();
    pool->Set(kPublicKey, *public_key);
    pool->Set(kTokens, base::ListValue());
  }
  auto* tokens = pool->FindList(kTokens);
  for (auto& token : *unblinded) {
    tokens->Append(std::move(token));
  }
  prefs.SetDict(prefs::kOfferEventTokens, std::move(pools));

  // Waiting events get a fresh delay, so the server can't link a refill to
  // the spends that follow.
  auto waiting = std::move(states_[event_type].waiting_payloads);
  states_[event_type].waiting_payloads.clear();
  for (auto& payload : waiting) {
    ScheduleSend(event_type, std::move(payload));
  }
}

void OfferEvents::FailRefill(const std::string& event_type,
                             const char* reason) {
  Log(FROM_HERE) << reason;
  auto& state = states_[event_type];
  state.waiting_payloads.clear();
  state.refill_blocked_until = base::TimeTicks::Now() + kRefillBackoff;
}

}  // namespace brave_rewards::internal
