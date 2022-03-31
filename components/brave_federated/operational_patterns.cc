/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "brave/components/brave_federated/operational_patterns.h"

#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace {

constexpr int kDefaultResponseCode = -1;

static constexpr char federatedLearningUrl[] = "https://fl.brave.com/";

constexpr char kLastCheckedSlotPrefName[] = "brave.federated.last_checked_slot";
constexpr char kCollectionIdPrefName[] = "brave.federated.collection_id";
constexpr char kCollectionIdExpirationPrefName[] =
    "brave.federated.collection_id_expiration";

constexpr int kMinutesBeforeRetry = 5;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("operational_pattern", R"(
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

namespace brave_federated {

OperationalPatterns::OperationalPatterns(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : pref_service_(pref_service), url_loader_factory_(url_loader_factory) {
  DCHECK(pref_service_);
  DCHECK(url_loader_factory_);
}

OperationalPatterns::~OperationalPatterns() {}

void OperationalPatterns::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kLastCheckedSlotPrefName, kDefaultResponseCode);
  registry->RegisterStringPref(kCollectionIdPrefName, {});
  registry->RegisterTimePref(kCollectionIdExpirationPrefName, base::Time());
}

void OperationalPatterns::Start() {
  DCHECK(!simulate_local_training_step_timer_);
  DCHECK(!collection_slot_periodic_timer_);

  LoadPrefs();
  MaybeResetCollectionId();

  simulate_local_training_step_timer_ =
      std::make_unique<base::RetainingOneShotTimer>();
  simulate_local_training_step_timer_->Start(
      FROM_HERE,
      base::Seconds(brave_federated::features::
                        GetSimulateLocalTrainingStepDurationValue() *
                    60),
      this, &OperationalPatterns::OnSimulateLocalTrainingStepTimerFired);

  collection_slot_periodic_timer_ = std::make_unique<base::RepeatingTimer>();
  collection_slot_periodic_timer_->Start(
      FROM_HERE,
      base::Seconds(brave_federated::features::GetCollectionSlotSizeValue() *
                    60 / 2),
      this, &OperationalPatterns::OnCollectionSlotStartTimerFired);
}

void OperationalPatterns::Stop() {
  simulate_local_training_step_timer_.reset();
  collection_slot_periodic_timer_.reset();

  SendDelete();
}

void OperationalPatterns::LoadPrefs() {
  last_checked_slot_ = pref_service_->GetInteger(kLastCheckedSlotPrefName);
  collection_id_ = pref_service_->GetString(kCollectionIdPrefName);
  collection_id_expiration_time_ =
      pref_service_->GetTime(kCollectionIdExpirationPrefName);
}

void OperationalPatterns::SavePrefs() {
  pref_service_->SetInteger(kLastCheckedSlotPrefName, last_checked_slot_);
  pref_service_->SetString(kCollectionIdPrefName, collection_id_);
  pref_service_->SetTime(kCollectionIdExpirationPrefName,
                         collection_id_expiration_time_);
}

void OperationalPatterns::ClearPrefs() {
  pref_service_->ClearPref(kLastCheckedSlotPrefName);
  pref_service_->ClearPref(kCollectionIdPrefName);
  pref_service_->ClearPref(kCollectionIdExpirationPrefName);
}

void OperationalPatterns::OnCollectionSlotStartTimerFired() {
  simulate_local_training_step_timer_->Reset();
}

void OperationalPatterns::OnSimulateLocalTrainingStepTimerFired() {
  SendCollectionSlot();
}

void OperationalPatterns::PrepareSend(
    std::unique_ptr<network::ResourceRequest> resource_request,
    std::string payload) {
  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/json");
}

void OperationalPatterns::SendCollectionSlot() {
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

  PrepareSend(std::move(resource_request), BuildPayload());

  url_loader_->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&OperationalPatterns::OnCollectionSlotUploadComplete,
                     base::Unretained(this)));
}

void OperationalPatterns::SendDelete() {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(federatedLearningUrl);
  resource_request->headers.SetHeader("X-Brave-FL-Operational-Patterns", "?1");
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->method = "DELETE";

  PrepareSend(std::move(resource_request), BuildDeletePayload());

  url_loader_->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&OperationalPatterns::OnDeleteUploadComplete,
                     base::Unretained(this)));
}

void OperationalPatterns::OnCollectionSlotUploadComplete(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  int response_code = kDefaultResponseCode;
  if (headers)
    response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    last_checked_slot_ = current_collected_slot_;
    SavePrefs();
  }
}

void OperationalPatterns::OnDeleteUploadComplete(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  int response_code = kDefaultResponseCode;
  if (headers)
    response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    ClearPrefs();
  } else {
    auto retry_timer = std::make_unique<base::RetainingOneShotTimer>();
    retry_timer->Start(FROM_HERE, base::Seconds(kMinutesBeforeRetry * 60), this,
                       &OperationalPatterns::SendDelete);
  }
}

std::string OperationalPatterns::BuildPayload() const {
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

std::string OperationalPatterns::BuildDeletePayload() const {
  base::Value root(base::Value::Type::DICTIONARY);

  root.SetKey("collection_id", base::Value(collection_id_));
  root.SetKey("wiki-link", base::Value("https://github.com/brave/brave-browser/"
                                       "wiki/Operational-Patterns"));

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

int OperationalPatterns::GetCurrentCollectionSlot() const {
  base::Time::Exploded now;
  base::Time::Now().LocalExplode(&now);

  return ((now.day_of_month - 1) * 24 * 60 + now.hour * 60 + now.minute) /
         brave_federated::features::GetCollectionSlotSizeValue();
}

void OperationalPatterns::MaybeResetCollectionId() {
  const base::Time now = base::Time::Now();
  if (collection_id_.empty() || (!collection_id_expiration_time_.is_null() &&
                                 now > collection_id_expiration_time_)) {
    ResetCollectionId();
  }
}

void OperationalPatterns::ResetCollectionId() {
  const base::Time now = base::Time::Now();
  collection_id_ =
      base::ToUpperASCII(base::UnguessableToken::Create().ToString());
  collection_id_expiration_time_ =
      now + base::Seconds(brave_federated::features::GetCollectionIdLifetime() *
                          24 * 60 * 60);
  SavePrefs();
}

}  // namespace brave_federated
