/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events_database_table.h"

#include <functional>
#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/database/database_bind_util.h"
#include "bat/ads/internal/base/database/database_column_util.h"
#include "bat/ads/internal/base/database/database_table_util.h"
#include "bat/ads/internal/base/database/database_transaction_util.h"
#include "bat/ads/internal/base/logging_util.h"

namespace ads {
namespace database {
namespace table {

namespace {

constexpr char kTableName[] = "text_embedding_html_events";

int BindParameters(mojom::DBCommand* command, const TextEmbeddingHTMLEventList& text_embedding_html_events) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& text_embedding_html_event : text_embedding_html_events) {
    BindDouble(command, index++, text_embedding_html_event.timestamp.ToDoubleT());
    BindString(command, index++, text_embedding_html_event.version);
    BindString(command, index++, text_embedding_html_event.locale);
    BindString(command, index++, text_embedding_html_event.embedding);

    count++;
  }

  return count;
}

TextEmbeddingEventInfo GetFromRecord(mojom::DBRecord* record) {
  DCHECK(record);

  TextEmbeddingEventInfo text_embedding_html_event;

  text_embedding_html_event.timestamp = base::Time::FromDoubleT(ColumnDouble(record, 0));
  text_embedding_html_event.version = ColumnString(record, 1);
  text_embedding_html_event.locale = ColumnString(record, 2);
  text_embedding_html_event.embedding = ColumnString(record, 3);

  return text_embedding_html_event;
}

}  // namespace

TextEmbeddingHTMLEvents::TextEmbeddingHTMLEvents() = default;

TextEmbeddingHTMLEvents::~TextEmbeddingHTMLEvents() = default;

void TextEmbeddingHTMLEvents::LogEvent(const TextEmbeddingEventInfo& text_embedding_html_event, ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  InsertOrUpdate(transaction.get(), {text_embedding_html_event});

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

// void AdEvents::GetIf(const std::string& condition,
//                      GetAdEventsCallback callback) {
//   const std::string& query = base::StringPrintf(
//       "SELECT "
//       "ae.uuid, "
//       "ae.type, "
//       "ae.confirmation_type, "
//       "ae.campaign_id, "
//       "ae.creative_set_id, "
//       "ae.creative_instance_id, "
//       "ae.advertiser_id, "
//       "ae.timestamp "
//       "FROM %s AS ae "
//       "WHERE %s "
//       "ORDER BY timestamp DESC ",
//       GetTableName().c_str(), condition.c_str());

//   RunTransaction(query, callback);
// }

void TextEmbeddingHTMLEvents::GetAll(GetTextEmbeddingHTMLEventsCallback callback) {
  const std::string& query = base::StringPrintf(
      "SELECT "
      "tehe.timestamp, "
      "tehe.version, "
      "tehe.locale, "
      "tehe.embedding "
      "FROM %s AS tehe "
      "ORDER BY timestamp DESC",
      GetTableName().c_str());

  RunTransaction(query, callback);
}

// void AdEvents::GetForType(const mojom::AdType ad_type,
//                           GetAdEventsCallback callback) {
//   const std::string& ad_type_as_string = AdType(ad_type).ToString();

//   const std::string& query = base::StringPrintf(
//       "SELECT "
//       "ae.uuid, "
//       "ae.type, "
//       "ae.confirmation_type, "
//       "ae.campaign_id, "
//       "ae.creative_set_id, "
//       "ae.creative_instance_id, "
//       "ae.advertiser_id, "
//       "ae.timestamp "
//       "FROM %s AS ae "
//       "WHERE type = '%s' "
//       "ORDER BY timestamp DESC",
//       GetTableName().c_str(), ad_type_as_string.c_str());

//   RunTransaction(query, callback);
// }

// void AdEvents::PurgeExpired(ResultCallback callback) {
//   const std::string& query = base::StringPrintf(
//       "DELETE FROM %s "
//       "WHERE creative_set_id NOT IN "
//       "(SELECT creative_set_id from creative_ads) "
//       "AND creative_set_id NOT IN "
//       "(SELECT creative_set_id from creative_ad_conversions) "
//       "AND DATETIME('now') >= DATETIME(timestamp, 'unixepoch', '+3 month')",
//       GetTableName().c_str());

//   mojom::DBCommandPtr command = mojom::DBCommand::New();
//   command->type = mojom::DBCommand::Type::EXECUTE;
//   command->command = query;

//   mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
//   transaction->commands.push_back(std::move(command));

//   AdsClientHelper::Get()->RunDBTransaction(
//       std::move(transaction),
//       std::bind(&OnResultCallback, std::placeholders::_1, callback));
// }

// void AdEvents::PurgeOrphaned(const mojom::AdType ad_type,
//                              ResultCallback callback) {
//   const std::string& ad_type_as_string = AdType(ad_type).ToString();

//   const std::string& query = base::StringPrintf(
//       "DELETE FROM %s "
//       "WHERE uuid IN (SELECT uuid from %s GROUP BY uuid having count(*) = 1) "
//       "AND confirmation_type IN (SELECT confirmation_type from %s "
//       "WHERE confirmation_type = 'served') "
//       "AND type = '%s'",
//       GetTableName().c_str(), GetTableName().c_str(), GetTableName().c_str(),
//       ad_type_as_string.c_str());

//   mojom::DBCommandPtr command = mojom::DBCommand::New();
//   command->type = mojom::DBCommand::Type::EXECUTE;
//   command->command = query;

//   mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
//   transaction->commands.push_back(std::move(command));

//   AdsClientHelper::Get()->RunDBTransaction(
//       std::move(transaction),
//       std::bind(&OnResultCallback, std::placeholders::_1, callback));
// }

std::string TextEmbeddingHTMLEvents::GetTableName() const {
  return kTableName;
}

void TextEmbeddingHTMLEvents::Migrate(mojom::DBTransaction* transaction,
                       const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 25: {
      MigrateToV25(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void TextEmbeddingHTMLEvents::RunTransaction(const std::string& query,
                              GetTextEmbeddingHTMLEventsCallback callback) {
  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // timestamp
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // version
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // locale
      mojom::DBCommand::RecordBindingType::STRING_TYPE  // embedding
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&TextEmbeddingHTMLEvents::OnGetTextEmbeddingHTMLEvents, this,
                                        std::placeholders::_1, callback));
}

void TextEmbeddingHTMLEvents::InsertOrUpdate(mojom::DBTransaction* transaction,
                              const TextEmbeddingHTMLEventList& text_embedding_html_events) {
  DCHECK(transaction);

  if (text_embedding_html_events.empty()) {
    return;
  }

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), text_embedding_html_events);

  transaction->commands.push_back(std::move(command));
}

std::string TextEmbeddingHTMLEvents::BuildInsertOrUpdateQuery(mojom::DBCommand* command,
                                               const TextEmbeddingHTMLEventList& text_embedding_html_events) {
  DCHECK(command);

  const int count = BindParameters(command, text_embedding_html_events);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(timestamp, "
      "version, "
      "locale, "
      "embedding) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(4, count).c_str());
}

void TextEmbeddingHTMLEvents::OnGetTextEmbeddingHTMLEvents(mojom::DBCommandResponsePtr response,
                             GetTextEmbeddingHTMLEventsCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get ad events");
    callback(/* success */ false, {});
    return;
  }

  TextEmbeddingHTMLEventList text_embedding_html_events;

  for (const auto& record : response->result->get_records()) {
    const TextEmbeddingEventInfo& text_embedding_html_event = GetFromRecord(record.get());
    text_embedding_html_events.push_back(text_embedding_html_event);
  }

  callback(/* success */ true, text_embedding_html_events);
}

void TextEmbeddingHTMLEvents::MigrateToV25(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string& query = base::StringPrintf(
      "CREATE TABLE text_embedding_html_events "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "timestamp TIMESTAMP NOT NULL, "
      "version TEXT NOT NULL, "
      "locale TEXT NOT NULL, "
      "embedding TEXT NOT NULL)");

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

// void AdEvents::MigrateToV13(mojom::DBTransaction* transaction) {
//   DCHECK(transaction);

//   RenameTable(transaction, "ad_events", "ad_events_temp");

//   const std::string& query = base::StringPrintf(
//       "CREATE TABLE ad_events "
//       "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
//       "uuid TEXT NOT NULL, "
//       "type TEXT, "
//       "confirmation_type TEXT, "
//       "campaign_id TEXT NOT NULL, "
//       "creative_set_id TEXT NOT NULL, "
//       "creative_instance_id TEXT NOT NULL, "
//       "advertiser_id TEXT, "
//       "timestamp TIMESTAMP NOT NULL); "
//       "INSERT INTO ad_events "
//       "(id, "
//       "uuid, "
//       "type, "
//       "confirmation_type, "
//       "campaign_id, "
//       "creative_set_id, "
//       "creative_instance_id, "
//       "timestamp) "
//       "SELECT id, "
//       "uuid, "
//       "type, "
//       "confirmation_type, "
//       "campaign_id, "
//       "creative_set_id, "
//       "creative_instance_id, "
//       "timestamp "
//       "FROM ad_events_temp");

//   mojom::DBCommandPtr command = mojom::DBCommand::New();
//   command->type = mojom::DBCommand::Type::EXECUTE;
//   command->command = query;

//   transaction->commands.push_back(std::move(command));

//   DropTable(transaction, "ad_events_temp");
// }

// void AdEvents::MigrateToV17(mojom::DBTransaction* transaction) {
//   DCHECK(transaction);

//   CreateTableIndex(transaction, "ad_events", "timestamp");
// }

}  // namespace table
}  // namespace database
}  // namespace ads
