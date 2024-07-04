// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct BraveVPNRegionLoadingIndicatorView: View {
  @State
  private var isAnimating = false

  var body: some View {
    ProgressView()
      .progressViewStyle(BraveVPNCircularProgressViewStyle(thickness: 3))
  }
}

struct BraveVPNCircularProgressViewStyle: ProgressViewStyle {
  private let thickness: Double

  init(thickness: Double) {
    self.thickness = thickness
  }

  func makeBody(configuration: Configuration) -> some View {
    TimelineView(.animation(minimumInterval: 0.1, paused: false)) { context in
      BraveVPNCircularProgressView(
        endDate: context.date,
        thickness: thickness
      )
    }
  }

  private struct BraveVPNCircularProgressView: View {
    @State
    private var startDate: Date = .now

    private var thickness: Double

    private let endDate: Date

    init(endDate: Date, thickness: Double) {
      self.endDate = endDate
      self.thickness = thickness
    }

    private var progress: Double {
      return (endDate.timeIntervalSince1970 - startDate.timeIntervalSince1970) * 3
    }

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
          .rotationEffect(.degrees(90.0 * progress))
          .animation(.linear, value: endDate)
      }
    }
  }
}

#if DEBUG
struct BraveVPNLoadingIndicatorView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionLoadingIndicatorView()
  }
}
#endif
