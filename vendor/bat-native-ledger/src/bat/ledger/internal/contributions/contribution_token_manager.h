/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_TOKEN_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_TOKEN_MANAGER_H_

#include <set>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "bat/ledger/internal/contributions/contribution_data.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/bat_ledger_observer.h"
#include "bat/ledger/internal/core/future.h"

namespace ledger {

class ContributionTokenManager : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "contribution-token-manager";

  class Hold {
   public:
    Hold();

    Hold(base::WeakPtr<BATLedgerContext> context,
         std::vector<ContributionToken>&& tokens);

    ~Hold();

    Hold(const Hold& other) = delete;
    Hold& operator=(const Hold& other) = delete;

    Hold(Hold&& other);
    Hold& operator=(Hold&& other);

    void Release();

    Hold Split(size_t count);

    void OnTokensRedeemed(const std::string& contribution_id);

    double GetTotalValue();

    const std::vector<ContributionToken>& tokens() const { return tokens_; }

   private:
    Hold(base::WeakPtr<Hold> parent, size_t token_count);

    base::WeakPtr<Hold> parent_;
    base::WeakPtr<BATLedgerContext> context_;
    std::vector<ContributionToken> tokens_;
    base::WeakPtrFactory<Hold> weak_factory_{this};
  };

  ContributionTokenManager();
  ~ContributionTokenManager() override;

  Future<Hold> ReserveTokens(ContributionTokenType type, double amount);

  Future<Hold> ReserveTokens(const std::vector<int64_t>& token_ids);

  Future<double> GetAvailableTokenBalance(ContributionTokenType type);

  Future<bool> InsertTokens(const std::vector<ContributionToken>& tokens,
                            ContributionTokenType token_type);

  bool IsTokenReserved(int64_t token_id);

 private:
  void AddReservedTokens(const std::vector<ContributionToken>& tokens);
  void RemoveReservedTokens(const std::vector<ContributionToken>& tokens);

  std::set<int64_t> reserved_token_ids_;
};

using ContributionTokenHold = ContributionTokenManager::Hold;

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTIONS_CONTRIBUTION_TOKEN_MANAGER_H_
