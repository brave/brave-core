// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI
import UIKit

public struct URLElidedText: View {
  public var text: String

  @Environment(\.font)
  private var font: Font?

  @Environment(\.truncationMode)
  private var truncationMode: Text.TruncationMode

  private var paragraphStyle: NSParagraphStyle {
    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.lineBreakMode =
      truncationMode == .head
      ? .byTruncatingHead : truncationMode == .middle ? .byTruncatingMiddle : .byTruncatingTail
    paragraphStyle.baseWritingDirection = .leftToRight
    return paragraphStyle
  }

  public init(text: String) {
    self.text = text
  }

  public var body: some View {
    // LRM character prevents Text elements from rendering RTL special characters
    Text(
      AttributedString(
        "\u{200E}\(text)",
        attributes: .init([.font: font ?? .body, .paragraphStyle: paragraphStyle])
      )
    )
  }
}
