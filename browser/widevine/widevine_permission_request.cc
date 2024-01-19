/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_permission_request.h"

#include "brave/browser/widevine/widevine_utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/permissions/permission_widevine_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "components/permissions/request_type.h"
#include "components/prefs/pref_service.h"
#include "components/url_formatter/elide_url.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

// static
bool WidevinePermissionRequest::is_test_ = false;

WidevinePermissionRequest::WidevinePermissionRequest(
    content::WebContents* web_contents,
    bool for_restart)
    : PermissionRequest(
          web_contents->GetVisibleURL(),
          permissions::RequestType::kWidevine,
          /*has_gesture=*/false,
          base::BindRepeating(&WidevinePermissionRequest::PermissionDecided,
                              base::Unretained(this)),
          base::BindOnce(&WidevinePermissionRequest::DeleteRequest,
                         base::Unretained(this))),
      web_contents_(web_contents),
      for_restart_(for_restart) {}

WidevinePermissionRequest::~WidevinePermissionRequest() = default;

#if BUILDFLAG(IS_ANDROID)
permissions::PermissionRequest::AnnotatedMessageText
WidevinePermissionRequest::GetDialogAnnotatedMessageText(
    const GURL& embedding_origin) const {
  return permissions::PermissionRequest::AnnotatedMessageText(
      l10n_util::GetStringFUTF16(
          GetWidevinePermissionRequestTextFrangmentResourceId(false),
          url_formatter::FormatUrlForSecurityDisplay(
              requesting_origin(),
              url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC)),
      {});
}
#else
std::u16string WidevinePermissionRequest::GetMessageTextFragment() const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      GetWidevinePermissionRequestTextFrangmentResourceId(for_restart_));
}
#endif

void WidevinePermissionRequest::PermissionDecided(ContentSetting result,
                                                  bool is_one_time,
                                                  bool is_final_decision) {
  // Permission granted
  if (result == ContentSetting::CONTENT_SETTING_ALLOW) {
    if (!for_restart_) {
      EnableWidevineCdm();
    } else {
#if BUILDFLAG(IS_ANDROID)
      EnableWidevineCdm();
#endif
      // Prevent relaunch during the browser test.
      // This will cause abnormal termination during the test.
      if (!is_test_) {
        // Try relaunch after handling permission grant logics in this turn.
        base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE, base::BindOnce(&chrome::AttemptRelaunch));
      }
    }
    // Permission denied
  } else if (result == ContentSetting::CONTENT_SETTING_BLOCK) {
    Profile* profile =
        static_cast<Profile*>(web_contents_->GetBrowserContext());
    profile->GetPrefs()->SetBoolean(kAskEnableWidvine, !get_dont_ask_again());
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
