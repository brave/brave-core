/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/publisher/publisher_service.h"

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/future_join.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {

namespace {

class GetPublisherJob : public BATLedgerJob<absl::optional<Publisher>> {
 public:
  void Start(const std::string& id) {
    context().GetLedgerImpl()->publisher()->GetServerPublisherInfo(
        id, ContinueWithLambda(this, &GetPublisherJob::OnPublisherInfo));
  }

 private:
  void OnPublisherInfo(mojom::ServerPublisherInfoPtr publisher) {
    if (!publisher) {
      return Complete({});
    }

    Publisher p;
    p.id = publisher->publisher_key;
    p.registered = publisher->status != mojom::PublisherStatus::NOT_VERIFIED;

    absl::optional<ExternalWalletProvider> provider;

    switch (publisher->status) {
      case mojom::PublisherStatus::NOT_VERIFIED:
      case mojom::PublisherStatus::CONNECTED:
        break;
      case mojom::PublisherStatus::UPHOLD_VERIFIED:
        provider = ExternalWalletProvider::kUphold;
        break;
      case mojom::PublisherStatus::BITFLYER_VERIFIED:
        provider = ExternalWalletProvider::kBitflyer;
        break;
      case mojom::PublisherStatus::GEMINI_VERIFIED:
        provider = ExternalWalletProvider::kGemini;
        break;
    }

    if (provider && !publisher->address.empty()) {
      p.wallets.push_back(
          ExternalWallet{.provider = *provider, .address = publisher->address});
    }

    Complete(std::move(p));
  }
};

class GetPublishersJob : public BATLedgerJob<std::map<std::string, Publisher>> {
 public:
  void Start(const std::vector<std::string>& publisher_ids) {
    std::vector<Future<absl::optional<Publisher>>> futures;
    for (auto& id : publisher_ids) {
      futures.push_back(context().StartJob<GetPublisherJob>(id));
    }
    JoinFutures(std::move(futures))
        .Then(ContinueWith(this, &GetPublishersJob::OnPublishersLoaded));
  }

 private:
  void OnPublishersLoaded(std::vector<absl::optional<Publisher>> publishers) {
    std::map<std::string, Publisher> result;
    for (auto& p : publishers) {
      if (p) {
        result[p->id] = std::move(*p);
      }
    }
    Complete(std::move(result));
  }
};

}  // namespace

Future<absl::optional<Publisher>> PublisherService::GetPublisher(
    const std::string& publisher_id) {
  return context().StartJob<GetPublisherJob>(publisher_id);
}

Future<std::map<std::string, Publisher>> PublisherService::GetPublishers(
    const std::vector<std::string>& publisher_ids) {
  return context().StartJob<GetPublishersJob>(publisher_ids);
}

}  // namespace ledger
