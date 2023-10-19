/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_fallback_redirect_nav_data.h"

#include <memory>
#include <utility>

#include "brave/components/ipfs/ipfs_utils.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"

namespace {
const char kIpfsFallbackRedirectNavigationDataKey[] =
    "pfs-fallback-redirect-nav-data";

}

namespace ipfs {

IpfsFallbackRedirectNavigationData::IpfsFallbackRedirectNavigationData(
    const GURL& url)
    : original_url_(url) {}
IpfsFallbackRedirectNavigationData::IpfsFallbackRedirectNavigationData(
    const GURL& url,
    const bool block_auto_redirect,
    const bool valid)
    : original_url_(url),
      block_auto_redirect_(block_auto_redirect),
      valid_(valid) {}

IpfsFallbackRedirectNavigationData::~IpfsFallbackRedirectNavigationData() =
    default;

// static
IpfsFallbackRedirectNavigationData*
IpfsFallbackRedirectNavigationData::GetOrCreate(
    content::NavigationEntry* entry) {
  DCHECK(entry);
  if (!GetFallbackData(entry)) {
    entry->SetUserData(kIpfsFallbackRedirectNavigationDataKey,
                       std::make_unique<IpfsFallbackRedirectNavigationData>());
  }
  return GetFallbackData(entry);
}

// static
IpfsFallbackRedirectNavigationData*
IpfsFallbackRedirectNavigationData::GetFallbackData(
    content::NavigationEntry* entry) {
  DCHECK(entry);
  auto* data = static_cast<IpfsFallbackRedirectNavigationData*>(
      entry->GetUserData(kIpfsFallbackRedirectNavigationDataKey));
  return data;
}

// static
IpfsFallbackRedirectNavigationData*
IpfsFallbackRedirectNavigationData::GetFallbackDataFromRedirectChain(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  IpfsFallbackRedirectNavigationData* ipfs_fallback_nav_data = nullptr;
  auto& controller = web_contents->GetController();
  for (int i = 0; i < controller.GetEntryCount(); i++) {
    auto* entry = controller.GetEntryAtIndex(i);
    if (!entry) {
      continue;
    }
    if (!ipfs_fallback_nav_data) {
      auto* nav_data = GetFallbackData(entry);
      ipfs_fallback_nav_data =
          (nav_data && nav_data->IsValid()) ? nav_data : nullptr;
    } else {
      break;
    }
  }
  return ipfs_fallback_nav_data;
}

// static
bool IpfsFallbackRedirectNavigationData::IsAutoRedirectBlocked(
    content::WebContents* web_contents,
    const GURL& current_page_url,
    const bool remove_from_history) {
  DCHECK(web_contents);
  bool is_blocked{false};
  auto& controller = web_contents->GetController();
  for (int i = 0; i < controller.GetEntryCount(); i++) {
    auto* entry = controller.GetEntryAtIndex(i);
    if (!entry) {
      continue;
    }
    const auto* nav_data = GetFallbackData(entry);
    is_blocked = nav_data && nav_data->IsValid() &&
                 nav_data->IsAutoRedirectBlocked() &&
                 !nav_data->GetOriginalUrl().is_empty() &&
                 nav_data->GetOriginalUrl() == current_page_url;
    if (is_blocked) {
      if (remove_from_history) {
        controller.RemoveEntryAtIndex(i);
      }
      break;
    }
  }
  return is_blocked;
}

// static
bool IpfsFallbackRedirectNavigationData::IsSameIpfsLink(
    content::WebContents* web_contents,
    const GURL& current_page_url) {
  DCHECK(web_contents);
  auto& controller = web_contents->GetController();
  if (controller.GetEntryCount() <= 0) {
    return false;
  }

  auto current_page_url_converted =
      ipfs::IsIPFSScheme(current_page_url)
          ? current_page_url
          : ipfs::ExtractSourceFromGateway(current_page_url);
  for (int i = controller.GetEntryCount() - 1; i >= 0; i--) {
    auto* entry = controller.GetEntryAtIndex(i);
    if (!entry) {
      continue;
    }
    auto entry_url_converted = ipfs::ExtractSourceFromGateway(entry->GetURL());
    return entry_url_converted.has_value() &&
           entry_url_converted.value() == current_page_url_converted;
  }
  return false;
}

// static
void IpfsFallbackRedirectNavigationData::CleanAll(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  auto& controller = web_contents->GetController();
  for (int i = 0; i < controller.GetEntryCount(); i++) {
    auto* entry = controller.GetEntryAtIndex(i);
    if (!entry) {
      continue;
    }
    entry->RemoveUserData(kIpfsFallbackRedirectNavigationDataKey);
  }
}

GURL IpfsFallbackRedirectNavigationData::GetOriginalUrl() const {
  return original_url_;
}

bool IpfsFallbackRedirectNavigationData::IsAutoRedirectBlocked() const {
  return block_auto_redirect_;
}

bool IpfsFallbackRedirectNavigationData::IsValid() const {
  return valid_;
}

void IpfsFallbackRedirectNavigationData::SetOriginalUrl(const GURL& url) {
  original_url_ = url;
}

void IpfsFallbackRedirectNavigationData::SetAutoRedirectBlock(
    const bool new_val) {
  block_auto_redirect_ = new_val;
}

void IpfsFallbackRedirectNavigationData::SetValid(const bool new_val) {
  valid_ = new_val;
}

}  // namespace ipfs
