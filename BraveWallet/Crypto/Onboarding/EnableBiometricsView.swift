/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import LocalAuthentication
import BraveUI

struct EnableBiometricsView: View {
  var action: (_ enable: Bool) -> Void
  
  var body: some View {
    VStack {
      Image("pin-migration-graphic")
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(maxWidth: 250)
        .padding()
      Text("Unlock Brave Wallet with your Face ID, Touch ID, or passcode.")
        .font(.headline)
        .fixedSize(horizontal: false, vertical: true)
        .multilineTextAlignment(.center)
        .padding(.bottom)
      Button(action: { action(true) }) {
        Text("Enable")
      }
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
    }
    .frame(maxWidth: .infinity)
    .padding(20)
    .overlay(
      Button(action: { action(false) }) {
        Image(systemName: "xmark")
          .padding(16)
      }
      .font(.headline)
      .foregroundColor(.gray),
      alignment: .topTrailing
    )
    .accessibilityEmbedInScrollView()
  }
}

#if DEBUG
struct EnableBiometricsView_Previews: PreviewProvider {
  static var previews: some View {
    PopupView {
      EnableBiometricsView(action: { _ in })
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
