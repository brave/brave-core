/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI

struct WalletDisclosureGroup<Label: View, Content: View>: View {
  @Binding var isExpanded: Bool
  @ViewBuilder var content: () -> Content
  @ViewBuilder var label: () -> Label
  
  var body: some View {
    HStack {
      label()
      Spacer()
      Image(braveSystemName: "leo.carat.down")
        .rotationEffect(.degrees(isExpanded ? 180 : 0))
        .foregroundColor(Color(.braveBlurpleTint))
    }
    .contentShape(Rectangle())
    .onTapGesture {
      withAnimation {
        isExpanded.toggle()
      }
    }
    if isExpanded {
      content()
    }
  }
}
