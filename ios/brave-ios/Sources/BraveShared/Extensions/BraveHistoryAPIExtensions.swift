// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension BraveHistoryAPI {
  @MainActor public func search(
    withQuery query: String?,
    options: HistorySearchOptions
  ) async -> [HistoryNode] {
    let holder = HistoryCancellableHolder()
    return await withTaskCancellationHandler(
      operation: {
        return await withCheckedContinuation { continuation in
          holder.value = search(
            withQuery: query,
            options: options,
            completion: {
              holder.value = nil
              continuation.resume(returning: $0)
            }
          )
        }
      },
      onCancel: {
        holder.value?.cancel()
      }
    )
  }
}

/// Holder to suppress the Sendable warning on `HistoryCancellable`.
private final class HistoryCancellableHolder: @unchecked Sendable {
  var value: HistoryCancellable?
}
