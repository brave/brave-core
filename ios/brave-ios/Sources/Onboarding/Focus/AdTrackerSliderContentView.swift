// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct AdTrackerSliderContentView: View {
  var body: some View {
    SwipeDifferenceView {
      Image("focus-website-ads", bundle: .module)
    } trailing: {
      Image("focus-website-noads", bundle: .module)
    }
    .frame(width: .infinity, height: 420)
  }
}

struct SwipeDifferenceView<Leading: View, Trailing: View>: View {
  @State private var progress: CGFloat = 0.25
  @GestureState private var initialProgress: CGFloat?

  @Environment(\.layoutDirection) private var layoutDirection

  var leading: Leading
  var trailing: Trailing

  init(
    @ViewBuilder leading: () -> Leading,
    @ViewBuilder trailing: () -> Trailing
  ) {
    self.leading = leading()
    self.trailing = trailing()
  }

  var isRTL: Bool {
    layoutDirection == .rightToLeft
  }

  @ViewBuilder private var actualLeading: some View {
    if isRTL {
      trailing
    } else {
      leading
    }
  }

  @ViewBuilder private var actualTrailing: some View {
    if isRTL {
      leading
    } else {
      trailing
    }
  }

  var body: some View {
    ZStack {
      actualLeading
      actualTrailing
        .mask {
          GeometryReader { proxy in
            Color.black
              .frame(width: proxy.size.width * (isRTL ? progress : (1 - progress)))
              .frame(
                maxWidth: .infinity,
                maxHeight: .infinity,
                alignment: isRTL ? .leading : .trailing
              )
          }
        }
    }
    .overlay {
      // Fake "grabber"/splitter
      GeometryReader { proxy in
        Color(braveSystemName: .textPrimary)
          .frame(width: 4)
          .shadow(radius: 8)
          .overlay {
            Image("focus-slider-knob", bundle: .module)
              .padding(.leading, 6)
          }
          .offset(x: (proxy.size.width * progress))
      }
    }
    .overlay {
      // Drag gesture view
      GeometryReader { proxy in
        Color.clear
          .contentShape(Rectangle())
          .gesture(
            DragGesture()
              .updating(
                $initialProgress,
                body: { value, state, _ in
                  // Sets initialProgress when the gesture is active
                  if state == nil {
                    state = progress
                  }
                }
              )
              .onChanged { value in
                guard let initialProgress else { return }
                withAnimation(.interactiveSpring) {
                  progress = max(
                    0,
                    min(1, initialProgress + (value.translation.width) / proxy.size.width)
                  )
                  if isRTL {
                    progress = 1 - progress
                  }
                }
              }
          )
      }
    }
    .padding()
  }
}

#Preview("LTR") {
  AdTrackerSliderContentView()
}

#Preview("RTL") {
  AdTrackerSliderContentView()
    .environment(\.layoutDirection, .rightToLeft)
}
