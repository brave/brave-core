/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_WRITER_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_WRITER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/importer/profile_writer.h"
#include "net/cookies/canonical_cookie.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/common/importer/brave_ledger.h"

struct BraveStats;
struct BraveReferral;
class BraveInProcessImporterBridge;
struct ImportedWindowState;

class BraveProfileWriter : public ProfileWriter,
                           public brave_rewards::RewardsServiceObserver,
                           public base::SupportsWeakPtr<BraveProfileWriter>{
 public:
  explicit BraveProfileWriter(Profile* profile);

  virtual void AddCookies(const std::vector<net::CanonicalCookie>& cookies);
  virtual void UpdateStats(const BraveStats& stats);
  virtual void UpdateLedger(const BraveLedger& ledger);
  virtual void UpdateReferral(const BraveReferral& referral);
  virtual void UpdateWindows(const ImportedWindowState& windowState);
  virtual void UpdateSettings(const SessionStoreSettings& settings);

  void SetBridge(BraveInProcessImporterBridge* bridge);

  void OnIsWalletCreated(bool created);

  // brave_rewards::RewardsServiceObserver:
  void OnWalletInitialized(brave_rewards::RewardsService* rewards_service,
                           uint32_t result) override;
  void OnRecoverWallet(brave_rewards::RewardsService* rewards_service,
                       unsigned int result,
                       double balance,
                       std::vector<brave_rewards::Grant> grants) override;
  void OnWalletProperties(
      brave_rewards::RewardsService* rewards_service,
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties> properties) override;

 protected:
  friend class base::RefCountedThreadSafe<BraveProfileWriter>;
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;
  void CancelWalletImport(std::string msg);
  void SetWalletProperties(brave_rewards::RewardsService* rewards_service);
  void BackupWallet();
  void OnWalletBackupComplete(bool result);
  ~BraveProfileWriter() override;

 private:
  brave_rewards::RewardsService* rewards_service_;
  BraveInProcessImporterBridge* bridge_ptr_;
  double new_contribution_amount_;
  unsigned int pinned_item_count_;
  BraveLedger ledger_;
  // Only used when wallet exists and first action is guaranteed
  // to be FetchWalletProperties(). See notes in brave_profile_writer.cc
  bool consider_for_backup_;
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_WRITER_H_
