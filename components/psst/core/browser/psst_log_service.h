/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_LOG_SERVICE_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_LOG_SERVICE_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/values.h"
#include "brave/components/psst/core/common/psst_ui_common.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace psst {

// Single in-memory timeline of PSST flow diagnostics for one profile, shown on
// brave://psst-internals. Both the browser-side flow
// (`PsstTabWebContentsObserver`) and the injected user/policy scripts (via
// `mojom::PsstLogSink`) write here, so their entries interleave in order.
//
// Entries are never persisted or sent off-device, and the service is not
// created for off-the-record profiles or when the PSST feature is disabled
// (see `PsstLogServiceFactory`).
class PsstLogService : public KeyedService {
 public:
  // Oldest entries are dropped past this count.
  static constexpr size_t kMaxLogEntries = 1000;

  // `tab_id` for entries with no associated tab.
  static constexpr int32_t kInvalidTabId = -1;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPsstLogEntryAdded(const mojom::PsstLogEntry& entry) {}
    virtual void OnPsstLogCleared() {}

   protected:
    ~Observer() override = default;
  };

  PsstLogService();
  PsstLogService(const PsstLogService&) = delete;
  PsstLogService& operator=(const PsstLogService&) = delete;
  ~PsstLogService() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Appends an entry, stamping it with the current time and reducing `url` to
  // its eTLD+1 — or to its host for URLs with no registry (IP literals,
  // localhost), or to the empty string if `url` is invalid. `data` is
  // JSON-serialized for display, if present. Evicts the oldest entry once
  // `kMaxLogEntries` is reached.
  void AddEntry(mojom::PsstLogSource source,
                mojom::PsstLogLevel level,
                int32_t tab_id,
                std::string_view frame_token,
                const GURL& url,
                std::string_view message,
                std::optional<base::DictValue> data = std::nullopt);

  // Appends an entry from the browser-side flow, which is tab- rather than
  // frame-scoped.
  void AddNativeEntry(mojom::PsstLogLevel level,
                      int32_t tab_id,
                      const GURL& url,
                      std::string_view message,
                      std::optional<base::DictValue> data = std::nullopt);

  // Returns a copy of the buffered timeline, oldest first.
  std::vector<mojom::PsstLogEntryPtr> GetEntries() const;

  // Drops the whole timeline, notifying observers via `OnPsstLogCleared()`.
  void Clear();

  base::WeakPtr<PsstLogService> GetWeakPtr();

  // KeyedService:
  void Shutdown() override;

 private:
  base::circular_deque<mojom::PsstLogEntryPtr> entries_;
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<PsstLogService> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_LOG_SERVICE_H_
