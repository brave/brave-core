/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_permission_request.h"

#include "brave/browser/widevine/widevine_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#endif

WidevinePermissionRequest::WidevinePermissionRequest(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
}

WidevinePermissionRequest::~WidevinePermissionRequest() {
}

permissions::PermissionRequest::IconId WidevinePermissionRequest::GetIconId()
    const {
  return vector_icons::kExtensionIcon;
}

base::string16 WidevinePermissionRequest::GetMessageTextFragment() const {
  return l10n_util::GetStringUTF16(
      GetWidevinePermissionRequestTextFrangmentResourceId());
}

GURL WidevinePermissionRequest::GetOrigin() const {
  return web_contents_->GetVisibleURL();
}

void WidevinePermissionRequest::PermissionGranted() {
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
  EnableWidevineCdmComponent(web_contents_);
#endif

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  // Run next commands at the next loop turn to prevent this is destroyed
  // by restarting process. This should be destroyed by RequestFinished().
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&InstallBundleOrRestartBrowser));
#endif
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

permissions::PermissionRequestType
WidevinePermissionRequest::GetPermissionRequestType() const {
  return permissions::PermissionRequestType::PERMISSION_WIDEVINE;
}

base::string16 WidevinePermissionRequest::GetExplanatoryMessageText() const {
  return l10n_util::GetStringUTF16(IDS_WIDEVINE_INSTALL_MESSAGE);
}
