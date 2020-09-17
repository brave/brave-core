/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/dayparts_database_table.h"

#include <iostream>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/daypart_frequency_cap.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {
namespace table {

namespace {
const char kTableName[] = "dayparts";
}  // namespace

Dayparts::Dayparts(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Dayparts::~Dayparts() = default;

void Dayparts::InsertOrUpdate(
    DBTransaction* transaction,
    const CreativeAdNotificationList& creative_ad_notifications) {
  DCHECK(transaction);

  if (creative_ad_notifications.empty()) {
    return;
  }

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::RUN;
  std::string result = BuildInsertOrUpdateQuery(command.get(),
      creative_ad_notifications);

  if (result.compare("") != 0) {
    command->command = result;
    transaction->commands.push_back(std::move(command));
  }
}

std::string Dayparts::get_table_name() const {
  return kTableName;
}

void Dayparts::Migrate(
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

int Dayparts::BindParameters(
    DBCommand* command,
    const CreativeAdNotificationList& creative_ad_notifications) {
  DCHECK(command);

  int count = 0;
  int index = 0;
  std::vector<std::string> parsed_daypart;

  for (const auto& creative_ad_notification : creative_ad_notifications) {
    for (const auto& daypart : creative_ad_notification.dayparts) {
      if (&daypart == NULL || daypart.compare("") == 0) {
        std::cout << "**** albert SKIPPED inserting into database" << std::endl;
        continue;
      }
      std::cout << "albert inserting into database: " << daypart << std::endl;
      parsed_daypart = DaypartFrequencyCap::ParseDaypart(daypart);
      /*
      BindString(command, index++,
          creative_ad_notification.creative_instance_id);
      BindString(command, index++, parsed_daypart[0]);
      BindInt(command, index++, std::stoi(parsed_daypart[1]));
      BindInt(command, index++, std::stoi(parsed_daypart[2]));
      */
      BindString(command, index++,
          creative_ad_notification.creative_instance_id);
      BindString(command, index++, daypart);

      count++;
    }
  }

  return count;
}

std::string Dayparts::BuildInsertOrUpdateQuery(
    DBCommand* command,
    const CreativeAdNotificationList& creative_ad_notifications) {
  const int count = BindParameters(command, creative_ad_notifications);

  // Since there's a LEFT JOIN, the daypart code allows the field to be null
  if (count == 0) {
    return "";
  }

  /*
  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
          "(creative_instance_id, "
          "days_of_week, "
          "start_minute, "
          "end_minute) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(4, count).c_str());
      */
  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
          "(creative_instance_id, "
          "daypart) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(2, count).c_str());
}

void Dayparts::CreateTableV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  /*
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
          "(creative_instance_id TEXT NOT NULL, "
          "days_of_week TEXT NOT NULL, "
          "start_minute INT NOT NULL, "
          "end_minute INT NOT NULL, "
          "CONSTRAINT fk_creative_instance_id "
              "FOREIGN KEY (creative_instance_id) "
              "REFERENCES creative_ad_notifications (creative_instance_id) "
              "ON DELETE CASCADE)",
      get_table_name().c_str());
      */
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
          "(creative_instance_id TEXT NOT NULL, "
          "daypart TEXT NOT NULL, "
          "CONSTRAINT fk_creative_instance_id "
              "FOREIGN KEY (creative_instance_id) "
              "REFERENCES creative_ad_notifications (creative_instance_id) "
              "ON DELETE CASCADE)",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void Dayparts::CreateIndexV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  CreateIndex(transaction, get_table_name(), "daypart");
}

void Dayparts::MigrateToV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  Drop(transaction, get_table_name());

  CreateTableV1(transaction);
//  CreateIndexV1(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
