/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/puppeteer_storage_manager.h"

#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "brave/components/permissions/contexts/brave_puppeteer_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"

namespace ephemeral_storage {

PuppeteerStorageManager::PuppeteerStorageManager(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<PuppeteerStorageManager>(*web_contents) {
  LOG(INFO) << "[PUPPETEER_DEBUG] PuppeteerStorageManager created for WebContents";
  auto* browser_context = web_contents->GetBrowserContext();
  host_content_settings_map_ =
      HostContentSettingsMapFactory::GetForProfile(browser_context);
  ephemeral_storage_service_ =
      EphemeralStorageServiceFactory::GetForContext(browser_context);
  LOG(INFO) << "[PUPPETEER_DEBUG] PuppeteerStorageManager initialized, ephemeral_storage_service: "
            << (ephemeral_storage_service_ ? "available" : "null");
}

PuppeteerStorageManager::~PuppeteerStorageManager() = default;

void PuppeteerStorageManager::CheckAndApplyPuppeteerStorage(
    content::RenderFrameHost* frame_host) {
  if (!frame_host || !frame_host->GetParent()) {
    LOG(INFO) << "[PUPPETEER_DEBUG] Skipping non-iframe frame";
    return;  // Only apply to iframes
  }

  url::Origin iframe_origin = frame_host->GetLastCommittedOrigin();
  LOG(INFO) << "[PUPPETEER_DEBUG] Checking puppeteer storage for iframe: " << iframe_origin;

  if (!HasPuppeteerPermission(iframe_origin)) {
    LOG(INFO) << "[PUPPETEER_DEBUG] No puppeteer permission, skipping storage isolation";
    return;  // No puppeteer permission
  }

  LOG(INFO) << "[PUPPETEER_DEBUG] Has puppeteer permission, proceeding with storage isolation";

  // Get parent origin for storage key generation
  content::RenderFrameHost* main_frame = web_contents()->GetPrimaryMainFrame();
  if (!main_frame) {
    LOG(INFO) << "[PUPPETEER_DEBUG] No main frame found, aborting";
    return;
  }

  url::Origin parent_origin = main_frame->GetLastCommittedOrigin();
  LOG(INFO) << "[PUPPETEER_DEBUG] Parent origin: " << parent_origin;

  // Get or create puppeteer storage token using DevTools frame token
  LOG(INFO) << "[PUPPETEER_DEBUG] Getting puppeteer storage token using DevTools frame token";
  auto token = frame_host->GetDevToolsFrameToken();

  LOG(INFO) << "[PUPPETEER_DEBUG] Got storage token, configuring partition";
  ConfigureStoragePartition(frame_host, token);
  LOG(INFO) << "[PUPPETEER_DEBUG] Applied puppeteer storage isolation for: " << iframe_origin;
}

void PuppeteerStorageManager::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host) {
    return;
  }
  LOG(INFO) << "[PUPPETEER_DEBUG] RenderFrameCreated for: "
            << render_frame_host->GetLastCommittedOrigin()
            << ", is_iframe: " << (render_frame_host->GetParent() != nullptr);
  // DISABLED: Using render_frame_host_impl.cc system that always works for pompel.me
  // CheckAndApplyPuppeteerStorage(render_frame_host);
}

void PuppeteerStorageManager::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() || navigation_handle->IsErrorPage()) {
    return;
  }

  // DISABLED: Using render_frame_host_impl.cc system that always works for pompel.me
  // CheckAndApplyPuppeteerStorage(navigation_handle->GetRenderFrameHost());
}

bool PuppeteerStorageManager::HasPuppeteerPermission(const url::Origin& origin) const {
  // Use the centralized permission checking method
  bool allowed = BravePuppeteerPermissionContext::IsOriginAllowedForPuppeteerMode(
      web_contents()->GetBrowserContext(), origin);
  LOG(INFO) << "[PUPPETEER_DEBUG] HasPuppeteerPermission for " << origin
            << ": " << (allowed ? "ALLOWED" : "DENIED");
  return allowed;
}

void PuppeteerStorageManager::ConfigureStoragePartition(
    content::RenderFrameHost* frame_host,
    const base::UnguessableToken& storage_token) {
  if (!frame_host || !ephemeral_storage_service_) {
    return;
  }

  url::Origin iframe_origin = frame_host->GetLastCommittedOrigin();
  DVLOG(1) << "Configuring puppeteer storage partition for: " << iframe_origin
           << " with token: " << storage_token;

  // Enable puppeteer storage for this origin in the service
  ephemeral_storage_service_->EnablePuppeteerStorageForOrigin(iframe_origin);

  // Create a separate storage partition for this iframe
  content::RenderFrameHost* main_frame = web_contents()->GetPrimaryMainFrame();
  if (!main_frame) {
    return;
  }

  url::Origin parent_origin = main_frame->GetLastCommittedOrigin();
  ephemeral_storage_service_->CreatePuppeteerStoragePartition(
      iframe_origin, parent_origin);

  DVLOG(1) << "Successfully configured puppeteer storage isolation for: "
           << iframe_origin;
}

void PuppeteerStorageManager::GrantPuppeteerPermissionForTesting(const url::Origin& origin) {
  if (!host_content_settings_map_) {
    return;
  }

  GURL origin_url = origin.GetURL();
  DVLOG(1) << "Granting puppeteer permission for testing: " << origin;

  // Grant ALLOW permission for this origin
  host_content_settings_map_->SetContentSettingDefaultScope(
      origin_url, origin_url, ContentSettingsType::BRAVE_PUPPETEER,
      CONTENT_SETTING_ALLOW);

  DVLOG(1) << "Successfully granted puppeteer permission for: " << origin;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PuppeteerStorageManager);

}  // namespace ephemeral_storage