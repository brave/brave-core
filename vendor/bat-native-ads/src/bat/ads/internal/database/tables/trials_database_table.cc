/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/trials_database_table.h"

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {
namespace table {

namespace {
const char kTableName[] = "trials";
}  // namespace

Trials::Trials(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Trials::~Trials() = default;

void Trials::InsertOrUpdate(
    DBTransaction* transaction,
    const TrialsList& trials) {
  DCHECK(transaction);

  if (trials.empty()) {
    return;
  }

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(),
      trials);

  transaction->commands.push_back(std::move(command));
}

std::string Trials::get_table_name() const {
  return kTableName;
}

void Trials::Migrate(
    DBTransaction* transaction,
    const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 1: {
      MigrateToV1(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

int Trials::BindParameters(
    DBCommand* command,
    const TrialsList& trials) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad_notification : trials) {
    BindString(command, index++, creative_ad_notification.creative_instance_id);
    BindString(command, index++,
        base::ToLowerASCII(creative_ad_notification.category));

    count++;
  }

  return count;
}

std::string Trials::BuildInsertOrUpdateQuery(
    DBCommand* command,
    const TrialsList& trials) {
  const int count = BindParameters(command, trials);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
          "(creative_instance_id, "
          "category) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(2, count).c_str());
}

void Trials::CreateTableV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
          "(creative_instance_id TEXT NOT NULL, "
          "category TEXT NOT NULL, "
          "UNIQUE(creative_instance_id, category) ON CONFLICT REPLACE, "
          "CONSTRAINT fk_creative_instance_id "
              "FOREIGN KEY (creative_instance_id) "
              "REFERENCES trials (creative_instance_id) "
              "ON DELETE CASCADE)",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void Trials::CreateIndexV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  CreateIndex(transaction, get_table_name(), "category");
}

void Trials::MigrateToV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  Drop(transaction, get_table_name());

  CreateTableV1(transaction);
  CreateIndexV1(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
