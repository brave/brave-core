// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/net/net_error_tab_helper.h"

#include "build/build_config.h"
#include "chrome/common/pref_names.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/tabs/public/tab_interface.h"
#include "url/gurl.h"

// Overrides the upstream class to be a no-op. Upstream sends DNS probes when a
// page fails with a DNS error, which Brave blocks. It observes nothing and
// binds none of its mojo interfaces.
//
// The private non-virtual helpers (OnMainFrameDnsError, InitializePref,
// ProbesAllowed) are left undefined on purpose: nothing calls them, and
// defining every member would trigger -Wunused-private-field.

namespace chrome_browser_net {

DEFINE_USER_DATA(NetErrorTabHelper);

NetErrorTabHelper::NetErrorTabHelper(tabs::TabInterface& tab,
                                     content::WebContents* contents)
    : network_diagnostics_receivers_(nullptr, this),
      network_easter_egg_receivers_(nullptr, this),
      net_error_page_support_(nullptr, this),
      dns_error_active_(false),
      dns_error_page_committed_(false),
#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
      is_showing_download_button_in_error_page_(false),
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)
      dns_probe_status_(error_page::DNS_PROBE_NOT_RUN),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this) {
}

NetErrorTabHelper::~NetErrorTabHelper() = default;

// static
NetErrorTabHelper* NetErrorTabHelper::From(tabs::TabInterface* tab) {
  return tab ? Get(tab->GetUnownedUserDataHost()) : nullptr;
}

// static
void NetErrorTabHelper::BindNetErrorPageSupport(
    mojo::PendingAssociatedReceiver<chrome::mojom::NetErrorPageSupport>
        receiver,
    content::RenderFrameHost* rfh) {}

// static
void NetErrorTabHelper::BindNetworkDiagnostics(
    mojo::PendingAssociatedReceiver<chrome::mojom::NetworkDiagnostics> receiver,
    content::RenderFrameHost* rfh) {}

// static
void NetErrorTabHelper::BindNetworkEasterEgg(
    mojo::PendingAssociatedReceiver<chrome::mojom::NetworkEasterEgg> receiver,
    content::RenderFrameHost* rfh) {}

// static
void NetErrorTabHelper::set_state_for_testing(TestingState state) {}

// static
void NetErrorTabHelper::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* prefs) {
  // Still registered, since sync's pref model associator refers to it.
  prefs->RegisterIntegerPref(prefs::kNetworkEasterEggHighScore, 0,
                             user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

void NetErrorTabHelper::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {}

void NetErrorTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {}

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
void NetErrorTabHelper::DownloadPageLater() {}

void NetErrorTabHelper::SetIsShowingDownloadButtonInErrorPage(
    bool showing_download_button) {}
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)

#if BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_ANDROID)
void NetErrorTabHelper::ShowPortalSignin() {}
#endif

void NetErrorTabHelper::StartDnsProbe() {}

void NetErrorTabHelper::SendInfo() {}

void NetErrorTabHelper::OnDnsProbeFinished(error_page::DnsProbeStatus result) {}

void NetErrorTabHelper::RunNetworkDiagnostics(const GURL& url) {}

void NetErrorTabHelper::GetHighScore(GetHighScoreCallback callback) {}

void NetErrorTabHelper::UpdateHighScore(uint32_t high_score) {}

void NetErrorTabHelper::ResetHighScore() {}

void NetErrorTabHelper::RunNetworkDiagnosticsHelper(
    const std::string& sanitized_url) {}

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
void NetErrorTabHelper::DownloadPageLaterHelper(const GURL& url) {}
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)

}  // namespace chrome_browser_net
