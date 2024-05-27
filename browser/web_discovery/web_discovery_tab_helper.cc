/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/web_discovery_tab_helper.h"

#include <memory>

#include "base/time/time.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/web_discovery/web_discovery_cta_util.h"
#include "brave/browser/web_discovery/web_discovery_infobar_delegate.h"
#include "brave/components/constants/url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

#if defined(TOOLKIT_VIEWS)
std::unique_ptr<infobars::InfoBar> CreateWebDiscoveryInfoBar(
    std::unique_ptr<WebDiscoveryInfoBarDelegate> delegate);
#endif

// static
void WebDiscoveryTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (contents && brave::IsRegularProfile(contents->GetBrowserContext()))
    WebDiscoveryTabHelper::CreateForWebContents(contents);
}

WebDiscoveryTabHelper::WebDiscoveryTabHelper(content::WebContents* contents)
    : content::WebContentsObserver(contents),
      content::WebContentsUserData<WebDiscoveryTabHelper>(*contents) {}

WebDiscoveryTabHelper::~WebDiscoveryTabHelper() = default;

void WebDiscoveryTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (validated_url.host() != kBraveSearchHost)
    return;

  // Only care about main frame.
  if (render_frame_host->GetParent())
    return;

  auto* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  if (!profile)
    return;

  auto* prefs = profile->GetPrefs();
  WebDiscoveryCTAState state =
      GetWebDiscoveryCTAState(prefs, GetWebDiscoveryCurrentCTAId());

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  if (!ShouldShowWebDiscoveryInfoBar(service, prefs, state))
    return;

  state.count++;
  state.last_displayed = base::Time::Now();
  SetWebDiscoveryCTAStateToPrefs(prefs, state);
  ShowInfoBar(prefs);
}

void WebDiscoveryTabHelper::ShowInfoBar(PrefService* prefs) {
  // Only support view toolkit based WebDiscovery InfoBar.
#if defined(TOOLKIT_VIEWS)
  infobars::ContentInfoBarManager::FromWebContents(web_contents())
      ->AddInfoBar(CreateWebDiscoveryInfoBar(
          std::make_unique<WebDiscoveryInfoBarDelegate>(prefs)));
#else
  NOTREACHED_IN_MIGRATION() << "We don't support WDP infobar";
#endif
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebDiscoveryTabHelper);
