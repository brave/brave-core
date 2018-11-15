/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_profile_writer.h"
#include "brave/common/importer/brave_ledger.h"
#include "brave/common/importer/brave_stats.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/utility/importer/brave_importer.h"
#include "brave/browser/importer/brave_in_process_importer_bridge.h"

#include "base/time/time.h"
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

BraveProfileWriter::BraveProfileWriter(Profile* profile)
    : ProfileWriter(profile) {}

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

void BraveProfileWriter::OnRecoverWallet(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    double balance,
    std::vector<brave_rewards::Grant> grants) {
  rewards_service_->RemoveObserver(this);

  if (result) {
    LOG(ERROR) << "An error occurred while trying to restore the wallet (result=" << result << ")";
  } else {
    LOG(INFO) << "Wallet restore successful";
    rewards_service_->SetContributionAmount(new_contribution_amount_);

    // Set the pinned item count (rewards can detect and take action)
    PrefService* prefs = profile_->GetOriginalProfile()->GetPrefs();
    prefs->SetUint64(kBravePaymentsPinnedItemCount, pinned_item_count_);
  }

  // Notify the caller that import is complete
  bridge_ptr_->FinishLedgerImport();
}

void BraveProfileWriter::UpdateLedger(const BraveLedger& ledger) {
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  if (!rewards_service_) {
    LOG(ERROR) << "Failed to get RewardsService for profile.";
    return;
  }
  rewards_service_->AddObserver(this);

  // TODO: uncomment me
  // // Avoid overwriting Brave Rewards wallet if one already exists.
  // if (!ledger.clobber_wallet && rewards_service_->IsWalletCreated()) {
  //   LOG(ERROR) << "Brave Rewards wallet already exists; skipping Brave Payments import.";
  //   // TODO communicate this failure mode to the user
  //   return;
  // }

  // Set the preferences read from session-store-1
  auto* payments = &ledger.settings.payments;
  rewards_service_->SetPublisherAllowVideos(payments->allow_media_publishers);
  rewards_service_->SetPublisherAllowNonVerified(payments->allow_non_verified);
  rewards_service_->SetPublisherMinVisitTime(payments->min_visit_time);
  rewards_service_->SetPublisherMinVisits(payments->min_visits);

  // Set the excluded sites
  for (const auto& publisher_key : ledger.excluded_publishers) {
    rewards_service_->ExcludePublisher(publisher_key);
  }

  // Set the recurring tips (formerly known as pinned sites)
  int sum_of_monthly_tips = 0;
  pinned_item_count_ = 0;
  for (const auto& item : ledger.pinned_publishers) {
    const auto& publisher_key = item.first;
    const auto& pin_percentage = item.second;
    // NOTE: this will truncate (ex: 0.90 would be 0, not 1)
    const int amount_in_bat = (int)((pin_percentage / 100.0) *
      ledger.settings.payments.contribution_amount);
    if (amount_in_bat > 0) {
      pinned_item_count_++;
      sum_of_monthly_tips += amount_in_bat;
      rewards_service_->OnDonate(publisher_key, amount_in_bat, true);
    }
  }

  // Adjust monthly contribution budget
  // Some may have been allocated for recurring tips
  bool auto_contribute_enabled = payments->enabled;
  const int minimum_monthly_contribution = 15;
  new_contribution_amount_ = payments->contribution_amount;
  if (sum_of_monthly_tips > 0) {
    new_contribution_amount_ -= sum_of_monthly_tips;
    // If left over budget is too low, turn off auto-contribute
    if (new_contribution_amount_ < minimum_monthly_contribution) {
      LOG(INFO) << "Setting auto-contribute to false.\n"
        << "Recurring contributions take up " << sum_of_monthly_tips << " of the monthly "
        << payments->contribution_amount << " budget.\nThis leaves " << new_contribution_amount_
        << " which is less than the minimum monthly auto-contribute amount ("
        << minimum_monthly_contribution << ").";
      auto_contribute_enabled = false;
      new_contribution_amount_ = minimum_monthly_contribution;
    }
  }
  rewards_service_->SetContributionAmount(new_contribution_amount_);
  rewards_service_->SetAutoContribute(auto_contribute_enabled);

  LOG(INFO) << "Starting wallet recovery...";
  rewards_service_->RecoverWallet(ledger.passphrase);
}