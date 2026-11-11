/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/offer_events/offer_events.h"

#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_rewards/core/engine/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"
#include "brave/components/brave_rewards/core/engine/util/rewards_prefs.h"
#include "brave/components/brave_rewards/core/engine/util/signer.h"
#include "brave/components/brave_rewards/core/engine/wallet/wallet.h"
#include "brave/components/brave_rewards/core/features.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/challenge_bypass_ristretto/public_key.h"
#include "brave/components/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/challenge_bypass_ristretto/signing_key.h"
#include "brave/components/challenge_bypass_ristretto/token.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {

using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::SigningKey;
using challenge_bypass_ristretto::Token;

class RewardsOfferEventsTest : public RewardsEngineTest {
 protected:
  void SetUp() override {
    auto wallet = mojom::RewardsWallet::New();
    wallet->payment_id = "a1ca63f7-e2be-5f4c-9c01-2fec88f08c28";
    wallet->recovery_seed = Signer::GenerateRecoverySeed();
    ASSERT_TRUE(engine().wallet()->SetWallet(std::move(wallet)));

    PinKey(KeyOf(issuer_key_));
    SetOffers({"offer-1", "offer-2"});
    engine().Get<OfferEvents>().SetSendDelayForTesting(base::TimeDelta());
  }

  // Lists each offer in /v1/cards with one event of `type`.
  void SetOffers(const std::vector<std::string>& offer_ids,
                 const std::string& type = "hover") {
    auto card = mojom::UICard::New();
    for (auto& offer_id : offer_ids) {
      auto spec = mojom::OfferEventSpec::New();
      spec->type = type;
      spec->trigger = mojom::OfferEventTrigger::kHover;
      auto item = mojom::UICardItem::New();
      item->offer_id = offer_id;
      item->events.push_back(std::move(spec));
      card->items.push_back(std::move(item));
    }
    std::vector<mojom::UICardPtr> cards;
    cards.push_back(std::move(card));
    engine().Get<OfferEvents>().SetOffers(cards);
  }

  static std::string KeyOf(SigningKey& key) {
    return key.GetPublicKey().EncodeBase64();
  }

  void PinKey(const std::string& key) {
    feature_list_.Reset();
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kOfferEventsFeature,
        {{"pinned_keys", "hover:" + key}, {"token_batch_size", "3"}});
  }

  std::string Url(const std::string& path) {
    return engine()
        .Get<EnvironmentConfig>()
        .rewards_api_url()
        .Resolve(path)
        .spec();
  }

  // Answers the next refill the way CBP does, signing with `signing_key`.
  void ExpectRefill(SigningKey signing_key, int status = net::HTTP_OK) {
    client().AddNetworkResultForTesting(
        Url("/v1/offer-events/hover/tokens"), mojom::UrlMethod::POST,
        base::BindLambdaForTesting([this, signing_key,
                                    status](const mojom::UrlRequest& request) {
          refill_headers_ = request.headers;
          auto response = mojom::UrlResponse::New();
          response->status_code = status;
          if (status != net::HTTP_OK) {
            return response;
          }
          auto body = base::JSONReader::ReadDict(request.content);
          std::vector<BlindedToken> blinded;
          std::vector<SignedToken> signed_tokens;
          base::ListValue signed_list;
          for (auto& value : *body->FindList("blinded_tokens")) {
            auto token = BlindedToken::DecodeBase64(value.GetString()).value();
            auto signed_token = signing_key.Sign(token).value();
            signed_list.Append(signed_token.EncodeBase64());
            blinded.push_back(std::move(token));
            signed_tokens.push_back(std::move(signed_token));
          }
          auto proof =
              BatchDLEQProof::Create(blinded, signed_tokens, signing_key)
                  .value();
          auto public_key = signing_key;
          base::DictValue result;
          result.Set("batch_proof", proof.EncodeBase64());
          result.Set("public_key", KeyOf(public_key));
          result.Set("signed_tokens", std::move(signed_list));
          base::JSONWriter::Write(result, &response->body);
          return response;
        }));
  }

  void ExpectEvent() {
    client().AddNetworkResultForTesting(
        Url("/v1/offer-events/hover"), mojom::UrlMethod::POST,
        base::BindLambdaForTesting([this](const mojom::UrlRequest& request) {
          events_.push_back(request.content);
          auto response = mojom::UrlResponse::New();
          response->status_code = net::HTTP_OK;
          return response;
        }));
  }

  void Record(const std::string& offer_id, const std::string& type = "hover") {
    engine().Get<OfferEvents>().RecordEvent(offer_id, type);
    task_environment().RunUntilIdle();
  }

  const base::DictValue* Pool() {
    return engine()
        .Get<RewardsPrefs>()
        .GetDict(prefs::kOfferEventTokens)
        .FindDict("hover");
  }

  size_t PoolSize() {
    auto* pool = Pool();
    return pool ? pool->FindList("tokens")->size() : 0;
  }

  base::test::ScopedFeatureList feature_list_;
  SigningKey issuer_key_ = SigningKey::Random();
  std::vector<std::string> refill_headers_;
  std::vector<std::string> events_;
};

TEST_F(RewardsOfferEventsTest, RefillsSignedAndSpendsOneToken) {
  ExpectRefill(issuer_key_);
  ExpectEvent();
  Record("offer-1");

  // The refill is signed with the Rewards wallet key.
  EXPECT_FALSE(refill_headers_.empty());

  ASSERT_EQ(events_.size(), 1u);
  auto body = base::JSONReader::ReadDict(events_[0]);
  ASSERT_TRUE(body);
  EXPECT_EQ(*body->FindString("payload"), R"({"offer_id":"offer-1"})");
  EXPECT_TRUE(body->FindString("t"));
  EXPECT_TRUE(body->FindString("signature"));

  EXPECT_EQ(*Pool()->FindString("public_key"), KeyOf(issuer_key_));
  EXPECT_EQ(PoolSize(), 2u);

  // The next event spends from the pool without a refill.
  ExpectEvent();
  Record("offer-2");
  EXPECT_EQ(events_.size(), 2u);
  EXPECT_EQ(PoolSize(), 1u);
}

TEST_F(RewardsOfferEventsTest, RefusesUnpinnedKeyAndBacksOff) {
  ExpectRefill(SigningKey::Random());
  ExpectEvent();
  Record("offer-1");
  EXPECT_TRUE(events_.empty());
  EXPECT_EQ(PoolSize(), 0u);

  // During the backoff, events are dropped without another refill.
  refill_headers_.clear();
  ExpectRefill(issuer_key_);
  Record("offer-2");
  EXPECT_TRUE(refill_headers_.empty());
  EXPECT_TRUE(events_.empty());
}

TEST_F(RewardsOfferEventsTest, DropsPoolWhenKeyIsUnpinned) {
  ExpectRefill(issuer_key_);
  ExpectEvent();
  Record("offer-1");
  ASSERT_EQ(PoolSize(), 2u);

  // The pinned key changes, so the old tokens can't be spent.
  PinKey(KeyOf(issuer_key_ = SigningKey::Random()));
  ExpectRefill(issuer_key_);
  ExpectEvent();
  Record("offer-2");
  ASSERT_EQ(events_.size(), 2u);
  EXPECT_EQ(*Pool()->FindString("public_key"), KeyOf(issuer_key_));
  EXPECT_EQ(PoolSize(), 2u);
}

TEST_F(RewardsOfferEventsTest, MalformedRefillResponseStoresNoTokens) {
  // A crafted response must fail the refill, not crash the engine.
  client().AddNetworkResultForTesting(
      Url("/v1/offer-events/hover/tokens"), mojom::UrlMethod::POST,
      base::BindLambdaForTesting([this](const mojom::UrlRequest& request) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        base::DictValue body;
        // A valid proof, so parsing reaches the bad signed tokens.
        auto blinded = Token::Random().Blind().value();
        auto signed_token = issuer_key_.Sign(blinded).value();
        auto proof =
            BatchDLEQProof::Create({blinded}, {signed_token}, issuer_key_)
                .value();
        body.Set("batch_proof", proof.EncodeBase64());
        body.Set("public_key", KeyOf(issuer_key_));
        body.Set("signed_tokens", base::ListValue().Append(1).Append(2));
        base::JSONWriter::Write(body, &response->body);
        return response;
      }));
  ExpectEvent();
  Record("offer-1");
  EXPECT_TRUE(events_.empty());
  EXPECT_EQ(PoolSize(), 0u);
}

TEST_F(RewardsOfferEventsTest, RefillLimitResponseDropsWaitingEvents) {
  ExpectRefill(issuer_key_, net::HTTP_TOO_MANY_REQUESTS);
  ExpectEvent();
  Record("offer-1");
  EXPECT_TRUE(events_.empty());
  EXPECT_EQ(PoolSize(), 0u);
}

TEST_F(RewardsOfferEventsTest, DropsEventsOverPendingLimit) {
  feature_list_.Reset();
  feature_list_.InitAndEnableFeatureWithParameters(
      features::kOfferEventsFeature,
      {{"pinned_keys", "hover:" + KeyOf(issuer_key_)},
       {"max_pending_events", "1"}});
  ExpectRefill(issuer_key_);
  ExpectEvent();
  ExpectEvent();
  engine().Get<OfferEvents>().RecordEvent("offer-1", "hover");
  engine().Get<OfferEvents>().RecordEvent("offer-2", "hover");
  task_environment().RunUntilIdle();
  ASSERT_EQ(events_.size(), 1u);
  EXPECT_NE(events_[0].find("offer-1"), std::string::npos);
}

TEST_F(RewardsOfferEventsTest, DropsEventsNotInCards) {
  ExpectRefill(issuer_key_);
  Record("offer-3");
  Record("offer-1", "click");
  EXPECT_TRUE(refill_headers_.empty());
}

TEST_F(RewardsOfferEventsTest, IgnoresMalformedOffersInCards) {
  ExpectRefill(issuer_key_);
  SetOffers({"offer 1", ""});
  Record("offer 1");
  Record("");
  SetOffers({"offer-1"}, "../hover");
  Record("offer-1", "../hover");
  EXPECT_TRUE(refill_headers_.empty());
}

}  // namespace brave_rewards::internal
