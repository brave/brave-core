/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_resource_dispatcher_host_delegate.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "brave/components/brave_shields/browser/local_data_files_service.h"
#include "extensions/buildflags/buildflags.h"

using content::ResourceType;

BraveResourceDispatcherHostDelegate::BraveResourceDispatcherHostDelegate() {
  g_brave_browser_process->ad_block_service()->Start();
  g_brave_browser_process->ad_block_custom_filters_service()->Start();
  g_brave_browser_process->ad_block_regional_service_manager()->Start();
  g_brave_browser_process->https_everywhere_service()->Start();
  // Ensure that all services that observe the local data files service
  // are created before calling Start().
  g_brave_browser_process->autoplay_whitelist_service();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  g_brave_browser_process->extension_whitelist_service();
#endif
  g_brave_browser_process->referrer_whitelist_service();
  g_brave_browser_process->tracking_protection_service();
  // Now start the local data files service, which calls all observers.
  g_brave_browser_process->local_data_files_service()->Start();
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
