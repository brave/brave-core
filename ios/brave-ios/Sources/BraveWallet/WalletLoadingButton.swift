// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct WalletLoadingButton<Label: View>: View {
  var isLoading: Bool
  var action: () -> Void
  var label: Label

  init(
    isLoading: Bool,
    action: @escaping () -> Void,
    @ViewBuilder label: () -> Label
  ) {
    self.isLoading = isLoading
    self.action = action
    self.label = label()
  }

  var body: some View {
    Button {
      if !isLoading {
        action()
      }
    } label: {
      ZStack {
        label
          .opacity(isLoading ? 0 : 1)
        ProgressView()
          .opacity(isLoading ? 1 : 0)
      }
    }
  }
}

#if DEBUG
struct WalletLoadingButton_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      WalletLoadingButton(
        isLoading: true,
        action: {
          // preview
        },
        label: {
          Text("Preview")
        }
      )
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
      .disabled(true)
      .frame(maxWidth: .infinity)
      WalletLoadingButton(
        isLoading: false,
        action: {
          // preview
        },
        label: {
          Text("Preview")
        }
      )
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
      .disabled(false)
      .frame(maxWidth: .infinity)
    }
  }
}
#endif
