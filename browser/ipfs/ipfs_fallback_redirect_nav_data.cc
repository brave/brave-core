/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_fallback_redirect_nav_data.h"

#include <memory>
#include <utility>

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
    const bool remove_this_entry_at_the_end)
    : original_url_(url),
      block_auto_redirect_(block_auto_redirect),
      remove_this_entry_at_the_end_(remove_this_entry_at_the_end) {}

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
IpfsFallbackRedirectNavigationData* IpfsFallbackRedirectNavigationData::Create(
    content::NavigationEntry* entry,
    std::unique_ptr<base::SupportsUserData::Data> data) {
  DCHECK(entry);
  entry->SetUserData(kIpfsFallbackRedirectNavigationDataKey, std::move(data));
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
IpfsFallbackRedirectNavigationData::FindFallbackData(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  auto& controller = web_contents->GetController();
  for (int i = 0; i < controller.GetEntryCount(); i++) {
    auto* entry = controller.GetEntryAtIndex(i);
    if (!entry) {
      continue;
    }
    auto* nav_data = GetFallbackData(entry);
    if (nav_data) {
      return nav_data;
    }
  }
  return nullptr;
}

// static
void IpfsFallbackRedirectNavigationData::CleanAll(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  int index_to_remove{-1};
  auto& controller = web_contents->GetController();
  for (int i = 0; i < controller.GetEntryCount(); i++) {
    auto* entry = controller.GetEntryAtIndex(i);
    if (!entry) {
      continue;
    }
    auto* nav_data = GetFallbackData(entry);
    if (!nav_data) {
      continue;
    }

    if (nav_data->GetRemoveFlag()) {
      index_to_remove = i;
    }

    entry->RemoveUserData(kIpfsFallbackRedirectNavigationDataKey);
  }

  if (index_to_remove >= 0) {
    controller.RemoveEntryAtIndex(index_to_remove);
  }
}

GURL IpfsFallbackRedirectNavigationData::GetOriginalUrl() const {
  return original_url_;
}

bool IpfsFallbackRedirectNavigationData::IsAutoRedirectBlocked() const {
  return block_auto_redirect_;
}

bool IpfsFallbackRedirectNavigationData::GetRemoveFlag() const {
  return remove_this_entry_at_the_end_;
}

void IpfsFallbackRedirectNavigationData::SetOriginalUrl(const GURL& url) {
  original_url_ = url;
}

void IpfsFallbackRedirectNavigationData::SetAutoRedirectBlock(
    const bool new_val) {
  block_auto_redirect_ = new_val;
}

void IpfsFallbackRedirectNavigationData::SetRemoveFlag(const bool new_val) {
  remove_this_entry_at_the_end_ = new_val;
}

std::unique_ptr<base::SupportsUserData::Data>
IpfsFallbackRedirectNavigationData::Clone() {
  auto copy = std::make_unique<IpfsFallbackRedirectNavigationData>();
  copy->original_url_ = original_url_;
  copy->block_auto_redirect_ = block_auto_redirect_;
  copy->remove_this_entry_at_the_end_ = remove_this_entry_at_the_end_;
  return std::move(copy);
}

std::string IpfsFallbackRedirectNavigationData::ToDebugString() const {
  return base::StringPrintf(
      "remove_this_entry_at_the_end_:%d block_auto_redirect:%d original_url:%s",
      remove_this_entry_at_the_end_, block_auto_redirect_,
      original_url_.spec().c_str());
}

}  // namespace ipfs
