/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_DATABASE_TABLE_H_

#include <functional>
#include <string>

#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_event_info.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {
namespace table {

using GetTextEmbeddingHtmlEventsCallback =
    std::function<void(const bool, const TextEmbeddingHtmlEventList&)>;

class TextEmbeddingHtmlEvents final : public TableInterface {
 public:
  TextEmbeddingHtmlEvents();
  ~TextEmbeddingHtmlEvents() override;
  TextEmbeddingHtmlEvents(const TextEmbeddingHtmlEvents&) = delete;
  TextEmbeddingHtmlEvents& operator=(const TextEmbeddingHtmlEvents&) = delete;

  void LogEvent(const TextEmbeddingEventInfo& text_embedding_html_event,
                ResultCallback callback);

  void GetAll(GetTextEmbeddingHtmlEventsCallback callback);

  void PurgeStale(ResultCallback callback);

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransaction* transaction,
               const int to_version) override;

 private:
  void RunTransaction(const std::string& query,
                      GetTextEmbeddingHtmlEventsCallback callback);

  void InsertOrUpdate(
      mojom::DBTransaction* transaction,
      const TextEmbeddingHtmlEventList& text_embedding_html_events);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommand* command,
      const TextEmbeddingHtmlEventList& text_embedding_html_events);

  void OnGetTextEmbeddingHtmlEvents(
      mojom::DBCommandResponsePtr response,
      GetTextEmbeddingHtmlEventsCallback callback);

  void MigrateToV25(mojom::DBTransaction* transaction);
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_DATABASE_TABLE_H_
