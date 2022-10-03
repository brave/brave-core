/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/embeddings_database_table.h"

#include <functional>
#include <utility>

#include "base/bind.h"
#include "base/check.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/database/database_bind_util.h"
#include "bat/ads/internal/base/database/database_table_util.h"
#include "bat/ads/internal/base/database/database_transaction_util.h"

namespace ads::database::table {

namespace {

constexpr char kTableName[] = "embeddings";

int BindParameters(mojom::DBCommandInfo* command,
                   const CreativeAdList& creative_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_set_id);
    BindString(command, index++, base::ToLowerASCII(creative_ad.embedding));

    count++;
  }

  return count;
}

}  // namespace

void Embeddings::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                              const CreativeAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

void Embeddings::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(transaction.get(), GetTableName());

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

std::string Embeddings::GetTableName() const {
  return kTableName;
}

void Embeddings::Migrate(mojom::DBTransactionInfo* transaction,
                       const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 26: {
      MigrateToV26(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Embeddings::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
  DCHECK(command);

  const int count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_set_id, "
      "embedding) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(2, count).c_str());
}

void Embeddings::MigrateToV26(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  DropTable(transaction, "embeddings");

  const std::string query =
      "CREATE TABLE IF NOT EXISTS embeddings"
      "(creative_set_id TEXT NOT NULL, "
      "embedding TEXT NOT NULL, "
      "PRIMARY KEY (creative_set_id), "
      "UNIQUE(creative_set_id) ON CONFLICT REPLACE)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace ads::database::table
