/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_profile_writer.h"
#include "brave/common/importer/brave_stats.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/utility/importer/brave_importer.h"
#include "brave/browser/importer/brave_in_process_importer_bridge.h"

#include "base/files/file_util.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/task/post_task.h"

#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "components/prefs/pref_service.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

#include <sstream>

BraveProfileWriter::BraveProfileWriter(Profile* profile)
    : ProfileWriter(profile),
    task_runner_(base::CreateSequencedTaskRunnerWithTraits({
      base::MayBlock(), base::TaskPriority::BEST_EFFORT,
      base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {
    }

BraveProfileWriter::~BraveProfileWriter() {
  CHECK(!IsInObserverList());
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
        true,  // secure_source
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

void BraveProfileWriter::OnWalletInitialized(brave_rewards::RewardsService*
  rewards_service, int error_code) {
  if (error_code) {
    rewards_service->RemoveObserver(this);
    // Cancel the import if wallet creation failed
    LOG(ERROR) << "An error occurred while trying to create a wallet to "
      << "restore into (error_code=" << error_code << ")";
    bridge_ptr_->Cancel();
    return;
  }

  LOG(INFO) << "Wallet creation successful\nStarting wallet recovery...";
  rewards_service->RecoverWallet(ledger_.passphrase);
}

void BraveProfileWriter::BackupWallet() {
  const base::FilePath profile_default_directory = profile_->GetPath();
  std::ostringstream backup_filename;
  backup_filename << "ledger_import_backup_"
    << base::NumberToString((unsigned long long)base::Time::Now().ToJsTime());

  LOG(INFO) << "Making backup of current \"ledger_state\" as "
    << "\"" << backup_filename.str() << "\"";

  base::PostTaskAndReplyWithResult<bool, bool>(
    task_runner_.get(),
    FROM_HERE,
    base::Bind(&base::CopyFile,
      profile_default_directory.AppendASCII("ledger_state"),
      profile_default_directory.AppendASCII(backup_filename.str())),
    base::Bind(&BraveProfileWriter::OnWalletBackupComplete,
      base::Unretained(this)));
}

void BraveProfileWriter::OnWalletBackupComplete(bool result) {
  if (!result) {
    rewards_service_->RemoveObserver(this);
    LOG(ERROR) << "Failed to make a backup of \"ledger_state\"";
    bridge_ptr_->Cancel();
    return;
  }

  LOG(INFO) << "Backup complete; Recovering imported wallet...";
  rewards_service_->RecoverWallet(ledger_.passphrase);
}

void BraveProfileWriter::OnWalletProperties(
  brave_rewards::RewardsService* rewards_service,
  int error_code,
  brave_rewards::WalletProperties* properties) {
  // Avoid overwriting Brave Rewards wallet if:
  // - it existed BEFORE import happened
  // - it has a non-zero balance
  // - caller didn't pass `true` for clobber_wallet
  if (properties->balance > 0) {
    if (!ledger_.clobber_wallet) {
      rewards_service->RemoveObserver(this);
      LOG(ERROR) << "Brave Rewards wallet existed before import and "
        << "has a balance of " << properties->balance << "; skipping "
        << "Brave Payments import.";
      bridge_ptr_->Cancel();
      return;
    }
    LOG(INFO) << "Existing wallet has a balance (" << properties->balance
      << ") and clobber_wallet is true; recover will be overwriting "
      << "this wallet (after making a backup)";
  } else {
    LOG(INFO) << "Existing wallet does not have a balance";
  }

  BackupWallet();
}

void BraveProfileWriter::OnRecoverWallet(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    double balance,
    std::vector<brave_rewards::Grant> grants) {
  rewards_service->RemoveObserver(this);

  if (result) {
    // Cancel the import if wallet restore failed
    LOG(ERROR) << "An error occurred while trying to restore the wallet "
      << "(result=" << result << ")";
    bridge_ptr_->Cancel();
    return;
  }

  LOG(INFO) << "Wallet restore successful";
  SetWalletProperties(rewards_service);

  // Set the pinned item count (rewards can detect and take action)
  PrefService* prefs = profile_->GetOriginalProfile()->GetPrefs();
  prefs->SetInteger(kBravePaymentsPinnedItemCount, pinned_item_count_);

  // Notify the caller that import is complete
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
  for (const auto& item : ledger_.pinned_publishers) {
    const auto& publisher_key = item.first;
    const auto& pin_percentage = item.second;
    // NOTE: this will truncate (ex: 0.90 would be 0, not 1)
    const int amount_in_bat = (int)((pin_percentage / 100.0) *
      ledger_.settings.payments.contribution_amount);
    if (amount_in_bat > 0) {
      pinned_item_count_++;
      sum_of_monthly_tips += amount_in_bat;
      rewards_service->OnDonate(publisher_key, amount_in_bat, true);
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
  rewards_service->SetUserChangedContribution();
  rewards_service->SetContributionAmount(new_contribution_amount_);
  rewards_service->SetAutoContribute(auto_contribute_enabled);
}

void BraveProfileWriter::UpdateLedger(const BraveLedger& ledger) {
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  if (!rewards_service_) {
    LOG(ERROR) << "Failed to get RewardsService for profile.";
    bridge_ptr_->Cancel();
    return;
  }

  ledger_ = BraveLedger(ledger);

  // If a wallet doesn't exist, we need to create one (needed for RecoverWallet)
  if (!rewards_service_->IsWalletCreated()) {
    rewards_service_->AddObserver(this);
    LOG(INFO) << "Creating wallet to use for import...";
    rewards_service_->CreateWallet();
    return;
  }

  LOG(INFO) << "Wallet exists; fetching details...";
  rewards_service_->AddObserver(this);
  rewards_service_->FetchWalletProperties();
}
