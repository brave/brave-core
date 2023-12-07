/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "brave/components/request_otr/browser/request_otr_p3a.h"
#include "brave/components/request_otr/common/features.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

namespace request_otr {

RequestOTRStorageTabHelper::RequestOTRStorageTabHelper(
    content::WebContents* contents)
    : content::WebContentsUserData<RequestOTRStorageTabHelper>(*contents) {}

RequestOTRStorageTabHelper::~RequestOTRStorageTabHelper() = default;

// static
RequestOTRStorageTabHelper* RequestOTRStorageTabHelper::GetOrCreate(
    content::WebContents* web_contents) {
  if (!base::FeatureList::IsEnabled(
          request_otr::features::kBraveRequestOTRTab)) {
    return nullptr;
  }
  RequestOTRStorageTabHelper* storage = FromWebContents(web_contents);
  if (!storage) {
    CreateForWebContents(web_contents);
    storage = FromWebContents(web_contents);
  }
  return storage;
}

void RequestOTRStorageTabHelper::set_requested_otr(bool otr) {
  if (!otr_ && otr) {
    // Session is starting, record P3A
    RecordSessionStats();
  }
  otr_ = otr;
}

void RequestOTRStorageTabHelper::RecordSessionStats() {
  PrefService* profile_prefs =
      user_prefs::UserPrefs::Get(GetWebContents().GetBrowserContext());
  p3a::RecordSessionCount(profile_prefs, true);
}

void RequestOTRStorageTabHelper::MaybeEnable1PESForUrl(
    ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
    const GURL& url,
    base::OnceCallback<void()> on_ready) {
  DCHECK(ephemeral_storage_service);
  blocked_domain_1pes_lifetime_ =
      BlockedDomain1PESLifetime::GetOrCreate(ephemeral_storage_service, url);
  blocked_domain_1pes_lifetime_->AddOnReadyCallback(std::move(on_ready));
  DVLOG(1) << "RequestOTRStorageTabHelper::MaybeEnable1PESForUrl successful!";
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(RequestOTRStorageTabHelper);

}  // namespace request_otr
