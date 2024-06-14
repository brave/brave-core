// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct BraveVPNRegionLoadingIndicatorView: View {

  @State
  private var isAnimating = false

  var body: some View {
    ZStack {
      Circle()
        .stroke(lineWidth: 7)
        .opacity(0.3)
        .foregroundStyle(Color(braveSystemName: .iconInteractive))
        .frame(width: 48, height: 48)
      Circle()
        .trim(from: 0.75, to: 1.0)
        .stroke(style: StrokeStyle(lineWidth: 7, lineCap: .round))
        .foregroundColor(Color(braveSystemName: .iconInteractive))
        .frame(width: 48, height: 48)
        .rotationEffect(Angle(degrees: isAnimating ? 360 : 0))
        .onAppear {
          withAnimation(Animation.linear(duration: 1.0).repeatForever(autoreverses: false)) {
            isAnimating = true
          }
        }
    }
  }
}

struct BraveVPNLoadingIndicatorView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionLoadingIndicatorView()
  }
}
