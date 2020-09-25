/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/browser/ephemeral_storage_tab_helper.h"

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/no_destructor.h"
// TODO(bridiver) - move ephemeral storage tab helper in brave/browser
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/features.h"

using content::BrowserContext;
using content::NavigationHandle;
using content::WebContents;

namespace {

content::SessionStorageNamespaceMap& session_storage_namespace_map() {
  static base::NoDestructor<content::SessionStorageNamespaceMap>
      session_storage_namespace_map;
  return *session_storage_namespace_map.get();
}

content::SessionStorageNamespaceMap& local_storage_namespace_map() {
  static base::NoDestructor<content::SessionStorageNamespaceMap>
      local_storage_namespace_map;
  return *local_storage_namespace_map.get();
}

std::string URLToStorageDomain(const GURL& url) {
  return net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

}  // namespace
namespace ephemeral_storage {

EphemeralStorageTabHelper::~EphemeralStorageTabHelper() {}

EphemeralStorageTabHelper::EphemeralStorageTabHelper(WebContents* web_contents)
    : WebContentsObserver(web_contents) {}

void EphemeralStorageTabHelper::ReadyToCommitNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame())
    return;
  if (navigation_handle->IsSameDocument())
    return;

  std::string domain = URLToStorageDomain(navigation_handle->GetURL());
  std::string partition_id = domain + "/ephemeral-storage";

  content::SessionStorageNamespaceMap::const_iterator it =
      session_storage_namespace_map().find(partition_id);

  if (it == session_storage_namespace_map().end()) {
    // Namespaces are added and removed together, so these two maps
    // should be in sync.
    DCHECK(local_storage_namespace_map().find(partition_id) ==
           local_storage_namespace_map().end());
    auto* browser_context = web_contents()->GetBrowserContext();

    auto instance = content::SiteInstance::CreateForURL(
        browser_context, navigation_handle->GetURL());

    auto* partition =
        BrowserContext::GetStoragePartition(browser_context, instance.get());

    auto session_storage_namespace = content::CreateSessionStorageNamespace(
        partition, domain + "/ephemeral-session-storage");
    session_storage_namespace_map()[partition_id] =
        std::move(session_storage_namespace);

    auto local_storage_namespace = content::CreateSessionStorageNamespace(
        partition, domain + "/ephemeral-local-storage");
    local_storage_namespace_map()[partition_id] =
        std::move(local_storage_namespace);
  }

  ClearEphemeralStorageIfNecessary();
}

void EphemeralStorageTabHelper::WebContentsDestroyed() {
  ClearEphemeralStorageIfNecessary();
}

bool EphemeralStorageTabHelper::IsAnotherTabOpenWithStorageDomain(
    const std::string& storage_domain) {
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (browser->profile() != web_contents()->GetBrowserContext())
      continue;

    TabStripModel* tab_strip = browser->tab_strip_model();
    for (int i = 0; i < tab_strip->count(); ++i) {
      WebContents* contents = tab_strip->GetWebContentsAt(i);
      const GURL& url = contents->GetLastCommittedURL();
      if (contents != web_contents() &&
          URLToStorageDomain(url) == storage_domain) {
        return true;
      }
    }
  }

  return false;
}

void EphemeralStorageTabHelper::ClearEphemeralStorageIfNecessary() {
  if (!base::FeatureList::IsEnabled(blink::features::kBraveEphemeralStorage)) {
    return;
  }

  std::string storage_domain =
      URLToStorageDomain(web_contents()->GetLastCommittedURL());
  if (IsAnotherTabOpenWithStorageDomain(storage_domain)) {
    return;
  }

  local_storage_namespace_map().erase(storage_domain + "/ephemeral-storage");
  session_storage_namespace_map().erase(storage_domain + "/ephemeral-storage");

  web_contents()
      ->GetBrowserContext()
      ->DeleteInMemoryStoragePartitionForMainFrameURL(
          web_contents()->GetLastCommittedURL());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(EphemeralStorageTabHelper)

}  // namespace ephemeral_storage
