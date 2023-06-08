/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"

#include <map>
#include <memory>
#include <set>

#include "base/feature_list.h"
#include "base/hash/md5.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"
#include "net/base/url_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using content::BrowserContext;
using content::NavigationHandle;
using content::SessionStorageNamespace;
using content::WebContents;

namespace ephemeral_storage {

namespace {

// TODO(bridiver) - share these constants with DOMWindowStorage
constexpr char kSessionStorageSuffix[] = "/ephemeral-session-storage";

// Session storage ids are expected to be 36 character long GUID strings. Since
// we are constructing our own ids, we convert our string into a 32 character
// hash and then use that make up our own GUID-like string. Because of the way
// we are constructing the string we should never collide with a real GUID and
// we only need to worry about hash collisions, which are unlikely.
std::string StringToSessionStorageId(const std::string& string,
                                     const std::string& suffix) {
  std::string hash = base::MD5String(string + suffix) + "____";
  DCHECK_EQ(hash.size(), 36u);
  return hash;
}

}  // namespace

// EphemeralStorageTabHelper helps to manage the lifetime of ephemeral storage.
// For more information about the design of ephemeral storage please see the
// design document at:
// https://github.com/brave/brave-browser/wiki/Ephemeral-Storage-Design
EphemeralStorageTabHelper::EphemeralStorageTabHelper(WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<EphemeralStorageTabHelper>(*web_contents) {
  DCHECK(base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage));

  // The URL might not be empty if this is a restored WebContents, for instance.
  // In that case we want to make sure it has valid ephemeral storage.
  const GURL& url = web_contents->GetLastCommittedURL();
  const std::string ephemeral_storage_domain =
      net::URLToEphemeralStorageDomain(url);
  CreateEphemeralStorageAreasForDomainAndURL(ephemeral_storage_domain, url);
}

EphemeralStorageTabHelper::~EphemeralStorageTabHelper() = default;

void EphemeralStorageTabHelper::WebContentsDestroyed() {}

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
}

void EphemeralStorageTabHelper::CreateEphemeralStorageAreasForDomainAndURL(
    const std::string& new_domain,
    const GURL& new_url) {
  if (new_url.is_empty()) {
    return;
  }

  auto* browser_context = web_contents()->GetBrowserContext();
  auto* site_instance = web_contents()->GetSiteInstance();
  auto* storage_partition = browser_context->GetStoragePartition(site_instance);

  // Session storage is always per-tab and never per-TLD, so we always delete
  // and recreate the session storage when switching domains.
  //
  // We need to explicitly release the storage namespace before recreating a
  // new one in order to make sure that we remove the final reference and free
  // it.
  session_storage_namespace_.reset();

  std::string session_partition_id = StringToSessionStorageId(
      content::GetSessionStorageNamespaceId(web_contents()),
      kSessionStorageSuffix);

  auto* opener_rfh = web_contents()->GetOpener();
  session_storage_namespace_ = content::CreateSessionStorageNamespace(
      storage_partition, session_partition_id,
      // clone the namespace if there is an opener
      // https://html.spec.whatwg.org/multipage/browsers.html#copy-session-storage
      opener_rfh ? absl::make_optional<std::string>(StringToSessionStorageId(
                       content::GetSessionStorageNamespaceId(
                           WebContents::FromRenderFrameHost(opener_rfh)),
                       kSessionStorageSuffix))
                 : absl::nullopt);

  tld_ephemeral_lifetime_ = TLDEphemeralLifetime::GetOrCreate(
      browser_context, new_domain, site_instance->GetStoragePartitionConfig());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(EphemeralStorageTabHelper);

}  // namespace ephemeral_storage
