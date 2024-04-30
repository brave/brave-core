// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreMedia
import Foundation
import SwiftUI

/// FIXME: Add doc
/// FIXME: Support RTL layout direction
@available(iOS 16.0, *)
struct MediaScrubber<Label: View>: View {
  @Binding var currentTime: TimeInterval
  var duration: TimeInterval
  @Binding var isScrubbing: Bool
  var label: Label

  init(
    currentTime: Binding<TimeInterval>,
    duration: TimeInterval,
    isScrubbing: Binding<Bool>,
    @ViewBuilder label: () -> Label
  ) {
    self._currentTime = currentTime
    self.duration = duration
    self._isScrubbing = isScrubbing
    self.label = label()
  }

  @GestureState private var isScrubbingState: Bool = false
  @ScaledMetric private var barHeight = 4
  @ScaledMetric private var thumbSize = 12

  private var currentValueLabel: Text {
    return Text(.seconds(currentTime), format: .time(pattern: .minuteSecond))
  }

  private var remainingTimeLabel: Text {
    let value = Text(.seconds(duration - currentTime), format: .time(pattern: .minuteSecond))
    return Text("-\(value)")
  }

  private var durationLabel: Text {
    Text(.seconds(duration), format: .time(pattern: .minuteSecond))
  }

  private var barShape: some InsettableShape {
    // FIXME: Design uses a regular Rectangle
    RoundedRectangle(cornerRadius: barHeight / 2, style: .continuous)
  }

  var body: some View {
    VStack {
      barShape
        .opacity(0.3)
        .foregroundStyle(.tint)
        .frame(height: barHeight)
        .overlay {
          if duration > 0 {
            // Active value
            GeometryReader { proxy in
              barShape
                .foregroundStyle(.tint)
                .frame(
                  width: min(
                    proxy.size.width,
                    CGFloat(currentTime / duration) * proxy.size.width
                  ),
                  alignment: .leading
                )
                .animation(.linear(duration: 0.1), value: currentTime)
            }
          }
        }
        .padding(.vertical, (thumbSize - barHeight) / 2)
        .overlay {
          if duration > 0 {
            // Thumb
            GeometryReader { proxy in
              Circle()
                .foregroundStyle(.tint)
                .frame(width: thumbSize, height: thumbSize)
                .scaleEffect(isScrubbing ? 1.5 : 1)
                .animation(.spring(response: 0.2, dampingFraction: 0.7), value: isScrubbing)
                .offset(
                  x: min(
                    proxy.size.width,
                    (CGFloat(currentTime / duration) * proxy.size.width)
                  ) - (thumbSize / 2)
                )
                .animation(.linear(duration: 0.1), value: currentTime)
                .gesture(
                  DragGesture(minimumDistance: 0)
                    .updating(
                      $isScrubbingState,
                      body: { _, state, _ in
                        state = true
                      }
                    )
                    .onChanged { state in
                      let seconds = max(
                        0,
                        min(
                          duration,
                          (state.location.x / proxy.size.width) * CGFloat(duration)
                        )
                      )
                      currentTime = seconds
                    }
                )
            }
          }
        }
        .disabled(duration.isZero)
      label
    }
    .onChange(of: isScrubbingState) { newValue in
      isScrubbing = newValue
    }
    .accessibilityRepresentation {
      if duration > 0 {
        Slider(
          value: $currentTime,
          in: 0.0...duration,
          step: 1
        ) {
          Text("Current Media Time")  // TODO: Localize
        } minimumValueLabel: {
          currentValueLabel
        } maximumValueLabel: {
          durationLabel
        }
      }
    }
  }
}

@available(iOS 16.0, *)
extension MediaScrubber where Label == DefaultMediaScrubberLabel {
  init(
    currentTime: Binding<TimeInterval>,
    duration: TimeInterval,
    isScrubbing: Binding<Bool>
  ) {
    self._currentTime = currentTime
    self.duration = duration
    self._isScrubbing = isScrubbing
    self.label = DefaultMediaScrubberLabel(
      currentTime: currentTime.wrappedValue,
      duration: duration
    )
  }
}

@available(iOS 16.0, *)
struct DefaultMediaScrubberLabel: View {
  var currentTime: TimeInterval
  var duration: TimeInterval

  @State private var isShowingTotalTime: Bool = false

  private var currentValueLabel: Text {
    return Text(.seconds(currentTime), format: .time(pattern: .minuteSecond))
  }

  private var remainingTimeLabel: Text {
    let value = Text(.seconds(duration - currentTime), format: .time(pattern: .minuteSecond))
    return Text("-\(value)")
  }

  private var durationLabel: Text {
    Text(.seconds(duration), format: .time(pattern: .minuteSecond))
  }

  var body: some View {
    HStack {
      currentValueLabel
      Spacer()
      Button {
        isShowingTotalTime.toggle()
      } label: {
        Group {
          if isShowingTotalTime {
            durationLabel
          } else {
            remainingTimeLabel
          }
        }
        .transition(.move(edge: .trailing).combined(with: .opacity))
      }
    }
    .foregroundStyle(.primary)
    .font(.footnote)
  }
}

@available(iOS 16.0, *)
extension Duration {
  fileprivate var isValid: Bool {
    components.seconds > 0
  }
  fileprivate var seconds: Double {
    Double(components.seconds)
  }
}

#if DEBUG
@available(iOS 16.0, *)
private struct MediaScrubberPreview: View {
  @State private var currentTime: TimeInterval = 0
  @State private var isScrubbing: Bool = false

  var body: some View {
    VStack {
      MediaScrubber(
        currentTime: $currentTime,
        duration: 1000,
        isScrubbing: $isScrubbing
      )
      .padding()
      MediaScrubber(
        currentTime: $currentTime,
        duration: 1000,
        isScrubbing: $isScrubbing
      )
      .tint(.red)
      .environment(\.colorScheme, .dark)
      .padding()
      .background(Color.black)

      Button {
        // FIXME: Currently animation is linear(0.1) based on animations in the actual MediaScrubber, see if its possible to only use those animations while scrubbing
        //        withAnimation(.spring()) {
        currentTime = 500
        //        }
      } label: {
        Text("Go to 50%")
      }
    }
  }
}
// swift-format-ignore
@available(iOS 16.0, *)
#Preview {
  MediaScrubberPreview()
}
#endif
