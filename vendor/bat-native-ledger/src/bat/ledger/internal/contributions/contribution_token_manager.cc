/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/contribution_token_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/sql_store.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

namespace {

using Hold = ContributionTokenManager::Hold;

mojom::CredsBatchType CredsTypeFromTokenType(ContributionTokenType type) {
  switch (type) {
    case ContributionTokenType::kVG:
      return mojom::CredsBatchType::PROMOTION;
    case ContributionTokenType::kSKU:
      return mojom::CredsBatchType::SKU;
  }
}

class GetAvailableJob : public BATLedgerJob<std::vector<ContributionToken>> {
 public:
  void Start(ContributionTokenType type) {
    static const char kSQL[] = R"sql(
      SELECT ut.token_id, ut.value, ut.token_value, ut.public_key
      FROM unblinded_tokens AS ut
      LEFT JOIN creds_batch AS cb ON cb.creds_id = ut.creds_id
      WHERE ut.reserved_at = 0 AND ut.redeemed_at = 0
        AND (ut.expires_at > ? OR ut.expires_at = 0)
        AND (cb.trigger_type IS NULL OR cb.trigger_type = ?)
    )sql";

    int64_t trigger_type = static_cast<int64_t>(CredsTypeFromTokenType(type));

    context()
        .Get<SQLStore>()
        .Query(kSQL, base::Time::Now().ToDoubleT(), trigger_type)
        .Then(ContinueWith(this, &GetAvailableJob::OnTokensRead));
  }

 private:
  void OnTokensRead(SQLReader reader) {
    auto& manager = context().Get<ContributionTokenManager>();
    std::vector<ContributionToken> tokens;

    while (reader.Step()) {
      ContributionToken token;
      token.id = reader.ColumnInt64(0);
      token.value = reader.ColumnDouble(1);
      token.unblinded_token = reader.ColumnString(2);
      token.public_key = reader.ColumnString(3);

      if (!manager.IsTokenReserved(token.id)) {
        tokens.push_back(std::move(token));
      }
    }

    Complete(std::move(tokens));
  }
};

class ReserveJob : public BATLedgerJob<Hold> {
 public:
  void Start(ContributionTokenType type, double amount) {
    if (amount <= 0) {
      context().LogError(FROM_HERE) << "Invalid token amount";
      return Complete(Hold());
    }

    amount_ = amount;

    context().StartJob<GetAvailableJob>(type).Then(
        ContinueWith(this, &ReserveJob::OnAvailableTokensRead));
  }

 private:
  void OnAvailableTokensRead(std::vector<ContributionToken> tokens) {
    double token_sum = 0;
    for (auto iter = tokens.begin(); iter != tokens.end(); ++iter) {
      if (token_sum >= amount_) {
        tokens.erase(iter, tokens.end());
        break;
      }
      token_sum += iter->value;
    }

    if (token_sum > amount_) {
      context().LogInfo(FROM_HERE)
          << "Token value is greater than requested amount";
    }

    Complete(Hold(context().GetWeakPtr(), std::move(tokens)));
  }

  double amount_;
};

class ReserveByIdJob : public BATLedgerJob<Hold> {
 public:
  void Start(const std::vector<int64_t>& token_ids) {
    std::string sql = R"sql(
      SELECT token_id, value, token_value, public_key
      FROM unblinded_tokens
      WHERE redeemed_at = 0 AND token_id IN
    )sql" + SQLStore::PlaceholderList(token_ids.size());

    context()
        .Get<SQLStore>()
        .Query(sql, token_ids)
        .Then(ContinueWith(this, &ReserveByIdJob::OnTokensRead));
  }

 private:
  void OnTokensRead(SQLReader reader) {
    std::vector<ContributionToken> tokens;

    while (reader.Step()) {
      tokens.push_back({.id = reader.ColumnInt64(0),
                        .value = reader.ColumnDouble(1),
                        .unblinded_token = reader.ColumnString(2),
                        .public_key = reader.ColumnString(3)});
    }

    Complete(Hold(context().GetWeakPtr(), std::move(tokens)));
  }
};

class MarkRedeemedJob : public BATLedgerJob<bool> {
 public:
  void Start(const std::vector<ContributionToken>& tokens,
             const std::string& contribution_id) {
    std::vector<int64_t> token_ids;
    for (auto& token : tokens) {
      token_ids.push_back(token.id);
    }

    std::string sql = R"sql(
      UPDATE unblinded_tokens
      SET redeemed_at = ?, redeem_id = ?
      WHERE token_id IN
    )sql" + SQLStore::PlaceholderList(token_ids.size());

    context()
        .Get<SQLStore>()
        .Run(sql, base::Time::Now().ToDoubleT(), contribution_id, token_ids)
        .Then(ContinueWith(this, &MarkRedeemedJob::OnExecuted));
  }

 private:
  void OnExecuted(SQLReader reader) {
    if (!reader.Succeeded()) {
      context().LogError(FROM_HERE)
          << "Error marking contribution tokens redeemed";
      return Complete(false);
    }

    Complete(true);
  }
};

class InsertTokensJob : public BATLedgerJob<bool> {
 public:
  void Start(const std::vector<ContributionToken>& tokens,
             ContributionTokenType token_type) {
    static const char kCredsSQL[] = R"sql(
      INSERT INTO creds_batch (creds_id, trigger_id, trigger_type, creds,
        blinded_creds, status)
      VALUES (?, ?, ?, ?, ?, ?)
    )sql";

    static const char kTokensSQL[] = R"sql(
      INSERT INTO unblinded_tokens (token_value, public_key, value, creds_id)
      VALUES (?, ?, ?, ?)
    )sql";

    // Although required at the database schema level, much of the data is not
    // required by the application when the tables are not being used to store
    // job status. In the future the schema for these tables should be updated.
    std::string creds_id = base::GUID::GenerateRandomV4().AsLowercaseString();
    std::string trigger_id = base::GUID::GenerateRandomV4().AsLowercaseString();
    int64_t trigger_type =
        static_cast<int64_t>(CredsTypeFromTokenType(token_type));
    int64_t creds_status =
        static_cast<int64_t>(mojom::CredsBatchStatus::FINISHED);

    SQLStore::CommandList commands;
    commands.push_back(SQLStore::CreateCommand(kCredsSQL, creds_id, trigger_id,
                                               trigger_type, "[]", "[]",
                                               creds_status));

    for (auto& token : tokens) {
      commands.push_back(
          SQLStore::CreateCommand(kTokensSQL, token.unblinded_token,
                                  token.public_key, token.value, creds_id));
    }

    context()
        .Get<SQLStore>()
        .RunTransaction(std::move(commands))
        .Then(ContinueWith(this, &InsertTokensJob::OnInserted));
  }

 private:
  void OnInserted(SQLReader reader) { Complete(reader.Succeeded()); }
};

}  // namespace

Hold::Hold() = default;

Hold::Hold(base::WeakPtr<BATLedgerContext> context,
           std::vector<ContributionToken>&& tokens)
    : context_(context), tokens_(std::move(tokens)) {
  DCHECK(context_);
  context_->Get<ContributionTokenManager>().AddReservedTokens(tokens_);
}

Hold::Hold(base::WeakPtr<Hold> parent, size_t token_count)
    : parent_(parent), context_(parent->context_) {
  DCHECK(parent_);
  while (token_count > 0 && !parent_->tokens_.empty()) {
    tokens_.push_back(std::move(parent_->tokens_.back()));
    parent_->tokens_.pop_back();
    token_count -= 1;
  }
}

Hold::~Hold() {
  Release();
}

Hold::Hold(Hold&& other) {
  *this = std::move(other);
}

Hold& Hold::operator=(Hold&& other) {
  parent_ = other.parent_;
  context_ = other.context_;
  tokens_ = std::move(other.tokens_);
  return *this;
}

void Hold::Release() {
  if (parent_) {
    // If this hold is linked to a parent hold, then move the tokens back to the
    // parent.
    for (auto& token : tokens_) {
      parent_->tokens_.push_back(std::move(token));
    }
  } else if (context_) {
    // Otherwise, release the tokens from the reserved pool.
    context_->Get<ContributionTokenManager>().RemoveReservedTokens(tokens_);
  }

  tokens_.clear();
}

Hold Hold::Split(size_t count) {
  return Hold(weak_factory_.GetWeakPtr(), count);
}

void Hold::OnTokensRedeemed(const std::string& contribution_id) {
  // Unlink the hold from its parent so that when it is released the tokens will
  // not be transferred back to a parent hold.
  parent_ = nullptr;

  if (context_) {
    context_->StartJob<MarkRedeemedJob>(tokens_, contribution_id);
  }
}

double Hold::GetTotalValue() {
  double value = 0;
  for (auto& token : tokens_) {
    value += token.value;
  }
  return value;
}

ContributionTokenManager::ContributionTokenManager() = default;

ContributionTokenManager::~ContributionTokenManager() = default;

Future<Hold> ContributionTokenManager::ReserveTokens(ContributionTokenType type,
                                                     double amount) {
  return context().StartJob<ReserveJob>(type, amount);
}

Future<Hold> ContributionTokenManager::ReserveTokens(
    const std::vector<int64_t>& token_ids) {
  return context().StartJob<ReserveByIdJob>(token_ids);
}

Future<double> ContributionTokenManager::GetAvailableTokenBalance(
    ContributionTokenType type) {
  return context().StartJob<GetAvailableJob>(type).Then(
      base::BindOnce([](std::vector<ContributionToken> tokens) {
        double token_sum = 0;
        for (auto& token : tokens) {
          token_sum += token.value;
        }
        return token_sum;
      }));
}

Future<bool> ContributionTokenManager::InsertTokens(
    const std::vector<ContributionToken>& tokens,
    ContributionTokenType token_type) {
  return context().StartJob<InsertTokensJob>(tokens, token_type);
}

bool ContributionTokenManager::IsTokenReserved(int64_t token_id) {
  return reserved_token_ids_.find(token_id) != reserved_token_ids_.end();
}

void ContributionTokenManager::AddReservedTokens(
    const std::vector<ContributionToken>& tokens) {
  for (auto& token : tokens) {
    reserved_token_ids_.insert(token.id);
  }
  context().Get<BATLedgerObserver>().OnAvailableBalanceUpdated();
}

void ContributionTokenManager::RemoveReservedTokens(
    const std::vector<ContributionToken>& tokens) {
  for (auto& token : tokens) {
    reserved_token_ids_.erase(token.id);
  }
  context().Get<BATLedgerObserver>().OnAvailableBalanceUpdated();
}

}  // namespace ledger
