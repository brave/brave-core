// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

public struct SectionFooterErrorView: View {
  private let errorMessage: String?
  
  public init(errorMessage: String?) {
    self.errorMessage = errorMessage
  }
  
  public var body: some View {
    if let errorMessage {
      HStack(alignment: .firstTextBaseline, spacing: 4) {
        Image(systemName: "exclamationmark.circle.fill")
        Text(errorMessage)
          .fixedSize(horizontal: false, vertical: true)
          .animation(nil, value: errorMessage)
      }
      .transition(
        .asymmetric(
          insertion: .opacity.animation(.default),
          removal: .identity
        )
      )
      .font(.footnote)
      .foregroundColor(Color(.braveErrorLabel))
      .padding(.bottom)
    } else {
      // SwiftUI will add/remove this Section footer when addressError
      // optionality is changed. This can cause keyboard to dismiss.
      // By using clear square, the footer remains and section is not
      // redrawn, so the keyboard does not dismiss as user types.
      Color.clear.frame(width: 1, height: 1)
        .accessibility(hidden: true)
        .transition(.identity)
    }
  }
}
