// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI
import UIKit

extension URLFormatter {
  public static func createAttributedString(
    string: String,
    font: Font,
    lineBreakMode: NSLineBreakMode
  ) -> AttributedString {
    var attributedString = AttributedString("\u{200E}\(string)")  // LRM character prevents Text elements from rendering RTL special characters

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.lineBreakMode = lineBreakMode
    paragraphStyle.baseWritingDirection = .leftToRight

    attributedString.setAttributes(
      .init([
        .font: font,
        .paragraphStyle: paragraphStyle,
      ])
    )

    return attributedString
  }
}
