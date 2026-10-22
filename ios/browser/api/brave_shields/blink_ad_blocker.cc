// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/brave_shields/blink_ad_blocker.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "ios/web/content/init/ios_content_browser_client.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"

namespace brave_shields {

namespace {

using network::mojom::RequestDestination;

std::string RequestTypeFor(RequestDestination destination) {
  switch (destination) {
    case RequestDestination::kDocument:
      return "main_frame";
    case RequestDestination::kFrame:
    case RequestDestination::kIframe:
    case RequestDestination::kFencedframe:
      return "sub_frame";
    case RequestDestination::kStyle:
      return "stylesheet";
    case RequestDestination::kScript:
    case RequestDestination::kWorker:
    case RequestDestination::kSharedWorker:
    case RequestDestination::kServiceWorker:
    case RequestDestination::kAudioWorklet:
    case RequestDestination::kPaintWorklet:
      return "script";
    case RequestDestination::kImage:
      return "image";
    case RequestDestination::kFont:
      return "font";
    case RequestDestination::kAudio:
    case RequestDestination::kVideo:
    case RequestDestination::kTrack:
      return "media";
    case RequestDestination::kObject:
    case RequestDestination::kEmbed:
      return "object";
    // fetch() and XHR have no destination.
    case RequestDestination::kEmpty:
      return "xhr";
    default:
      return "other";
  }
}

std::optional<rust::Box<adblock::Engine>> BuildEngine(
    base::OnceCallback<void(rust::Box<adblock::FilterSet>*)> load) {
  auto filter_set = adblock::new_filter_set(/*debug=*/false);
  std::move(load).Run(&filter_set);
  auto result = adblock::engine_from_filter_set(std::move(filter_set));
  if (result.result_kind != adblock::ResultKind::Success) {
    LOG(ERROR) << "Failed to build adblock engine: "
               << std::string(result.error_message);
    return std::nullopt;
  }
  return std::move(result.value);
}

}  // namespace

BlinkAdBlocker::BlinkAdBlocker(AdBlockFiltersProviderManager* manager)
    : manager_(manager) {
  adblock::set_domain_resolver();
  manager_->AddObserver(this);
  // There is no DAT cache to load from here, so the startup change
  // notification must not be suppressed.
  manager_->ForceNotifyObserver(*this, /*is_for_default_engine=*/true);
  // Unretained is safe: the destructor clears the blocker.
  web::SetSubresourceBlocker(base::BindRepeating(&BlinkAdBlocker::ShouldBlock,
                                                 base::Unretained(this)));
}

BlinkAdBlocker::~BlinkAdBlocker() {
  web::SetSubresourceBlocker({});
  manager_->RemoveObserver(this);
}

void BlinkAdBlocker::SetOnBlocked(
    base::RepeatingCallback<void(const GURL&)> on_blocked) {
  on_blocked_ = std::move(on_blocked);
}

void BlinkAdBlocker::OnChanged(bool is_default_engine) {
  if (!is_default_engine) {
    return;
  }
  manager_->LoadFilterSetForEngine(
      /*is_for_default_engine=*/true,
      base::BindOnce(&BlinkAdBlocker::OnFilterSetLoaded,
                     weak_factory_.GetWeakPtr()));
}

void BlinkAdBlocker::OnFilterSetLoaded(
    base::OnceCallback<void(rust::Box<adblock::FilterSet>*)> load) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&BuildEngine, std::move(load)),
      base::BindOnce(&BlinkAdBlocker::OnEngineBuilt,
                     weak_factory_.GetWeakPtr()));
}

void BlinkAdBlocker::OnEngineBuilt(
    std::optional<rust::Box<adblock::Engine>> engine) {
  if (engine) {
    engine_ = std::move(engine);
  }
}

bool BlinkAdBlocker::ShouldBlock(const network::ResourceRequest& request) {
  if (!engine_ || !request.request_initiator ||
      !request.url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }
  const url::Origin& initiator = *request.request_initiator;
  const bool is_third_party =
      !net::registry_controlled_domains::SameDomainOrHost(
          request.url, initiator,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  auto result = (*engine_)->matches(
      request.url.spec(), std::string(request.url.host()), initiator.host(),
      RequestTypeFor(request.destination), is_third_party, request.method,
      /*previously_matched_rule=*/false, /*force_check_exceptions=*/true);
  const bool blocked = result.important || (result.matched && !result.has_exception);
  if (blocked && on_blocked_) {
    on_blocked_.Run(request.url);
  }
  return blocked;
}

}  // namespace brave_shields
