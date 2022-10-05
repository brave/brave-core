/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/request_otr/request_otr_tab_helper.h"

#include <utility>

#include "base/command_line.h"
#include "base/containers/flat_set.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/infobars/request_otr_infobar_delegate.h"
#include "brave/components/request_otr/browser/request_otr_tab_storage.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

using request_otr::RequestOTRTabStorage;

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

  RequestOTRTabStorage* storage =
      RequestOTRTabStorage::GetOrCreate(web_contents());
  DCHECK(storage);
  if (!storage) {
    return;
  }
  if (!storage->RequestedOTR()) {
    return;
  }

  RequestOTRInfoBarDelegate::Create(
      infobars::ContentInfoBarManager::FromWebContents(web_contents()));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(RequestOTRTabHelper);
