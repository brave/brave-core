/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_permission_request.h"

#include "brave/browser/widevine/widevine_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "components/permissions/request_type.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

// static
bool WidevinePermissionRequest::is_test_ = false;

WidevinePermissionRequest::WidevinePermissionRequest(
    content::WebContents* web_contents,
    bool for_restart)
    : web_contents_(web_contents), for_restart_(for_restart) {}

WidevinePermissionRequest::~WidevinePermissionRequest() = default;

std::u16string WidevinePermissionRequest::GetMessageTextFragment() const {
  return l10n_util::GetStringUTF16(
      GetWidevinePermissionRequestTextFrangmentResourceId(for_restart_));
}

GURL WidevinePermissionRequest::GetOrigin() const {
  return web_contents_->GetVisibleURL();
}

void WidevinePermissionRequest::PermissionGranted(bool is_one_time) {
#if defined(OS_LINUX)
  // Prevent relaunch during the browser test.
  // This will cause abnormal termination during the test.
  if (for_restart_ && !is_test_) {
    // Try relaunch after handling permission grant logics in this turn.
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&chrome::AttemptRelaunch));
  }
#endif
  if (!for_restart_)
    EnableWidevineCdmComponent();
}

void WidevinePermissionRequest::PermissionDenied() {
  DontAskWidevineInstall(web_contents_, dont_ask_widevine_install_);
}

void WidevinePermissionRequest::Cancelled() {
  // Do nothing.
}

void WidevinePermissionRequest::RequestFinished() {
  delete this;
}

permissions::RequestType WidevinePermissionRequest::GetRequestType() const {
  return permissions::RequestType::kWidevine;
}

std::u16string WidevinePermissionRequest::GetExplanatoryMessageText() const {
  return l10n_util::GetStringUTF16(IDS_WIDEVINE_INSTALL_MESSAGE);
}
