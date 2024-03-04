// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct WaveProgressViewStyle: ProgressViewStyle {
  private var filledCircleColor = Color(UIColor(rgb: 0x7C91FF))

  func makeBody(configuration: Configuration) -> some View {
    Circle()
      .fill(filledCircleColor)
      .frame(width: 5.0, height: 5.0)
      .overlay(
        ZStack {
          Circle()
            .stroke(filledCircleColor, lineWidth: 20.0)
            .scaleEffect(1.0)
          Circle()
            .stroke(filledCircleColor, lineWidth: 20.0)
            .scaleEffect(1.5)
          Circle()
            .stroke(filledCircleColor, lineWidth: 20.0)
            .scaleEffect(2.0)
        }
        .opacity(0.0)
        .animation(Animation.easeInOut(duration: 1).repeatForever(autoreverses: false), value: true)
      )
  }
}

struct DotsProgressViewStyle: ProgressViewStyle {
  func makeBody(configuration: Configuration) -> some View {
    TimelineView(.periodic(from: .now, by: 0.3)) { context in
      DotsView(date: context.date)
    }
  }

  private struct DotsView: View {
    @State
    private var phase = 0
    private var maxPhase = 3
    private var emptyCircleColor = Color(UIColor(rgb: 0xD5DCFF))
    private var filledCircleColor = Color(UIColor(rgb: 0x7C91FF))

    let date: Date

    init(date: Date) {
      self.date = date
    }

    var body: some View {
      HStack(spacing: 4.0) {
        Circle()
          .fill(phase == 0 ? filledCircleColor : emptyCircleColor)
          .frame(width: 8.0, height: 8.0)
          .scaleEffect(phase == 0 ? 1.0 : 0.5)

        Circle()
          .fill(phase == 1 ? filledCircleColor : emptyCircleColor)
          .frame(width: 8.0, height: 8.0)
          .scaleEffect(phase == 1 ? 1.0 : 0.5)

        Circle()
          .fill(phase == 2 ? filledCircleColor : emptyCircleColor)
          .frame(width: 8.0, height: 8.0)
          .scaleEffect(phase == 2 ? 1.0 : 0.5)
      }
      .onChange(of: date) { (date: Date) in
        phase = (phase + 1) % maxPhase
      }
    }
  }
}

struct AIChatLoaderView: View {
  var body: some View {
    AIChatProductIcon(containerShape: Circle())

    ProgressView()
      .progressViewStyle(DotsProgressViewStyle())
  }
}
