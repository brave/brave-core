/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_file_select_utils.h"

#include "base/i18n/rtl.h"
#include "base/no_destructor.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/url_identity.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave {

namespace {

// If an origin is opaque but has a precursor, then returns the precursor
// origin. If the origin is not opaque, returns it unchanged. Unwrapping origins
// allows the dialog code to provide the user with a clearer picture of which
// page is actually showing the dialog.
url::Origin UnwrapOriginIfOpaque(const url::Origin& origin) {
  if (!origin.opaque()) {
    return origin;
  }

  const url::SchemeHostPort& precursor =
      origin.GetTupleOrPrecursorTupleIfOpaque();
  if (!precursor.IsValid()) {
    return origin;
  }

  return url::Origin::CreateFromNormalizedTuple(
      precursor.scheme(), precursor.host(), precursor.port());
}

}  // namespace

std::u16string GetFileSelectTitle(content::WebContents* web_contents,
                                  const url::Origin& alerting_frame_origin,
                                  const SiteTitleResourceIDMap& resource_ids) {
  // This implementation partially mirrors
  // ChromeAppModalDialogManagerDelegate::GetTitle().
  // TODO(sko) It's hard to test this behavior is in sync at this moment. Even
  // upstream tests aren't covering this. Need to figure out how we can test
  // extension and isolated web app case.
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());

  UrlIdentity url_identity = UrlIdentity::CreateFromUrl(
      profile, alerting_frame_origin.GetURL(),
      /*allowed_types*/
      {UrlIdentity::Type::kDefault, UrlIdentity::Type::kFile,
       UrlIdentity::Type::kIsolatedWebApp, UrlIdentity::Type::kChromeExtension},
      /*default_options*/ {.default_options = {}});

  if (url_identity.type == UrlIdentity::Type::kChromeExtension) {
    return url_identity.name;
  }

  if (url_identity.type == UrlIdentity::Type::kIsolatedWebApp) {
    return url_identity.name;
  }

  const auto main_frame_origin =
      web_contents->GetPrimaryMainFrame()->GetLastCommittedOrigin();
  return GetSiteFrameTitleForFileSelect(
      GetSiteFrameTitleType(main_frame_origin, alerting_frame_origin),
      alerting_frame_origin, resource_ids);
}

std::u16string GetSiteFrameTitleForFileSelect(
    SiteFrameTitleType type,
    const url::Origin& alerting_frame_origin,
    const SiteTitleResourceIDMap& resource_ids) {
  if (type == SiteFrameTitleType::kStandardSameOrigin ||
      type == SiteFrameTitleType::kStandardDifferentOrigin) {
    std::u16string origin_string =
        url_formatter::FormatOriginForSecurityDisplay(
            UnwrapOriginIfOpaque(alerting_frame_origin),
            url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS);
    return l10n_util::GetStringFUTF16(
        resource_ids.at(type),
        base::i18n::GetDisplayStringInLTRDirectionality(origin_string));
  }

  return l10n_util::GetStringUTF16(resource_ids.at(type));
}

SiteFrameTitleType GetSiteFrameTitleType(
    const url::Origin& main_frame_origin,
    const url::Origin& alerting_frame_origin) {
  // This implementation mirrors `AppModalDialogManager::GetSiteFrameTitle()`.
  // We have a test to check if the two implementations are in sync.
  //   - BraveFileSelectHelperUnitTest.GetSiteFrameTitleType_InSyncWithUpstream.
  const url::Origin unwrapped_main_frame_origin =
      UnwrapOriginIfOpaque(main_frame_origin);
  const url::Origin unwrapped_alerting_frame_origin =
      UnwrapOriginIfOpaque(alerting_frame_origin);

  const bool is_same_origin_as_main_frame =
      unwrapped_alerting_frame_origin.IsSameOriginWith(
          unwrapped_main_frame_origin);
  if (unwrapped_alerting_frame_origin.GetURL().IsStandard() &&
      !unwrapped_alerting_frame_origin.GetURL().SchemeIsFile()) {
    return is_same_origin_as_main_frame
               ? SiteFrameTitleType::kStandardSameOrigin
               : SiteFrameTitleType::kStandardDifferentOrigin;
  }
  return is_same_origin_as_main_frame
             ? SiteFrameTitleType::kNonStandardSameOrigin
             : SiteFrameTitleType::kNonStandardDifferentOrigin;
}

const SiteTitleResourceIDMap& GetFileSelectResourceIDsForOpen() {
  static const base::NoDestructor<brave::SiteTitleResourceIDMap>
      kResourcesForOpen(
          {{brave::SiteFrameTitleType::kStandardSameOrigin,
            IDS_FILE_SELECT_OPEN_TITLE},
           {brave::SiteFrameTitleType::kStandardDifferentOrigin,
            IDS_FILE_SELECT_OPEN_TITLE_IFRAME},
           {brave::SiteFrameTitleType::kNonStandardSameOrigin,
            IDS_FILE_SELECT_OPEN_TITLE_NONSTANDARD_URL},
           {brave::SiteFrameTitleType::kNonStandardDifferentOrigin,
            IDS_FILE_SELECT_OPEN_TITLE_NONSTANDARD_URL_IFRAME}});
  return *kResourcesForOpen;
}

const SiteTitleResourceIDMap& GetFileSelectResourceIDsForSave() {
  static const base::NoDestructor<brave::SiteTitleResourceIDMap>
      kResourcesForSave(
          {{brave::SiteFrameTitleType::kStandardSameOrigin,
            IDS_FILE_SELECT_SAVE_TITLE},
           {brave::SiteFrameTitleType::kStandardDifferentOrigin,
            IDS_FILE_SELECT_SAVE_TITLE_IFRAME},
           {brave::SiteFrameTitleType::kNonStandardSameOrigin,
            IDS_FILE_SELECT_SAVE_TITLE_NONSTANDARD_URL},
           {brave::SiteFrameTitleType::kNonStandardDifferentOrigin,
            IDS_FILE_SELECT_SAVE_TITLE_NONSTANDARD_URL_IFRAME}});
  return *kResourcesForSave;
}

}  // namespace brave
