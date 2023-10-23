/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI

struct WalletDisclosureGroup<Label: View, Content: View>: View {
  var isNFTGroup: Bool
  @Binding var isExpanded: Bool
  @ViewBuilder var content: () -> Content
  @ViewBuilder var label: () -> Label
  
  private var header: some View {
    HStack {
      label()
      Spacer()
      Image(braveSystemName: "leo.carat.down")
        .rotationEffect(.degrees(isExpanded ? 180 : 0))
        .foregroundColor(Color(.braveBlurpleTint))
        .animation(.default, value: isExpanded)
    }
    .padding(.horizontal)
    // when expanded, padding is applied to entire `LazyVStack`
    .padding(.vertical, isExpanded ? 0 : 6)
    .osAvailabilityModifiers {
      if !isExpanded {
        $0.overlay {
          RoundedRectangle(cornerRadius: 16)
            .stroke(Color(braveSystemName: .dividerSubtle), lineWidth: 1)
        }
      } else {
        $0
      }
    }
    .contentShape(Rectangle())
    .onTapGesture {
      isExpanded.toggle()
    }
  }
  
  var body: some View {
    LazyVStack {
      header
      if isExpanded {
        Divider()
          .padding(.top, 6)
          .padding(.horizontal, isNFTGroup ? nil : 8)
        content()
          .padding(.horizontal)
      }
    }
    // when collapsed, padding is applied to `header`
    .padding(.vertical, isExpanded ? 6 : 0)
    .osAvailabilityModifiers {
      if !isNFTGroup {
        if isExpanded {
          $0.overlay {
            RoundedRectangle(cornerRadius: 16)
              .stroke(Color(braveSystemName: .dividerSubtle), lineWidth: 1)
          }
        } else {
          $0
        }
      } else {
        $0
      }
    }
  }
}
