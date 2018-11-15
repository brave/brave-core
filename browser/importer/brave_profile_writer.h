/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_WRITER_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_WRITER_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/importer/profile_writer.h"
#include "net/cookies/canonical_cookie.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"

struct BraveStats;
struct BraveLedger;
class BraveInProcessImporterBridge;

class BraveProfileWriter : public ProfileWriter,
                           public brave_rewards::RewardsServiceObserver {
 public:
  explicit BraveProfileWriter(Profile* profile);

  virtual void AddCookies(const std::vector<net::CanonicalCookie>& cookies);
  virtual void UpdateStats(const BraveStats& stats);
  virtual void UpdateLedger(const BraveLedger& ledger);

  void SetBridge(BraveInProcessImporterBridge* bridge);

  // brave_rewards::RewardsServiceObserver:
  void OnRecoverWallet(brave_rewards::RewardsService* rewards_service,
                       unsigned int result,
                       double balance,
                       std::vector<brave_rewards::Grant> grants) override;

 protected:
  friend class base::RefCountedThreadSafe<BraveProfileWriter>;
  ~BraveProfileWriter() override;

 private:
  brave_rewards::RewardsService* rewards_service_;
  BraveInProcessImporterBridge* bridge_ptr_;
  double new_contribution_amount_;
  unsigned int pinned_item_count_;
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_WRITER_H_
