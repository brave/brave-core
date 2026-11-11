// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/private_new_tab_page/brave_private_new_tab_page_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_private_new_tab_ui/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/tor_launcher_factory.h"
#endif  // BUILDFLAG(ENABLE_TOR)

namespace {

constexpr auto kStuckPeriod = base::Seconds(45);

const char* GetDisclaimerDismissedPref(Profile* profile) {
  return profile->IsTor()
             ? brave_private_new_tab::prefs::kBraveTorWindowDisclaimerDismissed
             : brave_private_new_tab::prefs::
                   kBravePrivateWindowDisclaimerDismissed;
}

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

  auto window_open_disposition = WindowOpenDisposition::kCurrentTab;

  if (open_new_tab) {
    window_open_disposition = WindowOpenDisposition::kNewBackgroundTab;
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
                             WindowOpenDisposition::kNewForegroundTab,
                             ui::PageTransition::PAGE_TRANSITION_LINK, false),
      /*navigation_handle_callback=*/{});
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
