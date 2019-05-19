/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_profile_writer.h"

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "brave/common/importer/brave_stats.h"
#include "brave/common/importer/brave_referral.h"
#include "brave/common/importer/imported_browser_window.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/utility/importer/brave_importer.h"
#include "brave/browser/importer/brave_in_process_importer_bridge.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"

#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/task/post_task.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_tabrestore.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_data_util.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "ui/base/ui_base_types.h"

BraveProfileWriter::BraveProfileWriter(Profile* profile)
    : ProfileWriter(profile),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits({
          base::MayBlock(), base::TaskPriority::BEST_EFFORT,
          base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      consider_for_backup_(false) {
}

BraveProfileWriter::~BraveProfileWriter() {
  DCHECK(!IsInObserverList());
}

void BraveProfileWriter::AddCookies(
    const std::vector<net::CanonicalCookie>& cookies) {
  network::mojom::CookieManagerPtr cookie_manager;
  content::BrowserContext::GetDefaultStoragePartition(profile_)
      ->GetNetworkContext()
      ->GetCookieManager(mojo::MakeRequest(&cookie_manager));

  for (auto& cookie : cookies) {
    cookie_manager->SetCanonicalCookie(
        cookie,
        "https",  // secure_source
        true,  // modify_http_only
        // Fire and forget
        network::mojom::CookieManager::SetCanonicalCookieCallback());
  }
}

void BraveProfileWriter::UpdateStats(const BraveStats& stats) {
  PrefService* prefs = profile_->GetOriginalProfile()->GetPrefs();

  const uint64_t ads_blocked = prefs->GetUint64(kAdsBlocked);
  const uint64_t trackers_blocked = prefs->GetUint64(kTrackersBlocked);
  const uint64_t https_upgrades = prefs->GetUint64(kHttpsUpgrades);

  // Only update the current stats if they are less than the imported
  // stats; intended to prevent incorrectly updating the stats multiple
  // times from multiple imports.
  if (ads_blocked < uint64_t{stats.adblock_count}) {
      prefs->SetUint64(kAdsBlocked, ads_blocked + stats.adblock_count);
  }
  if (trackers_blocked < uint64_t{stats.trackingProtection_count}) {
    prefs->SetUint64(kTrackersBlocked,
                     trackers_blocked + stats.trackingProtection_count);
  }
  if (https_upgrades < uint64_t{stats.httpsEverywhere_count}) {
    prefs->SetUint64(kHttpsUpgrades,
                     https_upgrades + stats.httpsEverywhere_count);
  }
}
void BraveProfileWriter::SetBridge(BraveInProcessImporterBridge* bridge) {
  bridge_ptr_ = bridge;
}

void BraveProfileWriter::OnWalletInitialized(
    brave_rewards::RewardsService* rewards_service, uint32_t result) {
  if (result != 0 && result != 12) {  // 12: ledger::Result::WALLET_CREATED
    // Cancel the import if wallet creation failed
    std::ostringstream msg;
    msg << "An error occurred while trying to create a "
      << "wallet to restore into (result=" << result << ")";
    CancelWalletImport(msg.str());
    return;
  }

  LOG(INFO) << "Wallet creation successful\nStarting wallet recovery...";
  rewards_service->RecoverWallet(ledger_.passphrase);
}

void BraveProfileWriter::BackupWallet() {
  const base::FilePath profile_default_directory = profile_->GetPath();
  std::ostringstream backup_filename;
  backup_filename << "ledger_import_backup_"
                  << base::NumberToString(static_cast<uint64_t>(
                         base::Time::Now().ToJsTime()));

  LOG(INFO) << "Making backup of current \"ledger_state\" as "
    << "\"" << backup_filename.str() << "\"";

  base::PostTaskAndReplyWithResult(
    task_runner_.get(),
    FROM_HERE,
    base::Bind(&base::CopyFile,
      profile_default_directory.AppendASCII("ledger_state"),
      profile_default_directory.AppendASCII(backup_filename.str())),
    base::Bind(&BraveProfileWriter::OnWalletBackupComplete,
      this));
}

void BraveProfileWriter::OnWalletBackupComplete(bool result) {
  if (!result) {
    CancelWalletImport("Failed to make a backup of \"ledger_state\"");
    return;
  }

  LOG(INFO) << "Backup complete; Recovering imported wallet...";
  rewards_service_->RecoverWallet(ledger_.passphrase);
}

void BraveProfileWriter::OnWalletProperties(
  brave_rewards::RewardsService* rewards_service,
  int error_code,
  std::unique_ptr<brave_rewards::WalletProperties> properties) {
  if (error_code) {
    // Cancel the import if wallet properties failed
    // (ex: creation failed, wallet is corrupt, etc)
    std::ostringstream msg;
    msg << "An error occurred getting wallet properties "
      << "(error_code=" << error_code << ")";
    CancelWalletImport(msg.str());
    return;
  }

  // This handler will get fired periodically (until the observer
  // is removed). A backup only needs to be done if the wallet
  // already exists and this is the response from our request below
  // in `BraveProfileWriter::UpdateLedger`.
  //
  // A more proper way to do this would be to pass a transaction ID
  // into the original FetchWalletProperties() that also gets
  // propagated through to this handler.
  if (consider_for_backup_) {
    consider_for_backup_ = false;
    // Avoid overwriting Brave Rewards wallet if:
    // - it existed BEFORE import happened
    // - it has a non-zero balance
    if (properties->balance > 0) {
      std::ostringstream msg;
      msg << "Brave Rewards wallet existed before import and "
        << "has a balance of " << properties->balance << "; skipping "
        << "Brave Payments import.";
      CancelWalletImport(msg.str());
      return;
    } else {
      LOG(INFO) << "Existing wallet does not have a balance";
    }

    BackupWallet();
  }
}

void BraveProfileWriter::OnRecoverWallet(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    double balance,
    std::vector<brave_rewards::Grant> grants) {
  rewards_service->RemoveObserver(this);

  if (result) {
    // Cancel the import if wallet restore failed
    std::ostringstream msg;
    msg << "An error occurred while trying to restore "
      << "the wallet (result=" << result << ")";
    CancelWalletImport(msg.str());
    return;
  }

  LOG(INFO) << "Wallet restore successful";
  SetWalletProperties(rewards_service);

  // Set the pinned item count (rewards can detect and take action)
  PrefService* prefs = profile_->GetPrefs();
  prefs->SetInteger(kBravePaymentsPinnedItemCount, pinned_item_count_);

  // Notify the caller that import is complete
  DCHECK(bridge_ptr_);
  bridge_ptr_->FinishLedgerImport();
}

void BraveProfileWriter::CancelWalletImport(std::string msg) {
  if (IsInObserverList()) {
    rewards_service_->RemoveObserver(this);
  }
  LOG(ERROR) << msg;
  DCHECK(bridge_ptr_);
  // NOTE: calling bridge_ptr_->Cancel() may roll back previously imported
  // items. Instead, let's let the importer finish (errors are logged).
  bridge_ptr_->FinishLedgerImport();
}

void BraveProfileWriter::SetWalletProperties(brave_rewards::RewardsService*
  rewards_service) {
  // Set the preferences read from session-store-1
  auto* payments = &ledger_.settings.payments;
  rewards_service->SetPublisherAllowVideos(payments->allow_media_publishers);
  rewards_service->SetPublisherAllowNonVerified(payments->allow_non_verified);
  rewards_service->SetPublisherMinVisitTime(payments->min_visit_time);
  rewards_service->SetPublisherMinVisits(payments->min_visits);

  // Set the excluded sites
  for (const auto& publisher_key : ledger_.excluded_publishers) {
    rewards_service->ExcludePublisher(publisher_key);
  }

  // Set the recurring tips (formerly known as pinned sites)
  int sum_of_monthly_tips = 0;
  pinned_item_count_ = 0;
  for (const auto& publisher : ledger_.pinned_publishers) {
    // NOTE: this will truncate (ex: 0.90 would be 0, not 1)
    const int amount_in_bat =
        static_cast<int>((publisher.pin_percentage / 100.0) *
                         ledger_.settings.payments.contribution_amount);
    if (amount_in_bat > 0) {
      pinned_item_count_++;
      sum_of_monthly_tips += amount_in_bat;

      // Add publisher to `publisher_info`
      auto site = std::make_unique<brave_rewards::ContentSite>();
      site->id = publisher.key;
      site->verified = publisher.verified;
      site->excluded = 0;
      site->name = publisher.name;
      site->favicon_url = "";
      site->url = publisher.url;
      site->provider = publisher.provider;

      // Add `recurring_donation` entry
      rewards_service->OnDonate(publisher.key,
                                amount_in_bat,
                                true,
                                std::move(site));
    }
  }

  // Adjust monthly contribution budget
  // Some may have been allocated for recurring tips
  bool auto_contribute_enabled = payments->enabled;
  const int minimum_monthly_contribution = 10;
  new_contribution_amount_ = payments->contribution_amount;
  if (sum_of_monthly_tips > 0) {
    new_contribution_amount_ -= sum_of_monthly_tips;
    // If left over budget is too low, turn off auto-contribute
    if (new_contribution_amount_ < minimum_monthly_contribution) {
      LOG(INFO) << "Setting auto-contribute to false.\n"
        << "Recurring contributions take up " << sum_of_monthly_tips
        << " of the monthly " << payments->contribution_amount
        << " budget.\nThis leaves " << new_contribution_amount_
        << " which is less than the minimum monthly auto-contribute amount ("
        << minimum_monthly_contribution << ").";
      auto_contribute_enabled = false;
      new_contribution_amount_ = minimum_monthly_contribution;
    }
  }
  rewards_service->SetContributionAmount(new_contribution_amount_);
  rewards_service->SetAutoContribute(auto_contribute_enabled);
}

void BraveProfileWriter::UpdateLedger(const BraveLedger& ledger) {
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  if (!rewards_service_) {
    CancelWalletImport("Failed to get RewardsService for profile.");
    return;
  }

  ledger_ = BraveLedger(ledger);
  rewards_service_->IsWalletCreated(
      base::Bind(&BraveProfileWriter::OnIsWalletCreated, AsWeakPtr()));
}

void BraveProfileWriter::OnIsWalletCreated(bool created) {
  // If a wallet doesn't exist, we need to create one (needed for RecoverWallet)
  if (!created) {
    rewards_service_->AddObserver(this);
    LOG(INFO) << "Creating wallet to use for import...";
    rewards_service_->CreateWallet();
    return;
  }

  // This is the only situation where a wallet may already exist and
  // (after properties are fetched) should be considered for backup.
  consider_for_backup_ = true;
  LOG(INFO) << "Wallet exists; fetching details...";
  rewards_service_->AddObserver(this);
  rewards_service_->FetchWalletProperties();
}

void BraveProfileWriter::UpdateReferral(const BraveReferral& referral) {
  PrefService* local_state = g_browser_process->local_state();
  if (!local_state) {
    LOG(ERROR) << "Unable to get local_state! (needed to set referral info)";
    return;
  }

  if (!referral.week_of_installation.empty()) {
    LOG(INFO) << "Setting kWeekOfInstallation to "
      << "\"" << referral.week_of_installation << "\"";
    local_state->SetString(kWeekOfInstallation, referral.week_of_installation);
  }

  if (!referral.promo_code.empty() &&
    referral.promo_code.compare("none") != 0) {
    LOG(INFO) << "Setting kReferralPromoCode to "
      << "\"" << referral.promo_code << "\"";
    local_state->SetString(kReferralPromoCode, referral.promo_code);
  } else {
    local_state->ClearPref(kReferralPromoCode);
  }

  if (!referral.download_id.empty()) {
    LOG(INFO) << "Setting kReferralDownloadID to "
      << "\"" << referral.download_id << "\"";
    local_state->SetString(kReferralDownloadID, referral.download_id);
  } else {
    local_state->ClearPref(kReferralDownloadID);
  }

  if (referral.finalize_timestamp > 0) {
    LOG(INFO) << "Setting kReferralTimestamp to "
      << "\"" << referral.finalize_timestamp << "\"";
    local_state->SetTime(kReferralTimestamp,
      base::Time::FromJsTime(referral.finalize_timestamp));
  } else {
    local_state->ClearPref(kReferralTimestamp);
  }
}

Browser* OpenImportedBrowserWindow(
    const ImportedBrowserWindow& window,
    Profile* profile) {
  Browser::CreateParams params(Browser::TYPE_TABBED, profile, false);

  params.initial_bounds = gfx::Rect(window.top, window.left,
      window.width, window.height);

  ui::WindowShowState show_state = ui::SHOW_STATE_DEFAULT;
  if (window.state == "normal") {
    show_state = ui::SHOW_STATE_NORMAL;
  } else if (window.state == "minimized") {
    show_state = ui::SHOW_STATE_MINIMIZED;
  } else if (window.state == "maximized") {
    show_state = ui::SHOW_STATE_MAXIMIZED;
  } else if (window.state == "fullscreen") {
    show_state = ui::SHOW_STATE_FULLSCREEN;
  }
  params.initial_show_state = show_state;

  return new Browser(params);
}

void OpenImportedBrowserTabs(Browser* browser,
    const std::vector<ImportedBrowserTab>& tabs,
    bool pinned) {
  for (const auto tab : tabs) {
    std::vector<sessions::SerializedNavigationEntry> e;
    sessions::SerializedNavigationEntry entry;
    entry.set_virtual_url(tab.location);
    entry.set_original_request_url(tab.location);
    entry.set_is_restored(true);
    e.push_back(entry);

    chrome::AddRestoredTab(
        browser, e, browser->tab_strip_model()->count(), 0,
        "", false, pinned, true,
        base::TimeTicks::UnixEpoch(), nullptr,
        "", true /* from_session_restore */);
  }
}

int GetSelectedTabIndex(const ImportedBrowserWindow& window) {
  // The window has an activeFrameKey, which may be equal to the key for one of
  // its tabs. Find the matching tab, if one exists, and return its index in
  // the tabs vector.
  for (int i = 0; i < static_cast<int>(window.tabs.size()); i++) {
    if (window.activeFrameKey == window.tabs[i].key)
      return i;
  }

  // If there was no matching tab, default to returning the index of the
  // right-most tab.
  return window.tabs.size() - 1;
}

void ShowBrowser(Browser* browser, int selected_tab_index) {
  DCHECK(browser);
  DCHECK(browser->tab_strip_model()->count());
  browser->tab_strip_model()->ActivateTabAt(
      selected_tab_index,
      TabStripModel::UserGestureDetails(TabStripModel::GestureType::kOther));
  browser->window()->Show();
  browser->tab_strip_model()->GetActiveWebContents()->SetInitialFocus();
}

void PrependPinnedTabs(Browser* browser,
    const std::vector<ImportedBrowserTab>& tabs) {
  OpenImportedBrowserTabs(browser, tabs, true);
}

void BraveProfileWriter::UpdateWindows(
    const ImportedWindowState& windowState) {
  Browser* active = chrome::FindBrowserWithActiveWindow();
  Browser* first = nullptr;

  for (const auto window : windowState.windows) {
    Browser* browser = OpenImportedBrowserWindow(window, profile_);
    OpenImportedBrowserTabs(browser, window.tabs, false);
    ShowBrowser(browser, GetSelectedTabIndex(window));

    if (!first)
      first = browser;
  }

  PrependPinnedTabs(first, windowState.pinnedTabs);

  // Re-focus the window that was originally focused before import.
  if (active)
    active->window()->Show();
}


// NOTE: the strings used as keys match the values found in Muon:
// browser-laptop/js/data/searchProviders.js
// Providers that aren't in this map are no longer prepopulated (Amazon,
// Ecosia, GitHub, etc.) and the current default provider won't be changed.
const std::map<std::string,
        const TemplateURLPrepopulateData::PrepopulatedEngine>
    importable_engines = {
      {"Bing", TemplateURLPrepopulateData::bing},
      {"DuckDuckGo", TemplateURLPrepopulateData::duckduckgo},
      {"Google", TemplateURLPrepopulateData::google},
      {"Qwant", TemplateURLPrepopulateData::qwant},
      {"StartPage", TemplateURLPrepopulateData::startpage},
    };

void BraveProfileWriter::UpdateSettings(const SessionStoreSettings& settings) {
  int default_search_engine_id =
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_INVALID;

  // Set the default search engine
  TemplateURLService* url_service =
      TemplateURLServiceFactory::GetForProfile(profile_);
  if (url_service) {
    auto it = importable_engines.find(settings.default_search_engine);
    if (it != importable_engines.end()) {
      const TemplateURLPrepopulateData::PrepopulatedEngine engine = it->second;
      const std::unique_ptr<TemplateURLData> template_data =
          TemplateURLDataFromPrepopulatedEngine(engine);
      default_search_engine_id = engine.id;
      LOG(INFO) << "Setting default search engine to "
          << settings.default_search_engine;
      TemplateURL provider_url(*template_data);
      url_service->SetUserSelectedDefaultSearchProvider(&provider_url);
    }
  }

  // Save alternate engine (for private tabs) to preferences
  PrefService* prefs = profile_->GetPrefs();
  if (prefs) {
    prefs->SetBoolean(kUseAlternativeSearchEngineProvider,
        settings.use_alternate_private_search_engine);

    // Provider for Tor tabs
    if (settings.use_alternate_private_search_engine_tor) {
      // if enabled, set as a default. This gets resolved to either
      // DDG or Qwant in TorWindowSearchEngineProviderService
      prefs->SetInteger(kAlternativeSearchEngineProviderInTor,
           TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_INVALID);
    } else {
      // if disabled, set to same as regular search engine
      prefs->SetInteger(kAlternativeSearchEngineProviderInTor,
          default_search_engine_id);
    }
  }
}
