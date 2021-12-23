/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/web_discovery_tab_helper.h"

#include "brave/browser/ui/browser_dialogs.h"
#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/page.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace {

bool ShouldCreateWebDiscoveryTabHelper(PrefService* prefs) {
  DCHECK(prefs);
  return !prefs->GetBoolean(kDontAskEnableWebDiscovery) &&
         prefs->GetInteger(kBraveSearchVisitCount) < 20;
}

}  // namespace

// static
void WebDiscoveryTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  auto* prefs = user_prefs::UserPrefs::Get(contents->GetBrowserContext());
  if (prefs && ShouldCreateWebDiscoveryTabHelper(prefs))
    WebDiscoveryTabHelper::CreateForWebContents(contents);
}

WebDiscoveryTabHelper::WebDiscoveryTabHelper(content::WebContents* contents)
    : content::WebContentsObserver(contents),
      content::WebContentsUserData<WebDiscoveryTabHelper>(*contents) {}

WebDiscoveryTabHelper::~WebDiscoveryTabHelper() = default;

void WebDiscoveryTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  // Only care about main frame.
  if (render_frame_host->GetParent())
    return;

  if (validated_url != base::StringPiece(kBraveSearchUrl))
    return;

  auto* browser = chrome::FindBrowserWithWebContents(&GetWebContents());
  if (!browser)
    return;

  auto* profile = browser->profile();
  if (!profile)
    return;

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  if (!service)
    return;

  auto* prefs = profile->GetPrefs();
  if (!prefs)
    return;

  if (!NeedVisitCountHandling(prefs, service))
    return;

  IncreaseBraveSearchVisitCount(prefs);

  if (ShouldShowWebDiscoveryDialog(prefs))
    brave::ShowWebDiscoveryDialog(browser, &GetWebContents());
}

bool WebDiscoveryTabHelper::NeedVisitCountHandling(
    PrefService* prefs,
    TemplateURLService* template_service) {
  DCHECK(prefs && template_service);

  if (prefs->GetBoolean(kDontAskEnableWebDiscovery))
    return false;

  if (!IsBraveSearchDefault(template_service))
    return false;

  if (prefs->GetBoolean(kWebDiscoveryEnabled))
    return false;

  if (prefs->GetInteger(kBraveSearchVisitCount) >= 20)
    return false;

  return true;
}

bool WebDiscoveryTabHelper::IsBraveSearchDefault(
    TemplateURLService* template_service) {
  DCHECK(template_service);

  auto* template_url = template_service->GetDefaultSearchProvider();
  if (!template_url)
    return false;
  return template_url->prepopulate_id() ==
         TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE;
}

bool WebDiscoveryTabHelper::ShouldShowWebDiscoveryDialog(PrefService* prefs) {
  DCHECK(prefs);

  const int visit_count = prefs->GetInteger(kBraveSearchVisitCount);
  return (visit_count == 3 || visit_count == 10 || visit_count == 20);
}

void WebDiscoveryTabHelper::IncreaseBraveSearchVisitCount(PrefService* prefs) {
  DCHECK(prefs);

  const int visit_count = prefs->GetInteger(kBraveSearchVisitCount) + 1;
  // Don't need to increase anymore. We don't show again after 20th visit.
  if (visit_count > 20)
    return;

  prefs->SetInteger(kBraveSearchVisitCount, visit_count);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebDiscoveryTabHelper);
