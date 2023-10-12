// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_vpn/vpn_panel_handler.h"
#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"

#include <utility>

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace {

GURL GetURLForUIType(const std::string& type, GURL manage_url) {
  if (type == "recover" || type == "checkout") {
    DCHECK(manage_url.is_valid());
    std::string query = "intent=" + type + "&product=vpn";
    GURL::Replacements replacements;
    replacements.SetQueryStr(query);
    return manage_url.ReplaceComponents(replacements);
  } else if (type == "privacy") {
    return GURL("https://brave.com/privacy/browser/#vpn");
  } else if (type == "about") {
    return GURL(brave_vpn::kAboutUrl);
  }
  DCHECK_EQ(type, "manage");
  return manage_url;
}

bool ShouldOpenSingletonTab(const std::string& type) {
  return type == "manage" || type == "privacy" || type == "about";
}

void ShowSingletonVPNTab(Browser* browser, const GURL& url) {
  for (auto i = 0; i < browser->tab_strip_model()->GetTabCount(); i++) {
    auto* web_contents = browser->tab_strip_model()->GetWebContentsAt(i);
    const GURL& contents_url = web_contents->GetVisibleURL();
    bool is_equal = contents_url.SchemeIs(url.scheme()) &&
                    contents_url.DomainIs(url.host()) &&
                    contents_url.path() == url.path();
    if (is_equal) {
      browser->tab_strip_model()->ActivateTabAt(i);
      return;
    }
  }
  chrome::AddTabAt(browser, url, -1, true);
}

}  // namespace

VPNPanelHandler::VPNPanelHandler(
    mojo::PendingReceiver<brave_vpn::mojom::PanelHandler> receiver,
    VPNPanelUI* panel_controller,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      panel_controller_(panel_controller),
      profile_(profile) {}

VPNPanelHandler::~VPNPanelHandler() = default;

void VPNPanelHandler::ShowUI() {
  auto embedder = panel_controller_->embedder();
  brave_vpn::BraveVpnService* vpn_service =
      brave_vpn::BraveVpnServiceFactory::GetForProfile(profile_);
  CHECK(vpn_service);
  if (embedder) {
    embedder->ShowUI();
    vpn_service->ReloadPurchasedState();
  }
}

void VPNPanelHandler::CloseUI() {
  auto embedder = panel_controller_->embedder();
  if (embedder) {
    embedder->CloseUI();
  }
}

void VPNPanelHandler::OpenVpnUIUrl(
    const std::string& type,
    brave_vpn::mojom::ProductUrlsPtr product_urls) {
  auto* browser = chrome::FindLastActiveWithProfile(profile_);
  auto url = GetURLForUIType(type, GURL(product_urls->manage));
  if (ShouldOpenSingletonTab(type)) {
    ShowSingletonVPNTab(browser, url);
  } else {
    chrome::AddTabAt(browser, url, -1, true);
  }
}

void VPNPanelHandler::OpenVpnUI(const std::string& type) {
  brave_vpn::BraveVpnService* vpn_service =
      brave_vpn::BraveVpnServiceFactory::GetForProfile(profile_);
  CHECK(vpn_service);
  vpn_service->GetProductUrls(base::BindOnce(&VPNPanelHandler::OpenVpnUIUrl,
                                             base::Unretained(this), type));
}
