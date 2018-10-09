/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_resource_dispatcher_host_delegate.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"

using content::ResourceType;

BraveResourceDispatcherHostDelegate::BraveResourceDispatcherHostDelegate() {
  g_brave_browser_process->ad_block_service()->Start();
  if (brave_shields::AdBlockRegionalService::IsSupportedLocale(
          g_brave_browser_process->GetApplicationLocale()))
    g_brave_browser_process->ad_block_regional_service()->Start();
  g_brave_browser_process->https_everywhere_service()->Start();
  g_brave_browser_process->tracking_protection_service()->Start();
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
}
