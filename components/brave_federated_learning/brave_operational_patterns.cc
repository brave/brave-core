/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "brave/components/brave_federated_learning/brave_operational_patterns.h"

#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_federated_learning/brave_operational_patterns_features.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace brave {

namespace {

static constexpr char federatedLearningUrl[] = "https://fl.brave.com/";

constexpr char kLastCheckedSlotPrefName[] = "brave.federated.last_checked_slot";
constexpr char kCollectionIdPrefName[] = "brave.federated.collection_id";
constexpr char kCollectionIdExpirationPrefName[] =
    "brave.federated.collection_id_expiration";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_operational_patterns", R"(
        semantics {
          sender: "Operational Patterns Service"
          description:
            "Report of anonymized engagement statistics. For more info see "
            "https://github.com/brave/brave-browser/wiki/Operational-Patterns"
          trigger:
            "Reports are automatically generated on startup and at intervals "
            "while Brave is running."
          data:
            "Anonymized and encrypted engagement data."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This service is enabled only when P3A is enabled."
          policy_exception_justification:
            "Not implemented."
        }
    )");
}

}  // anonymous namespace

BraveOperationalPatterns::BraveOperationalPatterns(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : local_state_(pref_service), url_loader_factory_(url_loader_factory) {}

BraveOperationalPatterns::~BraveOperationalPatterns() {}

void BraveOperationalPatterns::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kLastCheckedSlotPrefName, -1);
  registry->RegisterStringPref(kCollectionIdPrefName, {});
  registry->RegisterTimePref(kCollectionIdExpirationPrefName, base::Time());
}

void BraveOperationalPatterns::Start() {
  DCHECK(!simulate_local_training_step_timer_);
  DCHECK(!collection_slot_periodic_timer_);

  LoadPrefs();
  MaybeResetCollectionId();

  simulate_local_training_step_timer_ =
      std::make_unique<base::RetainingOneShotTimer>();
  simulate_local_training_step_timer_->Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(
          operational_patterns::features::
              GetSimulateLocalTrainingStepDurationValue() *
          60),
      this, &BraveOperationalPatterns::OnSimulateLocalTrainingStepTimerFired);

  collection_slot_periodic_timer_ = std::make_unique<base::RepeatingTimer>();
  collection_slot_periodic_timer_->Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(
          operational_patterns::features::GetCollectionSlotSizeValue() * 60 /
          2),
      this, &BraveOperationalPatterns::OnCollectionSlotStartTimerFired);
}

void BraveOperationalPatterns::Stop() {
  simulate_local_training_step_timer_.reset();
  collection_slot_periodic_timer_.reset();
}

void BraveOperationalPatterns::LoadPrefs() {
  last_checked_slot_ = local_state_->GetInteger(kLastCheckedSlotPrefName);
  collection_id_ = local_state_->GetString(kCollectionIdPrefName);
  collection_id_expiration_time_ =
      local_state_->GetTime(kCollectionIdExpirationPrefName);
}

void BraveOperationalPatterns::SavePrefs() {
  local_state_->SetInteger(kLastCheckedSlotPrefName, last_checked_slot_);
  local_state_->SetString(kCollectionIdPrefName, collection_id_);
  local_state_->SetTime(kCollectionIdExpirationPrefName,
                        collection_id_expiration_time_);
}

void BraveOperationalPatterns::OnCollectionSlotStartTimerFired() {
  simulate_local_training_step_timer_->Reset();
}

void BraveOperationalPatterns::OnSimulateLocalTrainingStepTimerFired() {
  SendCollectionSlot();
}

void BraveOperationalPatterns::SendCollectionSlot() {
  current_collected_slot_ = GetCurrentCollectionSlot();
  if (current_collected_slot_ == last_checked_slot_) {
    return;
  }

  MaybeResetCollectionId();

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(federatedLearningUrl);
  resource_request->headers.SetHeader("X-Brave-FL-Operational-Patterns", "?1");

  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->method = "POST";

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(BuildPayload(), "application/json");

  url_loader_->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&BraveOperationalPatterns::OnUploadComplete,
                     base::Unretained(this)));
}

void BraveOperationalPatterns::OnUploadComplete(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  int response_code = -1;
  if (headers)
    response_code = headers->response_code();
  if (response_code == 200) {
    last_checked_slot_ = current_collected_slot_;
    SavePrefs();
  }
}

std::string BraveOperationalPatterns::BuildPayload() const {
  base::Value root(base::Value::Type::DICTIONARY);

  root.SetKey("collection_id", base::Value(collection_id_));
  root.SetKey("platform", base::Value(brave_stats::GetPlatformIdentifier()));
  root.SetKey("collection_slot", base::Value(current_collected_slot_));
  root.SetKey("wiki-link", base::Value("https://github.com/brave/brave-browser/"
                                       "wiki/Operational-Patterns"));

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

int BraveOperationalPatterns::GetCurrentCollectionSlot() const {
  base::Time::Exploded now;
  base::Time::Now().LocalExplode(&now);

  return ((now.day_of_month - 1) * 24 * 60 + now.hour * 60 + now.minute) /
         operational_patterns::features::GetCollectionSlotSizeValue();
}

void BraveOperationalPatterns::MaybeResetCollectionId() {
  const base::Time now = base::Time::Now();
  if (collection_id_.empty() || (!collection_id_expiration_time_.is_null() &&
                                 now > collection_id_expiration_time_)) {
    collection_id_ =
        base::ToUpperASCII(base::UnguessableToken::Create().ToString());
    collection_id_expiration_time_ =
        now + base::TimeDelta::FromSeconds(
                  operational_patterns::features::GetCollectionIdLifetime() *
                  24 * 60 * 60);
    SavePrefs();
  }
}

}  // namespace brave
