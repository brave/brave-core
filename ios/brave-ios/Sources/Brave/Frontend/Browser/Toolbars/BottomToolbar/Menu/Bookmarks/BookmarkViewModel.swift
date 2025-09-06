// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

/// View Model that holds all the information required to render a list of bookmarks
class BookmarkViewModel: ObservableObject {

  private var model: BookmarkModel

  private var listener: BookmarkModelListener?

  private(set) var currentSearchQuery: String?

  @Published
  private(set) var folder: BookmarkNode?

  @Published
  private(set) var items = [BookmarkNode]()

  @Published
  private(set) var isLoading = false

  init(model: BookmarkModel, folder: BookmarkNode?) {
    self.model = model
    self.folder = folder

    listener = model.addListener(
      .init({ event in
        Task { [weak self] in
          guard let self = self else { return }
          await self.refresh(query: self.currentSearchQuery)
        }
      })
    )
  }

  deinit {
    listener?.destroy()
  }

  func refresh() async {
    await refresh(query: currentSearchQuery)
  }

  @MainActor
  func refresh(query: String?) async {
    isLoading = true
    currentSearchQuery = query
    items = await model.bookmarks(for: folder, query: query)
    isLoading = false
  }
}
