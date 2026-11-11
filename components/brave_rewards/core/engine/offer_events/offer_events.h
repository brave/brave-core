/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_OFFER_EVENTS_OFFER_EVENTS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_OFFER_EVENTS_OFFER_EVENTS_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/brave_rewards/core/engine/rewards_engine_helper.h"
#include "brave/components/brave_rewards/core/mojom/rewards.mojom.h"
#include "url/gurl.h"

namespace brave_rewards::internal {

// Responsible for reporting offer wall events. Each event spends one blinded
// token from the pool for its event type, so the server can count events
// without learning who sent them.
class OfferEvents : public RewardsEngineHelper,
                    public WithHelperKey<OfferEvents> {
 public:
  explicit OfferEvents(RewardsEngine& engine);
  ~OfferEvents() override;

  // Queues an event for `offer_id` and sends it after a random delay. Drops
  // the event if SetOffers() did not list it or the pending limit is reached.
  void RecordEvent(const std::string& offer_id, const std::string& event_type);

  // Sets the offer events the page may report, from the latest /v1/cards
  // response. RecordEvent() drops any other offer and event type.
  void SetOffers(const std::vector<mojom::UICardPtr>& cards);

  void SetSendDelayForTesting(base::TimeDelta delay) {
    send_delay_for_testing_ = delay;
  }
  size_t PendingCountForTesting() const { return PendingCount(); }

 private:
  struct TypeState {
    TypeState();
    ~TypeState();

    // Events whose delay has passed but that have no token yet.
    std::vector<std::string> waiting_payloads;
    bool requesting_tokens = false;
    base::TimeTicks refill_blocked_until;
  };

  base::TimeDelta SendDelay() const;
  GURL ServiceUrl() const;
  size_t PendingCount() const;

  void ScheduleSend(const std::string& event_type, std::string payload);
  void OnSendTimer(const std::string& event_type, std::string payload);
  std::optional<std::string> TakeToken(const std::string& event_type);
  void SendEvent(const std::string& event_type,
                 const std::string& payload,
                 const std::string& unblinded);
  void OnEventResponse(mojom::UrlResponsePtr response);

  void RequestTokens(const std::string& event_type);
  void OnTokensResponse(const std::string& event_type,
                        std::string creds_json,
                        std::string blinded_creds_json,
                        mojom::UrlResponsePtr response);
  void FailRefill(const std::string& event_type, const char* reason);

  base::flat_map<std::string, TypeState> states_;
  // Pairs of offer ID and event type.
  base::flat_set<std::pair<std::string, std::string>> offers_;
  size_t scheduled_count_ = 0;
  std::optional<base::TimeDelta> send_delay_for_testing_;
  base::WeakPtrFactory<OfferEvents> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_OFFER_EVENTS_OFFER_EVENTS_H_
