/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_H_
#define BAT_LEDGER_LEDGER_H_

#include <memory>
#include <string>

#include "bat/ledger/export.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/publisher_info.h"

namespace ledger {

LEDGER_EXPORT struct VisitData {
  VisitData(const std::string& _tld,
            const std::string& _domain,
            const std::string& _path,
            uint64_t _duration);
  const std::string tld;
  const std::string domain;
  const std::string path;
  uint64_t duration;
};

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
  virtual void OnVisit(const VisitData& visit_data) = 0;
  virtual void OnMediaRequest(const std::string& url,
                              const std::string& urlQuery,
                              const std::string& type) = 0;

  virtual void SetPublisherInfo(std::unique_ptr<PublisherInfo> publisher_info,
                                PublisherInfoCallback callback) = 0;
  virtual void GetPublisherInfo(const PublisherInfo::id_type& publisher_id,
                                PublisherInfoCallback callback) = 0;
  virtual void GetPublisherInfoList(uint32_t start, uint32_t limit,
                                    PublisherInfoFilter filter,
                                    GetPublisherInfoListCallback callback) = 0;

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
