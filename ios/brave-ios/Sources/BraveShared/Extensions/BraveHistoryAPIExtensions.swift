// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Foundation
import ScreenTime
import Shared

extension BraveHistoryAPI {

  // MARK: Internal

  public func byFrequency(
    query: String,
    completion: @escaping ([HistoryNode]) -> Void
  ) -> HistoryCancellable? {
    if query.isEmpty {
      return nil
    }

    let options = HistorySearchOptions(
      maxCount: 100,
      duplicateHandling: .removeAll
    )

    return search(
      withQuery: query,
      options: options,
      completion: completion
    )
  }

  public func deleteAll(completion: @escaping () -> Void) {
    var screenTimeHistory: STWebHistory?
    do {
      screenTimeHistory = try STWebHistory(bundleIdentifier: Bundle.main.bundleIdentifier!)
    } catch {
      assertionFailure("STWebHistory could not be initialized: \(error)")
    }

    DispatchQueue.main.async {
      self.removeAll {
        screenTimeHistory?.deleteAllHistory()
        completion()
      }
    }
  }

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
