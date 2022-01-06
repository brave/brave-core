/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/contribution_store.h"

#include <string>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/sql_store.h"
#include "bat/ledger/internal/external_wallet/external_wallet_manager.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {

namespace {

using base::BindOnce;

class SaveCompletedJob : public BATLedgerJob<bool> {
 public:
  SaveCompletedJob() {
    report_id_ = GetCurrentBalanceReportID();
    contribution_id_ = base::GUID::GenerateRandomV4().AsLowercaseString();
  }

  void Start(const Contribution& contribution) {
    type_ = contribution.type;
    source_ = contribution.source;
    total_amount_ = contribution.amount;

    Save({{contribution.publisher_id, contribution.amount}});
  }

  void Start(const base::flat_map<std::string, double>& publisher_amounts,
             ContributionSource source) {
    type_ = ContributionType::kAutoContribute;
    source_ = source;
    total_amount_ = 0;

    for (auto& [_, amount] : publisher_amounts) {
      total_amount_ += amount;
    }

    Save(publisher_amounts);
  }

 private:
  void Save(const base::flat_map<std::string, double>& publisher_amounts) {
    SQLStore::CommandList commands;

    commands.push_back(CreateReportInsertCommand());
    commands.push_back(CreateReportUpdateCommand());
    commands.push_back(CreateContributionInfoInsertCommand());

    for (auto& [publisher_id, amount] : publisher_amounts) {
      commands.push_back(
          CreateContributionPublisherInsertCommand(publisher_id, amount));
    }

    context()
        .Get<SQLStore>()
        .RunTransaction(std::move(commands))
        .Then(ContinueWith(this, &SaveCompletedJob::OnTransactionCompleted));
  }

  void OnTransactionCompleted(SQLReader reader) {
    Complete(reader.Succeeded());
  }

  SQLStore::Command CreateReportInsertCommand() {
    static const char kSQL[] = R"sql(
      INSERT OR IGNORE INTO balance_report_info (balance_report_id)
      VALUES (?)
    )sql";

    return SQLStore::CreateCommand(kSQL, report_id_);
  }

  SQLStore::Command CreateReportUpdateCommand() {
    static const char kSQL[] = R"sql(
      UPDATE balance_report_info
      SET auto_contribute = auto_contribute + ?,
          tip_recurring = tip_recurring + ?,
          tip = tip + ?
      WHERE balance_report_id = ?
    )sql";

    double auto_contribute_value = 0;
    double recurring_tip_value = 0;
    double tip_value = 0;

    switch (type_) {
      case ContributionType::kOneTime:
        tip_value = total_amount_;
        break;
      case ContributionType::kRecurring:
        recurring_tip_value = total_amount_;
        break;
      case ContributionType::kAutoContribute:
        auto_contribute_value = total_amount_;
        break;
    }

    return SQLStore::CreateCommand(kSQL, auto_contribute_value,
                                   recurring_tip_value, tip_value, report_id_);
  }

  SQLStore::Command CreateContributionInfoInsertCommand() {
    static const char kSQL[] = R"sql(
      INSERT INTO contribution_info (contribution_id, amount, type, step,
        retry_count, created_at, processor)
      VALUES (?, ?, ?, ?, ?, ?, ?)
    )sql";

    return SQLStore::CreateCommand(
        kSQL, contribution_id_, total_amount_,
        static_cast<int64_t>(GetRewardsType(type_)),
        static_cast<int64_t>(mojom::ContributionStep::STEP_COMPLETED), 0,
        base::Time::Now().ToDoubleT(), static_cast<int64_t>(GetProcessor()));
  }

  SQLStore::Command CreateContributionPublisherInsertCommand(
      const std::string& publisher_id,
      double amount) {
    DCHECK(!publisher_id.empty());

    static const char kSQL[] = R"sql(
      INSERT INTO contribution_info_publishers (contribution_id, publisher_key,
        total_amount, contributed_amount)
      VALUES (?, ?, ?, ?)
    )sql";

    return SQLStore::CreateCommand(kSQL, contribution_id_, publisher_id, amount,
                                   amount);
  }

  static mojom::RewardsType GetRewardsType(ContributionType type) {
    switch (type) {
      case ContributionType::kOneTime:
        return mojom::RewardsType::ONE_TIME_TIP;
      case ContributionType::kRecurring:
        return mojom::RewardsType::RECURRING_TIP;
      case ContributionType::kAutoContribute:
        return mojom::RewardsType::AUTO_CONTRIBUTE;
    }
  }

  mojom::ContributionProcessor GetProcessor() {
    if (source_ != ContributionSource::kExternal) {
      return mojom::ContributionProcessor::BRAVE_TOKENS;
    }

    auto external_wallet =
        context().Get<ExternalWalletManager>().GetExternalWallet();

    if (!external_wallet) {
      return mojom::ContributionProcessor::BRAVE_TOKENS;
    }

    switch (external_wallet->provider) {
      case ExternalWalletProvider::kBitflyer:
        return mojom::ContributionProcessor::BITFLYER;
      case ExternalWalletProvider::kGemini:
        return mojom::ContributionProcessor::GEMINI;
      case ExternalWalletProvider::kUphold:
        return mojom::ContributionProcessor::UPHOLD;
    }
  }

  static std::string GetCurrentBalanceReportID() {
    base::Time::Exploded exploded_now;
    base::Time::Now().UTCExplode(&exploded_now);
    DCHECK(exploded_now.HasValidValues());
    return base::NumberToString(exploded_now.year) + "_" +
           base::NumberToString(exploded_now.month);
  }

  std::string report_id_;
  std::string contribution_id_;
  ContributionType type_;
  ContributionSource source_;
  double total_amount_ = 0;
};

}  // namespace

Future<bool> ContributionStore::SaveCompletedContribution(
    const Contribution& contribution) {
  return context().StartJob<SaveCompletedJob>(contribution);
}

Future<bool> ContributionStore::SaveCompletedAutoContribute(
    const base::flat_map<std::string, double>& publisher_amounts,
    ContributionSource source) {
  return context().StartJob<SaveCompletedJob>(publisher_amounts, source);
}

Future<bool> ContributionStore::SavePendingContribution(
    const std::string& publisher_id,
    double amount) {
  return SavePendingContribution(publisher_id, amount, base::Time::Now());
}

Future<bool> ContributionStore::SavePendingContribution(
    const std::string& publisher_id,
    double amount,
    base::Time created_at) {
  static const char kSQL[] = R"sql(
    INSERT INTO pending_contribution (publisher_id, amount, added_date,
      viewing_id, type)
    VALUES (?, ?, ?, ?, ?)
  )sql";

  int64_t added_date = static_cast<int64_t>(base::Time::Now().ToDoubleT());
  int64_t contribution_type =
      static_cast<int64_t>(mojom::RewardsType::ONE_TIME_TIP);

  return context()
      .Get<SQLStore>()
      .Run(kSQL, publisher_id, amount, added_date, "", contribution_type)
      .Then(BindOnce([](SQLReader reader) { return reader.Succeeded(); }));
}

Future<std::vector<PendingContribution>>
ContributionStore::GetPendingContributions() {
  static const char kSQL[] = R"sql(
    SELECT pending_contribution_id, publisher_id, amount, added_date
    FROM pending_contribution
  )sql";

  return context().Get<SQLStore>().Query(kSQL).Then(
      BindOnce([](SQLReader reader) {
        std::vector<PendingContribution> contributions;
        while (reader.Step()) {
          contributions.push_back(
              {.id = reader.ColumnInt64(0),
               .publisher_id = reader.ColumnString(1),
               .amount = reader.ColumnDouble(2),
               .created_at = base::Time::FromDoubleT(reader.ColumnInt64(3))});
        }
        return contributions;
      }));
}

Future<bool> ContributionStore::DeletePendingContribution(int64_t id) {
  static const char kSQL[] = R"sql(
    DELETE FROM pending_contribution WHERE pending_contribution_id = ?
  )sql";

  return context().Get<SQLStore>().Run(kSQL, id).Then(
      BindOnce([](SQLReader reader) { return reader.Succeeded(); }));
}

Future<bool> ContributionStore::ClearPendingContributions() {
  static const char kSQL[] = R"sql(DELETE FROM pending_contribution)sql";

  return context().Get<SQLStore>().Run(kSQL).Then(
      BindOnce([](SQLReader reader) { return reader.Succeeded(); }));
}

Future<std::vector<PublisherActivity>>
ContributionStore::GetPublisherActivity() {
  static const char kSQL[] = R"sql(
    SELECT activity_info.publisher_id, activity_info.visits,
      activity_info.duration
    FROM activity_info
    LEFT JOIN publisher_info
      ON publisher_info.publisher_id = activity_info.publisher_id
    WHERE activity_info.duration > 0 AND activity_info.reconcile_stamp = ?
      AND (publisher_info.excluded IS NULL OR publisher_info.excluded = 0)
  )sql";

  int64_t reconcile_stamp =
      context().GetLedgerImpl()->state()->GetReconcileStamp();

  return context()
      .Get<SQLStore>()
      .Query(kSQL, reconcile_stamp)
      .Then(BindOnce([](SQLReader reader) {
        std::vector<PublisherActivity> publishers;
        while (reader.Step()) {
          publishers.push_back(
              {.publisher_id = reader.ColumnString(0),
               .visits = reader.ColumnInt64(1),
               .duration = base::Seconds(reader.ColumnDouble(2))});
        }
        return publishers;
      }));
}

Future<bool> ContributionStore::ResetPublisherActivity() {
  // Resetting publisher activity data does not currently require a database
  // operation since records in `activity_info` are never deleted. Instead,
  // the `activity_info` table are keyed on the `reconcile_stamp` (the next
  // scheduled contribution time).
  return MakeReadyFuture(true);
}

Future<std::vector<RecurringContribution>>
ContributionStore::GetRecurringContributions() {
  static const char kSQL[] = R"sql(
    SELECT publisher_id, amount
    FROM recurring_donation
    WHERE amount > 0
  )sql";

  return context().Get<SQLStore>().Query(kSQL).Then(
      BindOnce([](SQLReader reader) {
        std::vector<RecurringContribution> contributions;
        while (reader.Step()) {
          contributions.push_back({.publisher_id = reader.ColumnString(0),
                                   .amount = reader.ColumnDouble(1)});
        }
        return contributions;
      }));
}

Future<base::Time> ContributionStore::GetLastScheduledContributionTime() {
  // Currently, the "next scheduled contribution time" is stored in Preferences.
  // In the future we will move this information into the database and store the
  // last scheduled contribution time instead.
  base::Time next = base::Time::FromDoubleT(
      context().GetLedgerImpl()->state()->GetReconcileStamp());

  return MakeReadyFuture(next - kScheduledContributionInterval);
}

Future<bool> ContributionStore::UpdateLastScheduledContributionTime() {
  context().GetLedgerImpl()->state()->ResetReconcileStamp();
  return MakeReadyFuture(true);
}

}  // namespace ledger
