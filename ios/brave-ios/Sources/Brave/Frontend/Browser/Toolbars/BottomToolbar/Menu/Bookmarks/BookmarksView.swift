// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import SwiftUI

struct BookmarksView: View {

  @Environment(\.dismiss)
  private var dismiss

  var model: BookmarkModel

  @State
  private var currentStack: [BookmarkNode] = []

  @State
  private var dismissOnPop = false

  var body: some View {
    NavigationStack(path: $currentStack) {
      BookmarksListView(
        model: model,
        viewModel: BookmarkViewModel(model: model, folder: nil),
        dismiss: $dismissOnPop
      )
      .navigationDestination(for: BookmarkNode.self) { node in
        BookmarksListView(
          model: model,
          viewModel: BookmarkViewModel(model: model, folder: node),
          dismiss: $dismissOnPop
        )
      }
    }
    .task {
      var stack: [BookmarkNode] = []
      var current = model.lastVisitedFolder

      while let folder = current {
        stack.append(folder)
        current = folder.parentNode
      }

      currentStack = stack.reversed()
    }
    .onChange(of: dismissOnPop) { _, newValue in
      if newValue {
        currentStack = []
        dismiss()
      }
    }
    .onChange(of: currentStack) { _, currentStack in
      if !currentStack.isEmpty {
        Preferences.Chromium.lastBookmarksFolderNodeId.value = currentStack.last?.id ?? -1
      }
    }
  }
}
