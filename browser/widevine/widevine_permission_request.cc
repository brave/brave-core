/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_permission_request.h"

#include "brave/browser/widevine/widevine_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "third_party/widevine/cdm/buildflags.h"

WidevinePermissionRequest::WidevinePermissionRequest(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
}

WidevinePermissionRequest::~WidevinePermissionRequest() {
}

PermissionRequest::IconId WidevinePermissionRequest::GetIconId() const {
  return kExtensionIcon;
}

base::string16 WidevinePermissionRequest::GetMessageTextFragment() const {
  return l10n_util::GetStringUTF16(
      GetWidevinePermissionRequestTextFrangmentResourceId());
}

GURL WidevinePermissionRequest::GetOrigin() const {
  return web_contents_->GetVisibleURL();
}

void WidevinePermissionRequest::PermissionGranted() {
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT) && !defined(OS_LINUX)
  EnableWidevineCdmComponent(web_contents_);
#endif

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  InstallBundleOrRestartBrowser();
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

PermissionRequestType
WidevinePermissionRequest::GetPermissionRequestType() const {
  return PermissionRequestType::PERMISSION_WIDEVINE;
}

base::string16 WidevinePermissionRequest::GetExplanatoryMessageText() const {
  return l10n_util::GetStringUTF16(IDS_WIDEVINE_INSTALL_MESSAGE);
}
