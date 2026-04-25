/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/web_discovery_tab_helper.h"

#include <memory>
#include <utility>

#include "base/time/time.h"
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

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
#include "brave/browser/web_discovery/web_discovery_service_factory.h"
#include "brave/components/web_discovery/browser/web_discovery_service.h"
#include "brave/components/web_discovery/common/features.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#endif

namespace web_discovery {

// static
void WebDiscoveryTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (!contents) {
    return;
  }

  auto* profile = Profile::FromBrowserContext(contents->GetBrowserContext());
  if (!profile || !profile->IsRegularProfile()) {
    return;
  }

  WebDiscoveryTabHelper::CreateForWebContents(contents);
}

WebDiscoveryTabHelper::WebDiscoveryTabHelper(content::WebContents* contents)
    : content::WebContentsObserver(contents),
      content::WebContentsUserData<WebDiscoveryTabHelper>(*contents) {}

WebDiscoveryTabHelper::~WebDiscoveryTabHelper() = default;

void WebDiscoveryTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
  MaybeExtractFromPage(render_frame_host, validated_url);
#endif

  if (validated_url.host() != kBraveSearchHost) {
    return;
  }

  // Only care about main frame.
  if (render_frame_host->GetParent()) {
    return;
  }

  auto* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  if (!profile) {
    return;
  }

  auto* prefs = profile->GetPrefs();
  WebDiscoveryCTAState state =
      GetWebDiscoveryCTAState(prefs, GetWebDiscoveryCurrentCTAId());

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  if (!ShouldShowWebDiscoveryInfoBar(service, prefs, state)) {
    return;
  }

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
#endif
}

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
void WebDiscoveryTabHelper::MaybeExtractFromPage(
    content::RenderFrameHost* render_frame_host,
    const GURL& url) {
  if (!base::FeatureList::IsEnabled(features::kBraveWebDiscoveryNative)) {
    return;
  }
  auto* web_discovery_service =
      WebDiscoveryServiceFactory::GetForBrowserContext(
          render_frame_host->GetBrowserContext());
  if (!web_discovery_service) {
    return;
  }
  if (!render_frame_host->IsInPrimaryMainFrame()) {
    return;
  }
  if (!web_discovery_service->ShouldExtractFromPage(url, render_frame_host)) {
    return;
  }
  mojo::Remote<mojom::DocumentExtractor> remote;
  render_frame_host->GetRemoteInterfaces()->GetInterface(
      remote.BindNewPipeAndPassReceiver());
  web_discovery_service->StartExtractingFromPage(url, std::move(remote));
}
#endif

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebDiscoveryTabHelper);

}  // namespace web_discovery
