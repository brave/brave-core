/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_permission_request.h"

#include <memory>

#include "base/check.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/permissions/permission_widevine_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "components/permissions/request_type.h"
#include "components/permissions/resolvers/content_setting_permission_resolver.h"
#include "components/prefs/pref_service.h"
#include "components/url_formatter/elide_url.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

// static
bool WidevinePermissionRequest::is_test_ = false;

WidevinePermissionRequest::WidevinePermissionRequest(
    PrefService* prefs,
    const GURL& requesting_origin,
    bool for_restart)
    : PermissionRequest(
          std::make_unique<permissions::PermissionRequestData>(
              std::make_unique<permissions::ContentSettingPermissionResolver>(
                  permissions::RequestType::kWidevine),
              false,
              requesting_origin),
          base::BindRepeating(&WidevinePermissionRequest::PermissionDecided,
                              base::Unretained(this))),
      prefs_(prefs),
      for_restart_(for_restart) {
  CHECK(prefs);
}

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
  return l10n_util::GetStringUTF16(
      GetWidevinePermissionRequestTextFrangmentResourceId(for_restart_));
}
#endif

void WidevinePermissionRequest::PermissionDecided(
    PermissionDecision decision,
    bool is_final_decision,
    const permissions::PermissionRequestData& request_data) {
  // Permission granted
  if (decision == PermissionDecision::kAllow) {
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
  } else if (decision == PermissionDecision::kDeny) {
    prefs_->SetBoolean(kAskEnableWidvine, !get_dont_ask_again());
    // Cancelled
  }
}

std::u16string WidevinePermissionRequest::GetExplanatoryMessageText() const {
  return l10n_util::GetStringUTF16(IDS_WIDEVINE_INSTALL_MESSAGE);
}
