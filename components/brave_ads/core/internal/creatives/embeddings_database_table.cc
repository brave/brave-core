/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/embeddings_database_table.h"

#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "embeddings";

size_t BindParameters(mojom::DBCommandInfo* command,
                      const CreativeAdList& creative_ads) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_set_id);
    BindString(command, index++,
               base::ToLowerASCII(VectorToDelimitedString(
                   creative_ad.embedding, kEmbeddingStringDelimiter)));

    count++;
  }

  return count;
}

void MigrateToV27(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE IF NOT EXISTS embeddings (creative_set_id TEXT NOT NULL, "
      "embedding TEXT NOT NULL, PRIMARY KEY (creative_set_id), "
      "UNIQUE(creative_set_id) ON CONFLICT REPLACE);";
  transaction->commands.push_back(std::move(command));
}

}  // namespace

void Embeddings::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                                const CreativeAdList& creative_ads) {
  CHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, creative_ads);
  transaction->commands.push_back(std::move(command));
}

void Embeddings::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(&*transaction, GetTableName());

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string Embeddings::GetTableName() const {
  return kTableName;
}

void Embeddings::Create(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE embeddings (creative_set_id TEXT NOT NULL, "
      "embedding TEXT NOT NULL, PRIMARY KEY (creative_set_id), "
      "UNIQUE(creative_set_id) ON CONFLICT REPLACE);";
  transaction->commands.push_back(std::move(command));
}

void Embeddings::Migrate(mojom::DBTransactionInfo* transaction,
                         const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 27: {
      MigrateToV27(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Embeddings::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, creative_ads);

  return base::ReplaceStringPlaceholders(
      "INSERT OR REPLACE INTO $1 (creative_set_id, embedding) VALUES $2;",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count*/ 2, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
