// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/private_new_tab_page/brave_private_new_tab_page_handler.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_private_new_tab_ui/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/tor/tor_launcher_factory.h"
#endif  // BUILDFLAG(ENABLE_TOR)

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/brave_vpn/vpn_utils.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

namespace {

using brave_private_new_tab::prefs::VpnPromoCache;

constexpr auto kStuckPeriod = base::Seconds(45);

const char* GetDisclaimerDismissedPref(Profile* profile) {
  return profile->IsTor()
             ? brave_private_new_tab::prefs::kBraveTorWindowDisclaimerDismissed
             : brave_private_new_tab::prefs::
                   kBravePrivateWindowDisclaimerDismissed;
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
// Longest the page's VPN card stays a skeleton before falling back.
constexpr auto kVpnPromoTimeout = base::Seconds(10);

// The VPN service only exists for regular profiles, and is null when VPN isn't
// available for it (feature off, policy-disabled, unsupported region, ...).
brave_vpn::BraveVpnService* GetVpnService(Profile* profile) {
  Profile* original_profile = profile->GetOriginalProfile();
  if (!brave_vpn::IsBraveVPNEnabled(original_profile)) {
    return nullptr;
  }
  return brave_vpn::BraveVpnServiceFactory::GetForProfile(original_profile);
}

// std::nullopt while loading. The trial offer only makes sense for people who
// never subscribed, so expired or failing subscriptions don't get the promo.
std::optional<bool> ShouldShowVpnPromoForState(
    brave_vpn::mojom::PurchasedState state) {
  switch (state) {
    case brave_vpn::mojom::PurchasedState::LOADING:
      return std::nullopt;
    case brave_vpn::mojom::PurchasedState::NOT_PURCHASED:
      return true;
    case brave_vpn::mojom::PurchasedState::PURCHASED:
    case brave_vpn::mojom::PurchasedState::SESSION_EXPIRED:
    case brave_vpn::mojom::PurchasedState::FAILED:
    case brave_vpn::mojom::PurchasedState::OUT_OF_CREDENTIALS:
      return false;
  }
  return false;
}
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

}  // namespace

// static
bool BravePrivateNewTabPageHandler::IsDisclaimerDismissed(Profile* profile) {
  return profile->GetOriginalProfile()->GetPrefs()->GetBoolean(
      GetDisclaimerDismissedPref(profile));
}

// static
bool BravePrivateNewTabPageHandler::IsTorDisabled(Profile* profile) {
#if BUILDFLAG(ENABLE_TOR)
  return TorProfileServiceFactory::IsTorDisabled(profile->GetOriginalProfile());
#else
  return true;
#endif  // BUILDFLAG(ENABLE_TOR)
}

// static
bool BravePrivateNewTabPageHandler::IsTorConnected() {
#if BUILDFLAG(ENABLE_TOR)
  auto* tor_launcher_factory = TorLauncherFactory::GetInstance();
  return tor_launcher_factory && tor_launcher_factory->IsTorConnected();
#else
  return false;
#endif  // BUILDFLAG(ENABLE_TOR)
}

// static
std::optional<bool> BravePrivateNewTabPageHandler::GetInitialShouldShowVpnPromo(
    Profile* profile) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* vpn_service = GetVpnService(profile);
  if (!vpn_service) {
    return false;
  }

  // The service answers synchronously today. The ref-counted holder keeps a
  // late reply safe, and that case simply falls back to the cache below.
  auto state = base::MakeRefCounted<
      base::RefCountedData<std::optional<brave_vpn::mojom::PurchasedState>>>();
  vpn_service->GetPurchasedState(base::BindOnce(
      [](scoped_refptr<base::RefCountedData<
             std::optional<brave_vpn::mojom::PurchasedState>>> state,
         brave_vpn::mojom::PurchasedInfoPtr info) {
        state->data = info->state;
      },
      state));
  if (state->data) {
    if (auto show = ShouldShowVpnPromoForState(*state->data)) {
      return show;
    }
  }

  // Still loading: use what an earlier page saw.
  switch (static_cast<VpnPromoCache>(
      profile->GetOriginalProfile()->GetPrefs()->GetInteger(
          brave_private_new_tab::prefs::kBravePrivateWindowVpnPromoCache))) {
    case VpnPromoCache::kShow:
      return true;
    case VpnPromoCache::kHide:
      return false;
    case VpnPromoCache::kUnknown:
      return std::nullopt;
  }
  return std::nullopt;
#else
  return false;
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
}

BravePrivateNewTabPageHandler::BravePrivateNewTabPageHandler(
    Profile* profile,
    content::WebContents* web_contents,
    mojo::PendingReceiver<brave_private_new_tab::mojom::PageHandler> receiver)
    : profile_(profile),
      web_contents_(web_contents),
      receiver_(this, std::move(receiver)) {
#if BUILDFLAG(ENABLE_TOR)
  tor_launcher_factory_ = TorLauncherFactory::GetInstance();
  if (tor_launcher_factory_)
    tor_launcher_factory_->AddObserver(this);
#endif
}

BravePrivateNewTabPageHandler::~BravePrivateNewTabPageHandler() {
#if BUILDFLAG(ENABLE_TOR)
  if (tor_launcher_factory_)
    tor_launcher_factory_->RemoveObserver(this);
#endif
}

void BravePrivateNewTabPageHandler::SetClientPage(
    mojo::PendingRemote<brave_private_new_tab::mojom::PrivateTabPage> page) {
  page_.Bind(std::move(page));
}

void BravePrivateNewTabPageHandler::SetDisclaimerDismissed(bool dismissed) {
  CHECK(profile_);

  profile_->GetOriginalProfile()->GetPrefs()->SetBoolean(
      GetDisclaimerDismissedPref(profile_), dismissed);
}

void BravePrivateNewTabPageHandler::GetIsTorConnected(
    GetIsTorConnectedCallback callback) {
  bool is_connected = false;
#if BUILDFLAG(ENABLE_TOR)
  if (tor_launcher_factory_)
    is_connected = tor_launcher_factory_->IsTorConnected();
#endif

  std::move(callback).Run(is_connected);
}

using ConnectionStatus = brave_private_new_tab::mojom::ConnectionStatus;

void BravePrivateNewTabPageHandler::GoToBraveSearch(const std::string& input,
                                                    bool open_new_tab) {
  CHECK(profile_);

  auto provider_data = TemplateURLDataFromPrepopulatedEngine(
      profile_->IsTor() ? TemplateURLPrepopulateData::brave_search_tor
                        : TemplateURLPrepopulateData::brave_search);
  auto t_url = std::make_unique<TemplateURL>(*provider_data);
  SearchTermsData search_terms_data;

  auto url = GURL(t_url->url_ref().ReplaceSearchTerms(
      TemplateURLRef::SearchTermsArgs(base::UTF8ToUTF16(input)),
      search_terms_data));

  auto window_open_disposition = WindowOpenDisposition::CURRENT_TAB;

  if (open_new_tab) {
    window_open_disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
  }

  web_contents_->OpenURL(
      content::OpenURLParams(url, content::Referrer(), window_open_disposition,
                             ui::PageTransition::PAGE_TRANSITION_FORM_SUBMIT,
                             false),
      /*navigation_handle_callback=*/{});
}

void BravePrivateNewTabPageHandler::GoToBraveSupport() {
  Profile* profile = profile_;
  if (profile_->IsTor()) {
    profile = profile_->GetOriginalProfile();
  }

  content::WebContents* web_contents = nullptr;

  auto* browser =
      ProfileBrowserCollection::GetForProfile(profile)->GetLastActiveBrowser();
  if (browser && browser->GetTabStripModel()) {
    web_contents = browser->GetTabStripModel()->GetActiveWebContents();
  }

  if (!web_contents)
    web_contents = web_contents_;

  web_contents->OpenURL(
      content::OpenURLParams(GURL("https://support.brave.app/"),
                             content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PageTransition::PAGE_TRANSITION_LINK, false),
      /*navigation_handle_callback=*/{});
}

void BravePrivateNewTabPageHandler::GetShouldShowVpnPromo(
    GetShouldShowVpnPromoCallback callback) {
  CHECK(profile_);
  // Tor windows never show the promo, and must not touch the shared cache.
  if (profile_->IsTor()) {
    std::move(callback).Run(false);
    return;
  }
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (auto* vpn_service = GetVpnService(profile_)) {
    vpn_promo_callbacks_.push_back(std::move(callback));
    if (!vpn_promo_timeout_.IsRunning()) {
      vpn_promo_timeout_.Start(
          FROM_HERE, kVpnPromoTimeout,
          base::BindOnce(&BravePrivateNewTabPageHandler::OnVpnPromoTimeout,
                         base::Unretained(this)));
    }
    // Observing first so a load that finishes after GetPurchasedState replies
    // LOADING still resolves the pending callbacks.
    if (!is_observing_vpn_) {
      Observe(vpn_service);
      is_observing_vpn_ = true;
    }
    vpn_service->GetPurchasedState(
        base::BindOnce(&BravePrivateNewTabPageHandler::OnGetPurchasedState,
                       weak_factory_.GetWeakPtr()));
    return;
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
  vpn_promo_callbacks_.push_back(std::move(callback));
  ResolveVpnPromo(/*show=*/false, /*cache=*/true);
}

void BravePrivateNewTabPageHandler::OpenVpnCheckout() {
  CHECK(profile_);
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  // Tor windows never offer the promo, and opening a regular-profile tab from
  // one would link the two sessions.
  if (profile_->IsTor()) {
    return;
  }
  auto* vpn_service = GetVpnService(profile_);
  if (!vpn_service) {
    return;
  }

  // The purchase must happen in the regular profile so the credentials are
  // available outside of private windows.
  const GURL url = brave_vpn::GetManageURLForUIType(
      brave_vpn::mojom::ManageURLType::CHECKOUT,
      GURL(brave_vpn::GetManageUrl(vpn_service->GetCurrentEnvironment())));
  NavigateParams params(profile_->GetOriginalProfile(), url,
                        ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  Navigate(&params);
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
}

void BravePrivateNewTabPageHandler::OpenTorWindow() {
  CHECK(profile_);
#if BUILDFLAG(ENABLE_TOR)
  if (profile_->IsTor() || IsTorDisabled(profile_)) {
    return;
  }
  brave::NewOffTheRecordWindowTor(profile_->GetOriginalProfile());
#endif  // BUILDFLAG(ENABLE_TOR)
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
void BravePrivateNewTabPageHandler::OnPurchasedStateChanged(
    brave_vpn::mojom::PurchasedState state,
    const std::optional<std::string>& description) {
  if (auto show = ShouldShowVpnPromoForState(state)) {
    ResolveVpnPromo(*show, /*cache=*/true);
  }
}

void BravePrivateNewTabPageHandler::OnGetPurchasedState(
    brave_vpn::mojom::PurchasedInfoPtr info) {
  OnPurchasedStateChanged(info->state, info->description);
}
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

void BravePrivateNewTabPageHandler::ResolveVpnPromo(bool show, bool cache) {
  CHECK(profile_);
  vpn_promo_timeout_.Stop();
  if (cache) {
    profile_->GetOriginalProfile()->GetPrefs()->SetInteger(
        brave_private_new_tab::prefs::kBravePrivateWindowVpnPromoCache,
        static_cast<int>(show ? VpnPromoCache::kShow : VpnPromoCache::kHide));
  }
  for (auto& callback : std::exchange(vpn_promo_callbacks_, {})) {
    std::move(callback).Run(show);
  }
}

void BravePrivateNewTabPageHandler::OnVpnPromoTimeout() {
  // Fall back to the Tor card without caching a guess.
  ResolveVpnPromo(/*show=*/false, /*cache=*/false);
}

void BravePrivateNewTabPageHandler::OnTorCircuitEstablished(bool result) {
  stuck_timer_.Stop();
  if (page_) {
    page_.get()->OnTorCircuitStatus(result
                                        ? ConnectionStatus::kConnected
                                        : ConnectionStatus::kConnectionStuck);
    page_.get()->OnTorCircuitEstablished(result);
  }
}

void BravePrivateNewTabPageHandler::OnTorInitializing(
    const std::string& percentage,
    const std::string& message) {
  stuck_timer_.Start(
      FROM_HERE, kStuckPeriod,
      base::BindOnce(&BravePrivateNewTabPageHandler::OnTorCircuitTimer,
                     base::Unretained(this),
                     ConnectionStatus::kConnectionSlow));
  if (page_) {
    page_.get()->OnTorInitializing(percentage, message);
    page_.get()->OnTorCircuitStatus(ConnectionStatus::kConnecting);
  }
}

void BravePrivateNewTabPageHandler::OnTorCircuitTimer(ConnectionStatus status) {
  if (!page_)
    return;

  if (status == ConnectionStatus::kConnectionSlow) {
    // First time shot of stuck_timer_  means that 'connection is slow' we
    // take another chance to connect and then notify that we get stuck.
    stuck_timer_.Start(
        FROM_HERE, kStuckPeriod,
        base::BindOnce(&BravePrivateNewTabPageHandler::OnTorCircuitTimer,
                       base::Unretained(this),
                       ConnectionStatus::kConnectionStuck));
  }

  page_.get()->OnTorCircuitStatus(status);
}
