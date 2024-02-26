// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Algorithms
import SwiftUI

/// Displays a grid of recovery words
struct RecoveryPhraseGrid<Word: Hashable, ID: Hashable, Content: View>: View {
  @Environment(\.sizeCategory) private var sizeCategory

  var data: [Word]
  var id: KeyPath<Word, ID>
  var spacing: CGFloat
  var content: (Word) -> Content

  init(
    data: [Word],
    id: KeyPath<Word, ID>,
    spacing: CGFloat = 8.0,
    @ViewBuilder content: @escaping (Word) -> Content
  ) {
    self.data = data
    self.id = id
    self.spacing = spacing
    self.content = content
  }

  private var numberOfColumns: Int {
    sizeCategory.isAccessibilityCategory ? 2 : 3
  }

  /// The data chunked into columns for iOS 13 APIs
  private var gridData: [[Word]] {
    data.chunks(ofCount: numberOfColumns).map(Array.init)
  }

  var body: some View {
    let columns: [GridItem] = (0..<numberOfColumns).map { _ in .init(.flexible()) }
    LazyVGrid(columns: columns, spacing: spacing) {
      ForEach(data, id: id) { entry in
        content(entry)
      }
    }
  }
}

#if DEBUG
struct RecoveryPhraseGrid_Previews: PreviewProvider {
  static let data: [String] = [
    "Tomato", "Green", "Velvet", "Span",
    "Celery", "Atoms", "Parent", "Stop",
    "Bowl", "Wishful", "Stone", "Exercise",
  ]
  static var previews: some View {
    Group {
      RecoveryPhraseGrid(data: Self.data, id: \.self) { word in
        Text(word)
          .frame(maxWidth: .infinity)
          .border(Color.gray)
      }
      .previewDisplayName("Normal")
      RecoveryPhraseGrid(data: Self.data, id: \.self) { word in
        Text(word)
          .frame(maxWidth: .infinity)
          .border(Color.gray)
      }
      .environment(\.sizeCategory, .accessibilityLarge)
      .previewDisplayName("Accessibility")
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
