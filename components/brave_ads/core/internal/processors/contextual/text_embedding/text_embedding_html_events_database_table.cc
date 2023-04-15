/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_events_database_table.h"

#include <utility>

#include "base/check.h"
#include "base/functional/callback.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_embedding/text_embedding_features.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "text_embedding_html_events";
constexpr char kDelimiter[] = " ";

int BindParameters(
    mojom::DBCommandInfo* command,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& text_embedding_html_event : text_embedding_html_events) {
    BindInt64(command, index++,
              text_embedding_html_event.created_at.ToDeltaSinceWindowsEpoch()
                  .InMicroseconds());
    BindString(command, index++, text_embedding_html_event.locale);
    BindString(command, index++, text_embedding_html_event.hashed_text_base64);
    BindString(command, index++,
               VectorToDelimitedString(text_embedding_html_event.embedding,
                                       kDelimiter));

    count++;
  }

  return count;
}

TextEmbeddingHtmlEventInfo GetFromRecord(mojom::DBRecordInfo* record) {
  DCHECK(record);

  TextEmbeddingHtmlEventInfo text_embedding_html_event;

  text_embedding_html_event.created_at = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(ColumnInt64(record, 0)));
  text_embedding_html_event.locale = ColumnString(record, 1);
  text_embedding_html_event.hashed_text_base64 = ColumnString(record, 2);
  text_embedding_html_event.embedding =
      DelimitedStringToVector(ColumnString(record, 3), kDelimiter);

  return text_embedding_html_event;
}

void OnGetTextEmbeddingHtmlEvents(
    GetTextEmbeddingHtmlEventsCallback callback,
    mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get embeddings");
    return std::move(callback).Run(/*success*/ false,
                                   /*text_embedding_html_events*/ {});
  }

  TextEmbeddingHtmlEventList text_embedding_html_events;

  for (const auto& record : command_response->result->get_records()) {
    const TextEmbeddingHtmlEventInfo& text_embedding_html_event =
        GetFromRecord(record.get());
    text_embedding_html_events.push_back(text_embedding_html_event);
  }

  std::move(callback).Run(/* success */ true, text_embedding_html_events);
}

void RunTransaction(const std::string& query,
                    GetTextEmbeddingHtmlEventsCallback callback) {
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE,   // created_at
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // locale
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // hashed_text_base64
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE  // embedding
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetTextEmbeddingHtmlEvents, std::move(callback)));
}

void MigrateToV25(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  const std::string& query = base::StringPrintf(
      "CREATE TABLE IF NOT EXISTS text_embedding_html_events "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "created_at TIMESTAMP NOT NULL, "
      "locale TEXT NOT NULL, "
      "hashed_text_base64 TEXT NOT NULL UNIQUE, "
      "embedding TEXT NOT NULL)");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace

void TextEmbeddingHtmlEvents::LogEvent(
    const TextEmbeddingHtmlEventInfo& text_embedding_html_event,
    ResultCallback callback) {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(transaction.get(), {text_embedding_html_event});

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void TextEmbeddingHtmlEvents::GetAll(
    GetTextEmbeddingHtmlEventsCallback callback) const {
  const std::string& query = base::StringPrintf(
      "SELECT "
      "tehe.created_at, "
      "tehe.locale, "
      "tehe.hashed_text_base64, "
      "tehe.embedding "
      "FROM %s AS tehe "
      "ORDER BY created_at DESC",
      GetTableName().c_str());

  RunTransaction(query, std::move(callback));
}

void TextEmbeddingHtmlEvents::PurgeStale(ResultCallback callback) const {
  const std::string limit =
      base::NumberToString(targeting::kTextEmbeddingHistorySize.Get());
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
      base::BindOnce(&OnResultCallback, std::move(callback)));
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
    const TextEmbeddingHtmlEventList& text_embedding_html_events) const {
  DCHECK(command);

  const int binded_parameters_count =
      BindParameters(command, text_embedding_html_events);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(created_at, "
      "locale, "
      "hashed_text_base64, "
      "embedding) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(/*parameters_count*/ 4,
                                        binded_parameters_count)
          .c_str());
}

}  // namespace brave_ads::database::table
