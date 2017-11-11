/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_resource_dispatcher_host_delegate.h"

#include "brave/components/brave_shields/browser/brave_shields_resource_throttle.h"

using content::ResourceType;

BraveResourceDispatcherHostDelegate::BraveResourceDispatcherHostDelegate() {
}

BraveResourceDispatcherHostDelegate::~BraveResourceDispatcherHostDelegate() {
}

void BraveResourceDispatcherHostDelegate::AppendStandardResourceThrottles(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    ResourceType resource_type,
    std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) {
  ChromeResourceDispatcherHostDelegate::AppendStandardResourceThrottles(
    request, resource_context, resource_type, throttles);

  content::ResourceThrottle* throttle = MaybeCreateBraveShieldsResourceThrottle(
    request, resource_type);
  throttles->push_back(base::WrapUnique(throttle));
}
