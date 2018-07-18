/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_H_
#define BAT_LEDGER_LEDGER_H_

#include <string>

#include "bat/ledger/export.h"
#include "bat/ledger/ledger_client.h"

namespace ledger {

class LEDGER_EXPORT Ledger {
 public:
  Ledger() = default;
  virtual ~Ledger() = default;

  // Not copyable, not assignable
  Ledger(const Ledger&) = delete;
  Ledger& operator=(const Ledger&) = delete;

  static Ledger* CreateInstance(LedgerClient* client);

  virtual void CreateWallet() = 0;
  virtual void Reconcile() = 0;
  virtual void SaveVisit(const std::string& publisher,
                         uint64_t duration,
                         bool ignoreMinTime) = 0;
  virtual void OnMediaRequest(const std::string& url,
                              const std::string& urlQuery,
                              const std::string& type) = 0;
  virtual void SetPublisherInclude(const std::string& publisher,
                                   bool include) = 0;
  virtual void SetPublisherDeleted(const std::string& publisher,
                                   bool deleted) = 0;
  virtual void SetPublisherPinPercentage(const std::string& publisher,
                                         bool pinPercentage) = 0;
  virtual void SetPublisherMinVisitTime(uint64_t duration_in_milliseconds) = 0;
  virtual void SetPublisherMinVisits(unsigned int visits) = 0;
  virtual void SetPublisherAllowNonVerified(bool allow) = 0;
  virtual void SetContributionAmount(double amount) = 0;
  virtual const std::string& GetBATAddress() const = 0;
  virtual const std::string& GetBTCAddress() const = 0;
  virtual const std::string& GetETHAddress() const = 0;
  virtual const std::string& GetLTCAddress() const = 0;
  virtual uint64_t GetPublisherMinVisitTime() const = 0; // In milliseconds
  virtual unsigned int GetPublisherMinVisits() const = 0;
  virtual bool GetPublisherAllowNonVerified() const = 0;
  virtual double GetContributionAmount() const = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_H_
