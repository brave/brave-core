/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events_database_table.h"

#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/database/database_bind_util.h"
#include "bat/ads/internal/base/database/database_column_util.h"
#include "bat/ads/internal/base/database/database_table_util.h"
#include "bat/ads/internal/base/database/database_transaction_util.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/features/text_embedding_features.h"

namespace ads {
namespace database {
namespace table {

namespace {

constexpr char kTableName[] = "text_embedding_html_events";

int BindParameters(
    mojom::DBCommandInfo* command,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& text_embedding_html_event : text_embedding_html_events) {
    BindDouble(command, index++,
               text_embedding_html_event.created_at.ToDoubleT());
    BindString(command, index++, text_embedding_html_event.version);
    BindString(command, index++, text_embedding_html_event.locale);
    BindString(command, index++, text_embedding_html_event.hashed_key);
    BindString(command, index++, text_embedding_html_event.embedding);

    count++;
  }

  return count;
}

TextEmbeddingEventInfo GetFromRecord(mojom::DBRecordInfo* record) {
  DCHECK(record);

  TextEmbeddingEventInfo text_embedding_html_event;

  text_embedding_html_event.created_at =
      base::Time::FromDoubleT(ColumnDouble(record, 0));
  text_embedding_html_event.version = ColumnString(record, 1);
  text_embedding_html_event.locale = ColumnString(record, 2);
  text_embedding_html_event.hashed_key = ColumnString(record, 3);
  text_embedding_html_event.embedding = ColumnString(record, 4);

  return text_embedding_html_event;
}

}  // namespace

TextEmbeddingHtmlEvents::TextEmbeddingHtmlEvents() = default;

TextEmbeddingHtmlEvents::~TextEmbeddingHtmlEvents() = default;

void TextEmbeddingHtmlEvents::LogEvent(
    const TextEmbeddingEventInfo& text_embedding_html_event,
    ResultCallback callback) {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(transaction.get(), {text_embedding_html_event});

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void TextEmbeddingHtmlEvents::GetAll(
    GetTextEmbeddingHtmlEventsCallback callback) {
  const std::string& query = base::StringPrintf(
      "SELECT "
      "tehe.created_at, "
      "tehe.version, "
      "tehe.locale, "
      "tehe.hashed_key, "
      "tehe.embedding "
      "FROM %s AS tehe "
      "ORDER BY created_at DESC",
      GetTableName().c_str());

  RunTransaction(query, callback);
}

void TextEmbeddingHtmlEvents::PurgeStale(ResultCallback callback) {
  std::string limit =
      std::to_string(targeting::features::GetTextEmbeddingsHistorySize());
  const std::string& query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE id NOT IN "
      "(SELECT id from %s ORDER BY created_at DESC LIMIT %s) ",
      GetTableName().c_str(), GetTableName().c_str(), limit.c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string TextEmbeddingHtmlEvents::GetTableName() const {
  return kTableName;
}

void TextEmbeddingHtmlEvents::Migrate(mojom::DBTransactionInfo* transaction,
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

void TextEmbeddingHtmlEvents::RunTransaction(
    const std::string& query,
    GetTextEmbeddingHtmlEventsCallback callback) {
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // created_at
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // version
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // locale
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // hashed_key
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE   // embedding
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      std::bind(&TextEmbeddingHtmlEvents::OnGetTextEmbeddingHtmlEvents, this,
                std::placeholders::_1, callback));
}

void TextEmbeddingHtmlEvents::InsertOrUpdate(
    mojom::DBTransactionInfo* transaction,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  DCHECK(transaction);

  if (text_embedding_html_events.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->command =
      BuildInsertOrUpdateQuery(command.get(), text_embedding_html_events);

  transaction->commands.push_back(std::move(command));
}

std::string TextEmbeddingHtmlEvents::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  DCHECK(command);

  const int count = BindParameters(command, text_embedding_html_events);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(created_at, "
      "version, "
      "locale, "
      "hashed_key, "
      "embedding) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(5, count).c_str());
}

void TextEmbeddingHtmlEvents::OnGetTextEmbeddingHtmlEvents(
    mojom::DBCommandResponseInfoPtr response,
    GetTextEmbeddingHtmlEventsCallback callback) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get embeddings");
    callback(/* success */ false, {});
    return;
  }

  TextEmbeddingHtmlEventList text_embedding_html_events;

  for (const auto& record : response->result->get_records()) {
    const TextEmbeddingEventInfo& text_embedding_html_event =
        GetFromRecord(record.get());
    text_embedding_html_events.push_back(text_embedding_html_event);
  }

  callback(/* success */ true, text_embedding_html_events);
}

void TextEmbeddingHtmlEvents::MigrateToV25(
    mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  const std::string& query = base::StringPrintf(
      "CREATE TABLE text_embedding_html_events "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "created_at TIMESTAMP NOT NULL, "
      "version TEXT NOT NULL, "
      "locale TEXT NOT NULL, "
      "hashed_key TEXT NOT NULL UNIQUE, "
      "embedding TEXT NOT NULL)");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace table
}  // namespace database
}  // namespace ads
