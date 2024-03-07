// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

// FIXME: Delete or realize into full modifier
struct DebugSizeModifier: ViewModifier {
  func body(content: Content) -> some View {
    content
      .overlay {
        GeometryReader { proxy in
          Color.clear.border(Color.red)
            .overlay(alignment: .bottomLeading) {
              let numberFormatter = {
                let nf = NumberFormatter()
                nf.maximumFractionDigits = 1
                nf.minimumFractionDigits = 0
                return nf
              }()
              let width = numberFormatter.string(from: NSNumber(value: proxy.size.width))!
              let height = numberFormatter.string(from: NSNumber(value: proxy.size.height))!
              Text("\(width) × \(height)")
                .font(.footnote)
                .foregroundStyle(.white)
                .background(Color.red)
                .fixedSize()
                .alignmentGuide(VerticalAlignment.bottom) { d in
                  d[.top] + 1
                }
            }
        }
      }
  }
}
