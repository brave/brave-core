/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "brave/components/brave_federated/operational_patterns.h"

#include "base/i18n/time_formatting.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/operational_patterns_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace {

static constexpr char kCollectionEndpoint[] = "https://fl.brave.com/";

constexpr char kLastSentSlotPrefName[] = "brave.federated.last_checked_slot";
constexpr char kCollectionIdPrefName[] = "brave.federated.collection_id";
constexpr char kCollectionIdExpirationPrefName[] =
    "brave.federated.collection_id_expiration";

constexpr int kLastSentSlotInitValue = -1;

constexpr int kSecondsBeforeRetry = 60;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("operational_pattern", R"(
        semantics {
          sender: "Operational Patterns"
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
            "This service is enabled only when opted in to ads and having "
            "P3A is enabled."
          policy_exception_justification:
            "Not implemented."
        }
    )");
}

}  // namespace

namespace brave_federated {

OperationalPatterns::OperationalPatterns(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : pref_service_(pref_service), url_loader_factory_(url_loader_factory) {
  DCHECK(pref_service_);
  DCHECK(url_loader_factory_);
}

OperationalPatterns::~OperationalPatterns() = default;

void OperationalPatterns::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kLastSentSlotPrefName, kLastSentSlotInitValue);
  registry->RegisterStringPref(kCollectionIdPrefName, {});
  registry->RegisterTimePref(kCollectionIdExpirationPrefName, base::Time());
}

void OperationalPatterns::Start() {
  DCHECK(!mock_task_timer_);
  DCHECK(!collection_timer_);

  const int collection_id_lifetime =
      brave_federated::features::GetCollectionIdLifetimeInSeconds();
  const int collection_slot_size =
      brave_federated::features::GetCollectionSlotSizeInSeconds();
  const int collection_timer_interval =
      brave_federated::features::GetCollectionTimerIntervalInSeconds();
  const int mock_training_duration =
      brave_federated::features::GetMockTaskDurationInSeconds();
  const bool mock_collection_requests = features::MockCollectionRequests();

  VLOG(1) << "Starting operational patterns with:\n"
          << " collection_id_lifetime=" << collection_id_lifetime << "s\n"
          << " collection_slot_size=" << collection_slot_size << "s\n"
          << " collection_timer_interval=" << collection_timer_interval << "s\n"
          << " mock_training_duration=" << mock_training_duration << "s\n"
          << " mock_collection_requests=" << mock_collection_requests;

  is_running_ = true;

  LoadPrefs();

  MaybeResetCollectionId();

  StartRepeatingCollectionTimer();
  StartMockTaskTimer();
}

void OperationalPatterns::Stop() {
  DCHECK(mock_task_timer_);
  DCHECK(collection_timer_);

  VLOG(1) << "Stopping operational patterns";
  is_running_ = false;

  StopRepeatingCollectionTimer();
  StopMockTaskTimer();

  SendDeletePing();
}

bool OperationalPatterns::IsRunning() {
  return is_running_;
}

///////////////////////////////////////////////////////////////////////////////

void OperationalPatterns::LoadPrefs() {
  VLOG(2) << "Loading preferences";

  last_sent_slot_ = pref_service_->GetInteger(kLastSentSlotPrefName);
  collection_id_ = pref_service_->GetString(kCollectionIdPrefName);
  collection_id_expiration_time_ =
      pref_service_->GetTime(kCollectionIdExpirationPrefName);
}

void OperationalPatterns::SavePrefs() {
  VLOG(2) << "Saving preferences";

  pref_service_->SetInteger(kLastSentSlotPrefName, last_sent_slot_);
  pref_service_->SetString(kCollectionIdPrefName, collection_id_);
  pref_service_->SetTime(kCollectionIdExpirationPrefName,
                         collection_id_expiration_time_);
}

void OperationalPatterns::ClearPrefs() {
  VLOG(2) << "Clearing preferences";

  pref_service_->ClearPref(kLastSentSlotPrefName);
  pref_service_->ClearPref(kCollectionIdPrefName);
  pref_service_->ClearPref(kCollectionIdExpirationPrefName);
}

// Repeating Collection Timer -------------------------------------------------

void OperationalPatterns::StartRepeatingCollectionTimer() {
  const int collection_slot = GetCollectionSlot();
  VLOG(2) << "Start Repeating Collection Timer in slot " << collection_slot;

  const int collection_timer_interval_in_seconds =
      brave_federated::features::GetCollectionTimerIntervalInSeconds();
  collection_timer_ = std::make_unique<base::RepeatingTimer>();
  collection_timer_->Start(
      FROM_HERE, base::Seconds(collection_timer_interval_in_seconds), this,
      &OperationalPatterns::OnRepeatingCollectionTimerFired);
}

void OperationalPatterns::OnRepeatingCollectionTimerFired() {
  const int collection_slot = GetCollectionSlot();
  VLOG(1) << base::TimeFormatShortDateAndTime(base::Time::Now())
          << " Repeating Collection Timer Fired in slot " << collection_slot;

  MaybeResetCollectionId();

  MaybeRestartMockTaskTimer();
}

void OperationalPatterns::StopRepeatingCollectionTimer() {
  DCHECK(collection_timer_);

  VLOG(2) << "Stop Repeating Collection Timer";

  collection_timer_.reset();
}

// Mock Task Timer ------------------------------------------------------------

void OperationalPatterns::StartMockTaskTimer() {
  const int collection_slot = GetCollectionSlot();
  VLOG(2) << "Start Mock Task Timer in slot " << collection_slot;

  const int mock_training_duration_in_seconds =
      brave_federated::features::GetMockTaskDurationInSeconds();
  mock_task_timer_ = std::make_unique<base::RetainingOneShotTimer>();
  mock_task_timer_->Start(FROM_HERE,
                          base::Seconds(mock_training_duration_in_seconds),
                          this, &OperationalPatterns::OnMockTaskTimerFired);
}

void OperationalPatterns::OnMockTaskTimerFired() {
  const int collection_slot = GetCollectionSlot();
  if (last_sent_slot_ == collection_slot) {
    VLOG(1) << base::TimeFormatShortDateAndTime(base::Time::Now())
            << " Mock Task Timer Fired in slot " << collection_slot
            << ", but Collection Ping already sent";
    return;
  }

  VLOG(1) << base::TimeFormatShortDateAndTime(base::Time::Now())
          << " Mock Task Timer Fired in slot " << collection_slot;

  SendCollectionPing(collection_slot);
}

void OperationalPatterns::StopMockTaskTimer() {
  DCHECK(mock_task_timer_);

  VLOG(2) << "Stop Mock Task Timer";

  mock_task_timer_.reset();
}

void OperationalPatterns::MaybeRestartMockTaskTimer() {
  DCHECK(mock_task_timer_);

  if (mock_task_timer_->IsRunning()) {
    VLOG(2) << "Mock Task Timer already running";
    return;
  }

  const int collection_slot = GetCollectionSlot();
  VLOG(2) << "Restart Mock Task Timer in slot " << collection_slot;

  mock_task_timer_->Reset();
}

// Collection Ping ------------------------------------------------------------

void OperationalPatterns::SendCollectionPing(int slot) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(kCollectionEndpoint);
  request->headers.SetHeader("X-Brave-FL-Operational-Patterns", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kPostMethod;

  VLOG(2) << "Send Collection Ping " << request->method << " " << request->url;

  sending_slot_ = slot;
  const std::string payload =
      BuildCollectionPingPayload(collection_id_, sending_slot_);

  VLOG(2) << "Payload " << payload;

  if (brave_federated::features::MockCollectionRequests()) {
    OnCollectionPingSendSuccess();
    return;
  }

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/json");
  url_loader_->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&OperationalPatterns::OnCollectionPingSend,
                     base::Unretained(this)));
}

void OperationalPatterns::OnCollectionPingSend(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  if (!headers) {
    VLOG(1) << "Failed to send collection ping";
    return;
  }

  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    OnCollectionPingSendSuccess();
    return;
  }

  VLOG(1) << "Failed to send collection ping with HTTP " << response_code;
}

void OperationalPatterns::OnCollectionPingSendSuccess() {
  VLOG(1) << "Successfully sent collection ping for slot " << sending_slot_;

  last_sent_slot_ = sending_slot_;
  SavePrefs();
}

// Delete Ping ---------------------------------------------------------------

void OperationalPatterns::SendDeletePing() {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(kCollectionEndpoint);
  request->headers.SetHeader("X-Brave-FL-Operational-Patterns", "?1");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kDeleteMethod;

  VLOG(2) << "Send Delete Ping " << request->method << " " << request->url;

  const std::string payload = BuildDeletePingPayload(collection_id_);

  VLOG(2) << "Payload " << payload;

  if (brave_federated::features::MockCollectionRequests()) {
    OnDeletePingSendSuccess();
    return;
  }

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->AttachStringForUpload(payload, "application/json");
  url_loader_->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&OperationalPatterns::OnDeletePingSend,
                     base::Unretained(this)));
}

void OperationalPatterns::OnDeletePingSend(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  if (!headers) {
    VLOG(1) << "Failed to send delete ping";
    return;
  }

  int response_code = headers->response_code();
  if (response_code == net::HTTP_OK) {
    OnDeletePingSendSuccess();
    return;
  }

  VLOG(1) << "Failed to send delete ping with HTTP " << response_code;

  auto retry_timer = std::make_unique<base::OneShotTimer>();
  retry_timer->Start(FROM_HERE, base::Seconds(kSecondsBeforeRetry), this,
                     &OperationalPatterns::SendDeletePing);

  VLOG(1) << "Retry in " << kSecondsBeforeRetry << "s";
}

void OperationalPatterns::OnDeletePingSendSuccess() {
  VLOG(1) << "Successfully sent delete ping";

  ClearPrefs();
}

// Collection ID --------------------------------------------------------------

void OperationalPatterns::MaybeResetCollectionId() {
  if (!ShouldResetCollectionId(collection_id_,
                               collection_id_expiration_time_)) {
    return;
  }

  ResetCollectionId();
}

void OperationalPatterns::ResetCollectionId() {
  collection_id_ = CreateCollectionId();
  const int collection_id_lifetime_in_seconds =
      brave_federated::features::GetCollectionIdLifetimeInSeconds();
  collection_id_expiration_time_ =
      base::Time::Now() + base::Seconds(collection_id_lifetime_in_seconds);

  VLOG(1) << base::TimeFormatShortDateAndTime(base::Time::Now())
          << " Reset collection ID to " << collection_id_;

  SavePrefs();
}

}  // namespace brave_federated
