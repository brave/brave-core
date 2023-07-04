/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/request_otr/request_otr_tab_helper.h"

#include <utility>

#include "base/command_line.h"
#include "base/containers/flat_set.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/infobars/request_otr_infobar_delegate.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#endif

using request_otr::RequestOTRStorageTabHelper;

RequestOTRTabHelper::RequestOTRTabHelper(content::WebContents* contents)
    : WebContentsObserver(contents),
      content::WebContentsUserData<RequestOTRTabHelper>(*contents),
      weak_factory_(this) {
  pref_service_ = user_prefs::UserPrefs::Get(contents->GetBrowserContext());
}

RequestOTRTabHelper::~RequestOTRTabHelper() = default;

void RequestOTRTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  RequestOTRStorageTabHelper* storage =
      RequestOTRStorageTabHelper::GetOrCreate(web_contents());
  // GetOrCreate should only return null if the runtime flag is disabled, and
  // this tab helper should never even be created if the runtime flag is
  // disabled, so a null here is bad news.
  DCHECK(storage);
  if (!storage) {
    return;
  }
  if (!storage->has_requested_otr()) {
    return;
  }

#if defined(TOOLKIT_VIEWS)
  RequestOTRInfoBarDelegate::Create(
      infobars::ContentInfoBarManager::FromWebContents(web_contents()));
#endif
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(RequestOTRTabHelper);
