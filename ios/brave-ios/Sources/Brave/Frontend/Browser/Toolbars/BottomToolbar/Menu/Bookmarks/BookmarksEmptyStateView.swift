// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import Foundation
import Preferences
import Strings
import SwiftUI

struct BookmarksEmptyStateView: View {
  var isSearching: Bool

  var body: some View {
    VStack(spacing: 16) {
      Image(braveSystemName: "leo.product.bookmarks")
        .font(.title)
        .imageScale(.large)
        .foregroundStyle(Color(braveSystemName: .iconSecondary))
      VStack {
        Text(
          isSearching
            ? Strings.Bookmarks.bookmarksEmptySearchTitle
            : Strings.Bookmarks.bookmarksEmptyStateTitle
        )
        .font(.headline)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
    }
  }
}

#if DEBUG
#Preview {
  BookmarksEmptyStateView(isSearching: false)
}
#endif
