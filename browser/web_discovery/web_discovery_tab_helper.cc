/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/web_discovery_tab_helper.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/url_constants.h"
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

bool ShouldCreateWebDiscoveryTabHelper(content::WebContents* contents) {
  DCHECK(contents);
  auto* context = contents->GetBrowserContext();
  if (!brave::IsRegularProfile(context))
    return false;
  auto* prefs = user_prefs::UserPrefs::Get(context);
  if (!prefs)
    return false;

  // TODO(simonhong): Check proper condition for infobar.
  return false;
}

}  // namespace

// static
void WebDiscoveryTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (contents && ShouldCreateWebDiscoveryTabHelper(contents))
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

  auto* browser = chrome::FindBrowserWithWebContents(web_contents());
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

  // TODO(simonhong): Handling new conditions for infobar.
  NOTIMPLEMENTED();
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

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebDiscoveryTabHelper);
