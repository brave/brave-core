/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "brave/components/brave_ads/core/ads_client_callback.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"

namespace brave_ads::database::table {

using GetTextEmbeddingHtmlEventsCallback = base::OnceCallback<void(
    const bool success,
    const TextEmbeddingHtmlEventList& text_embedding_html_events)>;

class TextEmbeddingHtmlEvents final : public TableInterface {
 public:
  void LogEvent(const TextEmbeddingHtmlEventInfo& text_embedding_html_event,
                ResultCallback callback);

  void GetAll(GetTextEmbeddingHtmlEventsCallback callback) const;

  void PurgeStale(ResultCallback callback) const;

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(
      mojom::DBTransactionInfo* transaction,
      const TextEmbeddingHtmlEventList& text_embedding_html_events);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommandInfo* command,
      const TextEmbeddingHtmlEventList& text_embedding_html_events) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_DATABASE_TABLE_H_
