/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "brave/components/permissions/contexts/brave_puppeteer_permission_context.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"
#include "net/base/url_util.h"

using content::BrowserContext;
using content::NavigationHandle;
using content::WebContents;

namespace ephemeral_storage {

// EphemeralStorageTabHelper helps to manage the lifetime of ephemeral storage.
// For more information about the design of ephemeral storage please see the
// design document at:
// https://github.com/brave/brave-browser/wiki/Ephemeral-Storage-Design
EphemeralStorageTabHelper::EphemeralStorageTabHelper(WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<EphemeralStorageTabHelper>(*web_contents),
      host_content_settings_map_(HostContentSettingsMapFactory::GetForProfile(
          web_contents->GetBrowserContext())),
      cookie_settings_(CookieSettingsFactory::GetForProfile(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()))) {
  DCHECK(base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage));

  // The URL might not be empty if this is a restored WebContents, for instance.
  // In that case we want to make sure it has valid ephemeral storage.
  const GURL& url = web_contents->GetLastCommittedURL();
  const std::string ephemeral_storage_domain =
      net::URLToEphemeralStorageDomain(url);
  CreateEphemeralStorageAreasForDomainAndURL(ephemeral_storage_domain, url);
  UpdateShieldsState(url);
}

EphemeralStorageTabHelper::~EphemeralStorageTabHelper() = default;

std::optional<base::UnguessableToken>
EphemeralStorageTabHelper::GetEphemeralStorageToken(const url::Origin& origin) {
  if (auto* ephemeral_storage_service =
          EphemeralStorageServiceFactory::GetForContext(
              web_contents()->GetBrowserContext())) {
    return ephemeral_storage_service->Get1PESToken(origin);
  }
  return std::nullopt;
}

std::optional<base::UnguessableToken>
EphemeralStorageTabHelper::GetEphemeralStorageToken(
    content::RenderFrameHost* render_frame_host,
    const url::Origin& origin) {
  // Just use the regular method for now - puppeteer isolation handled elsewhere
  LOG(INFO) << "[PUPPETEER_DEBUG] GetEphemeralStorageToken frame depth: " << render_frame_host->GetFrameDepth();
  
  // https://chromium.googlesource.com/chromium/src/+/master/content/public/browser/render_frame_host.h
  // if frame depth > 0, then it's not main frame
  // if puppeteer is enabled globally and it's not main frame, check if it has puppeteer permissions
  if (/* is puppeteer enabled */true && render_frame_host->GetFrameDepth() > 0) {
    // Check if this iframe has puppeteer permissions
    url::Origin main_frame_origin = render_frame_host->GetMainFrame()->GetLastCommittedOrigin();

    if (HasPuppeteerPermission(main_frame_origin)) {
      // If it has permissions, return a unique token for this iframe
      auto it = puppeteer_frames_.find(render_frame_host->GetGlobalId());
      if (it != puppeteer_frames_.end()) {
        return GetPuppeteerStorageToken(render_frame_host);
      } else {
        // Not found in map, add it
        EnablePuppeteerModeForFrame(render_frame_host);
        return GetPuppeteerStorageToken(render_frame_host);
      }
    }
  }

  return GetEphemeralStorageToken(origin);
}

void EphemeralStorageTabHelper::WebContentsDestroyed() {}

void EphemeralStorageTabHelper::DidStartNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  CreateProvisionalTLDEphemeralLifetime(navigation_handle);
}

void EphemeralStorageTabHelper::DidRedirectNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  CreateProvisionalTLDEphemeralLifetime(navigation_handle);
}

void EphemeralStorageTabHelper::DidFinishNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  // Clear all provisional ephemeral lifetimes. A committed ephemeral lifetime
  // is created in ReadyToCommitNavigation().
  provisional_tld_ephemeral_lifetimes_.clear();
}

void EphemeralStorageTabHelper::ReadyToCommitNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame()) {
    return;
  }
  if (navigation_handle->IsSameDocument()) {
    return;
  }

  const GURL& new_url = navigation_handle->GetURL();
  const GURL& last_committed_url = web_contents()->GetLastCommittedURL();

  std::string new_domain = net::URLToEphemeralStorageDomain(new_url);
  std::string previous_domain =
      net::URLToEphemeralStorageDomain(last_committed_url);
  if (new_domain != previous_domain) {
    // Create new storage areas for new ephemeral storage domain.
    CreateEphemeralStorageAreasForDomainAndURL(new_domain, new_url);
  }
  UpdateShieldsState(new_url);
}

void EphemeralStorageTabHelper::CreateEphemeralStorageAreasForDomainAndURL(
    const std::string& new_domain,
    const GURL& new_url) {
  if (new_url.is_empty()) {
    return;
  }

  auto* browser_context = web_contents()->GetBrowserContext();
  auto* site_instance = web_contents()->GetSiteInstance();

  tld_ephemeral_lifetime_ = TLDEphemeralLifetime::GetOrCreate(
      {browser_context, new_domain,
       site_instance->GetStoragePartitionConfig()});
}

void EphemeralStorageTabHelper::CreateProvisionalTLDEphemeralLifetime(
    NavigationHandle* navigation_handle) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveProvisionalTLDEphemeralLifetime)) {
    return;
  }

  const GURL& url = navigation_handle->GetURL();
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  const std::string new_domain = net::URLToEphemeralStorageDomain(url);
  if (new_domain.empty()) {
    return;
  }

  auto* browser_context = web_contents()->GetBrowserContext();
  auto* site_instance = web_contents()->GetSiteInstance();

  provisional_tld_ephemeral_lifetimes_.emplace(
      TLDEphemeralLifetime::GetOrCreate(
          {browser_context, new_domain,
           site_instance->GetStoragePartitionConfig()}));
}

void EphemeralStorageTabHelper::UpdateShieldsState(const GURL& url) {
  if (!host_content_settings_map_ || !tld_ephemeral_lifetime_) {
    return;
  }
  const bool shields_enabled =
      brave_shields::GetBraveShieldsEnabled(host_content_settings_map_, url);
  const bool cookies_restricted =
      brave_shields::GetCookieControlType(host_content_settings_map_,
                                          cookie_settings_.get(), url) !=
      brave_shields::ControlType::ALLOW;
  tld_ephemeral_lifetime_->SetShieldsStateOnHost(
      url.host(), shields_enabled && cookies_restricted);
}

// NEW: Puppeteer storage support methods
std::optional<base::UnguessableToken>
EphemeralStorageTabHelper::GetPuppeteerStorageToken(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host) {
    return std::nullopt;
  }
  return render_frame_host->GetDevToolsFrameToken();
}

void EphemeralStorageTabHelper::EnablePuppeteerModeForFrame(
    content::RenderFrameHost* frame_host) {
  if (!frame_host) {
    return;
  }

  url::Origin origin = frame_host->GetLastCommittedOrigin();
  if (HasPuppeteerPermission(origin)) {
    puppeteer_frames_[frame_host->GetGlobalId()] = origin;
    CreatePuppeteerStorageForFrame(frame_host);
  }
}

void EphemeralStorageTabHelper::DisablePuppeteerModeForFrame(
    content::RenderFrameHost* frame_host) {
  if (!frame_host) {
    return;
  }

  auto it = puppeteer_frames_.find(frame_host->GetGlobalId());
  if (it != puppeteer_frames_.end()) {
    puppeteer_frames_.erase(it);
  }
}

void EphemeralStorageTabHelper::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host || !render_frame_host->GetParent()) {
    return; // Only handle iframes
  }

  url::Origin iframe_origin = render_frame_host->GetLastCommittedOrigin();
  if (HasPuppeteerPermission(iframe_origin)) {
    EnablePuppeteerModeForFrame(render_frame_host);
  }
}

void EphemeralStorageTabHelper::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  DisablePuppeteerModeForFrame(render_frame_host);
}

bool EphemeralStorageTabHelper::HasPuppeteerPermission(
    const url::Origin& origin) const {
  // Use the centralized permission checking method
  return BravePuppeteerPermissionContext::IsOriginAllowedForPuppeteerMode(
      web_contents()->GetBrowserContext(), origin);
}

void EphemeralStorageTabHelper::CreatePuppeteerStorageForFrame(
    content::RenderFrameHost* frame_host) {
  if (!frame_host) {
    return;
  }

  // Get main frame origin
  content::RenderFrameHost* main_frame = web_contents()->GetPrimaryMainFrame();
  if (!main_frame) {
    return;
  }
  url::Origin main_frame_origin = main_frame->GetLastCommittedOrigin();

  auto token = GetPuppeteerStorageToken(frame_host);
  if (token) {
    DVLOG(1) << "Created puppeteer storage for frame: "
             << frame_host->GetLastCommittedOrigin();
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(EphemeralStorageTabHelper);

}  // namespace ephemeral_storage
