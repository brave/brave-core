// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct WalletIconButton: View {

  enum IconSymbol {
    case braveSystemName(String)
    case systemName(String)
  }

  let iconSymbol: IconSymbol
  let action: () -> Void

  @ScaledMetric var length: CGFloat = 36

  init(
    braveSystemName: String,
    action: @escaping () -> Void,
    length: CGFloat = 36
  ) {
    self.iconSymbol = .braveSystemName(braveSystemName)
    self.action = action
    self._length = .init(wrappedValue: length)
  }

  init(
    systemName: String,
    action: @escaping () -> Void,
    length: CGFloat = 36
  ) {
    self.iconSymbol = .systemName(systemName)
    self.action = action
    self._length = .init(wrappedValue: length)
  }

  var body: some View {
    Button(action: action) {
      Group {
        switch iconSymbol {
        case .braveSystemName(let braveSystemName):
          Image(braveSystemName: braveSystemName)
        case .systemName(let systemName):
          Image(systemName: systemName)
        }
      }
      .foregroundColor(Color(braveSystemName: .iconInteractive))
      .imageScale(.medium)
      .padding(6)
      .frame(width: length, height: length)
      .background(
        Circle()
          .strokeBorder(Color(braveSystemName: .dividerInteractive), lineWidth: 1)
      )
    }
  }
}
