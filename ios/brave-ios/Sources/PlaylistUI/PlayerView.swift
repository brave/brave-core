// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import DesignSystem
import Foundation
import SwiftUI

// FIXME: Add doc
@available(iOS 16.0, *)
struct PlayerView: View {
  @ObservedObject var playerModel: PlayerModel

  @State private var isControlsVisible: Bool = false
  @State private var autoHideControlsTask: Task<Void, Error>?
  @State private var dragOffset: CGSize = .zero

  @Environment(\.isFullScreen) private var isFullScreen
  @Environment(\.toggleFullScreen) private var toggleFullScreen

  var body: some View {
    // FIXME: Will likely need a true AVPlayerLayer representable
    VideoPlayer(player: playerModel.player)
      .disabled(true)
      // For some reason this is required or the status bar breaks when touching anything on the
      // screen on an iPad...
      .statusBarHidden(isFullScreen && !isControlsVisible)
      .aspectRatio(isFullScreen ? playerModel.aspectRatio : 16 / 9, contentMode: .fit)
      .offset(x: isFullScreen ? dragOffset.width : 0, y: isFullScreen ? dragOffset.height : 0.0)
      .scaleEffect(
        x: isFullScreen ? 1 - (abs(dragOffset.height) / 1000) : 1,
        y: isFullScreen ? 1 - (abs(dragOffset.height) / 1000) : 1,
        anchor: .center
      )
      .frame(maxWidth: isFullScreen ? .infinity : nil, maxHeight: isFullScreen ? .infinity : nil)
      .ignoresSafeArea(isFullScreen ? .all : [], edges: .vertical)
      // FIXME: Better accessibility copy
      .accessibilityLabel(isFullScreen ? "Tap to toggle controls" : "Media player")
      .accessibilityAddTraits(isFullScreen ? .isButton : [])
      .overlay {
        InlinePlaybackControlsView(model: playerModel)
          .background(
            Color.black.opacity(0.3)
              .allowsHitTesting(false)
              .ignoresSafeArea()
          )
          .opacity(isControlsVisible && isFullScreen ? 1 : 0)
          .accessibilityHidden(isControlsVisible && isFullScreen)
          .background {
            if isFullScreen {
              Color.clear
                .contentShape(.rect)
                .simultaneousGesture(
                  TapGesture().onEnded { _ in
                    withAnimation(.linear(duration: 0.1)) {
                      isControlsVisible.toggle()
                    }
                  }
                )
                .simultaneousGesture(
                  DragGesture()
                    .onChanged { value in
                      dragOffset = value.translation
                    }
                    .onEnded { value in
                      let finalOffset = value.predictedEndTranslation
                      if abs(finalOffset.height) > 200 {
                        withAnimation(.snappy) {
                          toggleFullScreen()
                        }
                      }
                      withAnimation(.snappy) {
                        dragOffset = .zero
                      }
                    }
                )
            }
          }
      }
      .onChange(of: isFullScreen) { newValue in
        // Delay showing controls until the animation to present full screen is done, unfortunately
        // there's no way of determining that in SwiftUI, so will just have to wait an arbitrary
        // amount of time.
        withAnimation(.linear(duration: 0.1).delay(newValue ? 0.25 : 0.0)) {
          isControlsVisible = newValue
        }
      }
      .onChange(of: isControlsVisible) { newValue in
        // FIXME: We should cancel hiding when the users finger is touching the screen
        autoHideControlsTask?.cancel()
        if newValue {
          autoHideControlsTask = Task {
            try await Task.sleep(for: .seconds(3))
            withAnimation(.linear(duration: 0.1)) {
              isControlsVisible = false
            }
          }
        }
      }
  }
}

@available(iOS 16.0, *)
extension PlayerView {
  /// Controls shown inside of the PlayerView, typically in fullscreen mode
  // FIXME: Find a way to share the actual control buttons?
  // FIXME: Background is solid when tapped
  struct InlinePlaybackControlsView: View {
    @Environment(\.toggleFullScreen) private var toggleFullScreen
    @State private var currentTime: TimeInterval = 0
    @State private var isScrubbing: Bool = false
    @State private var resumePlayingAfterScrub: Bool = false

    @ObservedObject var model: PlayerModel

    var body: some View {
      VStack {
        HStack(spacing: 16) {
          Button {
            model.playbackSpeed.cycle()
          } label: {
            Label("Playback Speed", braveSystemImage: model.playbackSpeed.braveSystemName)
              .transition(.opacity.animation(.linear(duration: 0.1)))
          }
          Spacer()
          Toggle(isOn: $model.isShuffleEnabled) {
            if model.isShuffleEnabled {
              Image(braveSystemName: "leo.shuffle.toggle-on")
                .transition(.opacity.animation(.linear(duration: 0.1)))
            } else {
              Image(braveSystemName: "leo.shuffle.off")
                .transition(.opacity.animation(.linear(duration: 0.1)))
            }
          }
          .toggleStyle(.button)
          Button {
            model.repeatMode.cycle()
          } label: {
            Group {
              // FIXME: Better accessibility labels
              switch model.repeatMode {
              case .none:
                Label("Repeat Mode: Off", braveSystemImage: "leo.loop.off")
              case .one:
                Label("Repeat Mode: One", braveSystemImage: "leo.loop.1")
              case .all:
                Label("Repeat Mode: All", braveSystemImage: "leo.loop.all")
              }
            }
            .transition(.opacity.animation(.linear(duration: 0.1)))
          }
        }
        Spacer()
        VStack {
          MediaScrubber(
            currentTime: Binding(
              get: { .seconds(currentTime) },
              set: { newValue in
                Task { await model.seek(to: TimeInterval(newValue.components.seconds)) }
              }
            ),
            duration: .seconds(model.duration),
            isScrubbing: $isScrubbing
          ) {
            HStack {
              CompactMediaScrubberLabel(
                currentTime: .seconds(currentTime),
                duration: .seconds(model.duration)
              )
              Spacer()
              Button {
                withAnimation {
                  toggleFullScreen()
                }
              } label: {
                Label("Exit Fullscreen", braveSystemImage: "leo.fullscreen.off")
              }
            }
            .buttonStyle(.playbackControl)
          }
          .tint(Color(braveSystemName: .iconInteractive))
        }
      }
      .padding(12)
      .overlay {
        // Always center the play/pause buttons
        HStack(spacing: 42) {
          Button {
            Task { await model.seekBackwards() }
          } label: {
            Label("Step Back", braveSystemImage: "leo.rewind.15")
          }
          .buttonStyle(.playbackControl(size: .large))
          Toggle(isOn: $model.isPlaying) {
            if model.isPlaying {
              Label("Pause", braveSystemImage: "leo.pause.filled")
                .transition(.playButtonTransition)
            } else {
              Label("Play", braveSystemImage: "leo.play.filled")
                .transition(.playButtonTransition)
            }
          }
          .toggleStyle(.button)
          .accessibilityAddTraits(!model.isPlaying ? .startsMediaSession : [])
          .buttonStyle(.playbackControl(size: .extraLarge))
          Button {
            Task { await model.seekForwards() }
          } label: {
            Label("Step Forward", braveSystemImage: "leo.forward.15")
          }
          .buttonStyle(.playbackControl(size: .large))
        }
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .ignoresSafeArea(edges: .vertical)
      }
      .buttonStyle(.playbackControl)
      // FIXME: Likely need to tie this to the ID of the current item via `task(id:_:)`
      .task {
        for await currentTime in model.currentTimeStream {
          self.currentTime = currentTime
        }
      }
    }
  }
}

@available(iOS 16.0, *)
struct CompactMediaScrubberLabel: View {
  var currentTime: Duration
  var duration: Duration

  private var currentValueLabel: Text {
    return Text(currentTime, format: .time(pattern: .minuteSecond))
  }

  private var remainingTimeLabel: Text {
    let value = Text(duration - currentTime, format: .time(pattern: .minuteSecond))
    return Text("-\(value)")
  }

  var body: some View {
    HStack {
      currentValueLabel
      Text("/")
      remainingTimeLabel
    }
    .font(.caption2)
  }
}
