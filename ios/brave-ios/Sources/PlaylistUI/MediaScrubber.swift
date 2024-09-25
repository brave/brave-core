// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import CoreMedia
import Foundation
import Strings
import SwiftUI

/// A control which mimics a SwiftUI Slider but allows the user to change the current time of an
/// associated piece of media using gestures.
///
/// `MediaScrubber` displays a provided `Label` View below it to display the current values it is
/// displaying based on `currentTime` and `duration`. The default label is
/// `DefaultMediaScrubberLabel` if none is provided in the initializer. `DefaultMediaScrubberLabel`
struct MediaScrubber<Label: View>: View {
  @Binding var currentTime: TimeInterval
  var duration: PlayerModel.ItemDuration
  @Binding var isScrubbing: Bool
  var label: Label

  init(
    currentTime: Binding<TimeInterval>,
    duration: PlayerModel.ItemDuration,
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
  @Environment(\.layoutDirection) private var layoutDirection

  private var currentValueLabel: Text {
    return Text(.seconds(currentTime), format: .time(pattern: .minuteSecond))
  }

  @ViewBuilder private var remainingTimeLabel: some View {
    switch duration {
    case .unknown:
      EmptyView()
    case .seconds(let duration):
      Text(.seconds(currentTime - duration), format: .time(pattern: .minuteSecond))
    case .indefinite:
      Text(Strings.Playlist.liveIndicator)
    }
  }

  private var durationLabel: Text {
    if case .seconds(let duration) = duration {
      return Text(.seconds(duration), format: .time(pattern: .minuteSecond))
    }
    return Text("")
  }

  private var barShape: some InsettableShape {
    RoundedRectangle(cornerRadius: barHeight / 2, style: .continuous)
  }

  var body: some View {
    VStack {
      barShape
        .opacity(0.3)
        .foregroundStyle(.tint)
        .frame(height: barHeight)
        .overlay {
          if case .seconds(let duration) = duration {
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
                .osAvailabilityModifiers { content in
                  if #available(iOS 17.0, *) {
                    content.transaction(value: currentTime) { tx in
                      if tx.animation == nil && !tx.disablesAnimations {
                        tx.animation = .linear(duration: 0.1)
                      }
                    }
                  } else {
                    content.animation(.linear(duration: 0.1), value: currentTime)
                  }
                }
            }
          }
        }
        .padding(.vertical, (thumbSize - barHeight) / 2)
        .overlay {
          if case .seconds(let duration) = duration {
            // Thumb
            GeometryReader { proxy in
              Circle()
                .foregroundStyle(.tint)
                .frame(width: thumbSize, height: thumbSize)
                .contentShape(.circle.inset(by: -8))
                .contentShape(.hoverEffect, .circle.inset(by: -8))
                .hoverEffect(.lift)
                .scaleEffect(isScrubbing ? 1.5 : 1)
                .animation(.spring(response: 0.2, dampingFraction: 0.7), value: isScrubbing)
                .offset(
                  x: min(
                    proxy.size.width,
                    CGFloat(currentTime / duration) * proxy.size.width
                  ) - (thumbSize / 2)
                )
                .gesture(
                  DragGesture(minimumDistance: 0, coordinateSpace: .named("MediaScrubber"))
                    .updating(
                      $isScrubbingState,
                      body: { _, state, _ in
                        state = true
                      }
                    )
                    .onChanged { state in
                      var percent = state.location.x / proxy.size.width
                      if layoutDirection == .rightToLeft {
                        percent = 1 - percent
                      }
                      let seconds = max(
                        0,
                        min(
                          duration,
                          percent * CGFloat(duration)
                        )
                      )
                      currentTime = seconds
                    }
                )
                .osAvailabilityModifiers { content in
                  if #available(iOS 17.0, *) {
                    content.transaction(value: currentTime) { tx in
                      if tx.animation == nil && !tx.disablesAnimations {
                        tx.animation = .linear(duration: 0.1)
                      }
                    }
                  } else {
                    content.animation(.linear(duration: 0.1), value: currentTime)
                  }
                }
            }
          }
        }
        .disabled(duration.seconds == nil)
      label
    }
    .coordinateSpace(name: "MediaScrubber")
    .onChange(of: isScrubbingState) { newValue in
      isScrubbing = newValue
    }
    .accessibilityRepresentation {
      if case .seconds(let duration) = duration, duration > 0 {
        Slider(
          value: $currentTime,
          in: 0.0...duration,
          step: 1
        ) {
          Text(Strings.Playlist.accessibilityCurrentMediaTime)
        } minimumValueLabel: {
          currentValueLabel
        } maximumValueLabel: {
          durationLabel
        }
      }
    }
  }
}

extension MediaScrubber where Label == DefaultMediaScrubberLabel {
  init(
    currentTime: Binding<TimeInterval>,
    duration: PlayerModel.ItemDuration,
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

struct DefaultMediaScrubberLabel: View {
  var currentTime: TimeInterval
  var duration: PlayerModel.ItemDuration

  @State private var isShowingTotalTime: Bool = false

  var body: some View {
    HStack {
      Text(.seconds(currentTime), format: .time(pattern: .minuteSecond))
      Spacer()
      switch duration {
      case .unknown:
        EmptyView()
      case .seconds(let duration):
        Button {
          // Only allow swapping when there's a remaining time available
          isShowingTotalTime.toggle()
        } label: {
          Group {
            if isShowingTotalTime {
              Text(.seconds(duration), format: .time(pattern: .minuteSecond))
            } else {
              Text(.seconds(currentTime - duration), format: .time(pattern: .minuteSecond))
            }
          }
          .transition(.move(edge: .trailing).combined(with: .opacity))
        }
      case .indefinite:
        HStack(spacing: 4) {
          Image(systemName: "circlebadge.fill")
            .imageScale(.small)
            .foregroundStyle(Color(braveSystemName: .red50))
          Text(Strings.Playlist.liveIndicator)
        }
      }
    }
    .foregroundStyle(.primary)
    .font(.footnote)
  }
}

#if DEBUG
private struct MediaScrubberPreview: View {
  @State private var currentTime: TimeInterval = 0
  @State private var isScrubbing: Bool = false

  var body: some View {
    VStack {
      MediaScrubber(
        currentTime: $currentTime,
        duration: .seconds(1000),
        isScrubbing: $isScrubbing
      )
      .padding()
      MediaScrubber(
        currentTime: $currentTime,
        duration: .seconds(1000),
        isScrubbing: $isScrubbing
      )
      .tint(.red)
      .environment(\.colorScheme, .dark)
      .padding()
      .background(Color.black)
      .environment(\.layoutDirection, .rightToLeft)

      MediaScrubber(
        currentTime: $currentTime,
        duration: .indefinite,
        isScrubbing: $isScrubbing
      )
      .padding()

      Button {
        withAnimation(.snappy) {
          currentTime = 500
        }
      } label: {
        Text(verbatim: "Go to 50%")
      }
    }
  }
}

#Preview {
  MediaScrubberPreview()
}
#endif
