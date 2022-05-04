/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_permission_request.h"

#include "brave/browser/widevine/widevine_utils.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "components/permissions/request_type.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"

// static
bool WidevinePermissionRequest::is_test_ = false;

WidevinePermissionRequest::WidevinePermissionRequest(
    content::WebContents* web_contents,
    bool for_restart)
    : PermissionRequest(
          web_contents->GetVisibleURL(),
          permissions::RequestType::kWidevine,
          /*has_gesture=*/false,
          base::BindOnce(&WidevinePermissionRequest::PermissionDecided,
                         base::Unretained(this)),
          base::BindOnce(&WidevinePermissionRequest::DeleteRequest,
                         base::Unretained(this))),
      web_contents_(web_contents),
      for_restart_(for_restart) {}

WidevinePermissionRequest::~WidevinePermissionRequest() = default;

std::u16string WidevinePermissionRequest::GetMessageTextFragment() const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      GetWidevinePermissionRequestTextFrangmentResourceId(for_restart_));
}

void WidevinePermissionRequest::PermissionDecided(ContentSetting result,
                                                  bool is_one_time) {
  // Permission granted
  if (result == ContentSetting::CONTENT_SETTING_ALLOW) {
#if BUILDFLAG(IS_LINUX)
    // Prevent relaunch during the browser test.
    // This will cause abnormal termination during the test.
    if (for_restart_ && !is_test_) {
      // Try relaunch after handling permission grant logics in this turn.
      base::SequencedTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::BindOnce(&chrome::AttemptRelaunch));
    }
#endif
    if (!for_restart_) {
      EnableWidevineCdmComponent();
    }
    // Permission denied
  } else if (result == ContentSetting::CONTENT_SETTING_BLOCK) {
    DontAskWidevineInstall(web_contents_, dont_ask_widevine_install_);
    // Cancelled
  } else {
    DCHECK(result == CONTENT_SETTING_DEFAULT);
    // Do nothing.
  }
}

void WidevinePermissionRequest::DeleteRequest() {
  delete this;
}

std::u16string WidevinePermissionRequest::GetExplanatoryMessageText() const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_WIDEVINE_INSTALL_MESSAGE);
}
