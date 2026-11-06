// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import os.log

public class BookmarksExportUtility {

  public init() {}

  // Export bookmarks from BraveCore to a file
  @MainActor
  public func exportBookmarks(to path: URL) async -> Bool {
    precondition(
      state == .none,
      "Bookmarks Import - Error Exporting while an Import/Export operation is in progress"
    )

    self.state = .exporting
    return await withCheckedContinuation { continuation in
      self.exporter.export(toFile: path.path) { [weak self] state in
        guard let self = self else {
          continuation.resume(returning: false)
          return
        }

        self.state = .none
        switch state {
        case .started:
          Logger.module.debug("Bookmarks Export - Export Started")
        case .errorCreatingFile, .errorWritingHeader, .errorWritingNodes:
          Logger.module.debug("Bookmarks Export - Export Error")
          continuation.resume(returning: false)
        case .cancelled:
          Logger.module.debug("Bookmarks Export - Export Cancelled")
          continuation.resume(returning: false)
        case .completed:
          Logger.module.debug("Bookmarks Export - Export Completed")
          continuation.resume(returning: true)
        @unknown default:
          fatalError()
        }
      }
    }
  }

  // MARK: - Private
  private var state: State = .none
  private let exporter = BraveBookmarksExporter()

  private enum State {
    case exporting
    case none
  }
}
