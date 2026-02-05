// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

public struct LetterBadge<Content: View>: View {
  public let content: Content

  init(@ViewBuilder content: () -> Content) {
    self.content = content()
  }

  public var body: some View {
    ZStack {
      RoundedRectangle(cornerRadius: 6, style: .continuous)
        .fill(Color(white: 0.95))
      content
    }
    .frame(width: 32, height: 32)
    .clipShape(RoundedRectangle(cornerRadius: 6, style: .continuous))
  }
}

extension LetterBadge where Content == Text {
  public init(_ character: Character) {
    self.content = Text(String(character))
  }

  public init(_ string: String) {
    self.content = Text(string)
  }
}
