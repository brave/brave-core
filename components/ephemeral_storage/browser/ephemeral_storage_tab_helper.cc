/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/browser/ephemeral_storage_tab_helper.h"

#include <string>

#include "base/no_destructor.h"
#include "base/feature_list.h"
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
// TODO(bridiver) - this is a layering violation
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"

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

}
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

  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
          navigation_handle->GetURL(),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  std::string partition_id = domain + "/ephemeral-storage";

  content::SessionStorageNamespaceMap::const_iterator it =
      session_storage_namespace_map().find(partition_id);

  // we only need to check one map since they are added/removed together
  if (it == session_storage_namespace_map().end()) {
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

  ClearEphemeralStorage();
}

void EphemeralStorageTabHelper::WebContentsDestroyed() {
  ClearEphemeralStorage();
}

void EphemeralStorageTabHelper::ClearEphemeralStorage() {
  if (!base::FeatureList::IsEnabled(blink::kBraveEphemeralStorage)) {
    return;
  }

  std::string storage_domain =
      net::registry_controlled_domains::GetDomainAndRegistry(
          web_contents()->GetLastCommittedURL(),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  for (Browser* browser : *BrowserList::GetInstance()) {
    if (browser->profile() == web_contents()->GetBrowserContext()) {
      TabStripModel* tab_strip = browser->tab_strip_model();
      for (int i = 0; i < tab_strip->count(); ++i) {
        WebContents* contents = tab_strip->GetWebContentsAt(i);
        if (contents != web_contents()) {
          std::string domain =
              net::registry_controlled_domains::GetDomainAndRegistry(
                  contents->GetLastCommittedURL(),
                  net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
          if (domain == storage_domain) {
            // There is an open tab with the same etld so don't delete the
            // namespace
            return;
          }
        }
      }
    }
  }

  local_storage_namespace_map().erase(storage_domain + "/ephemeral-storage");
  session_storage_namespace_map().erase(storage_domain + "/ephemeral-storage");
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(EphemeralStorageTabHelper)

}  // namespace ephemeral_storage
