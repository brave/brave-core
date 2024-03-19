// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Lottie
import SwiftUI

struct FocusAdTrackerSliderContentView: View {
  @Environment(\.colorScheme) private var colorScheme

  @State private var progress: CGFloat = 0.75

  var body: some View {
    SwipeDifferenceView(progress: $progress) {
      Image("focus-website-ads", bundle: .module)
        .overlay(
          LottieAnimationView(
            name: colorScheme == .dark ? "moving-ads-dark" : "moving-ads-light",
            bundle: .module
          )
          .loopMode(.loop)
          .resizable()
          .aspectRatio(contentMode: .fill)
        )
    } trailing: {
      Image("focus-website-noads", bundle: .module)
    }
    .frame(height: 420)
    .frame(maxWidth: .infinity)
    .onAppear {
      Timer.scheduledTimer(withTimeInterval: 2.5, repeats: false) { _ in
        withAnimation(.easeInOut(duration: 1.5)) {
          progress = 0.9
        }
        Timer.scheduledTimer(withTimeInterval: 1.5, repeats: false) { _ in
          withAnimation(.easeInOut(duration: 1.5)) {
            progress = 0.1
          }
          Timer.scheduledTimer(withTimeInterval: 1.5, repeats: false) { _ in
            withAnimation(.easeInOut(duration: 1.5)) {
              progress = 0.25
            }
          }
        }
      }
    }
  }
}

struct FocusVideoAdSliderContentView: View {
  @Environment(\.colorScheme) private var colorScheme

  var body: some View {
    LottieAnimationView(
      name: colorScheme == .dark ? "novideo-ads-dark" : "novideo-ads-light",
      bundle: .module
    )
    .loopMode(.loop)
    .resizable()
    .aspectRatio(contentMode: .fill)
    .frame(height: 804)
  }
}

struct SwipeDifferenceView<Leading: View, Trailing: View>: View {
  @Environment(\.layoutDirection) private var layoutDirection

  @GestureState private var initialProgress: CGFloat?

  @Binding var progress: CGFloat

  var leading: Leading
  var trailing: Trailing

  init(
    progress: Binding<CGFloat>,
    @ViewBuilder leading: () -> Leading,
    @ViewBuilder trailing: () -> Trailing
  ) {
    self._progress = progress
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

#Preview("AdTrackerLTR") {
  FocusAdTrackerSliderContentView()
}

#Preview("AdTrackerRTL") {
  FocusAdTrackerSliderContentView()
    .environment(\.layoutDirection, .rightToLeft)
}

#Preview("VideoTracker") {
  FocusVideoAdSliderContentView()
}
