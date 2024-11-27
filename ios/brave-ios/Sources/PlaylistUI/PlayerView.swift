// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import DesignSystem
import Foundation
import Strings
import SwiftUI

/// This view displays the main video player along with controls on top of it when the player is
/// being displayed full screen.
struct PlayerView: View {
  @ObservedObject var playerModel: PlayerModel

  @State private var isControlsVisible: Bool = false
  @State private var autoHideControlsTask: Task<Void, Error>?
  @State private var dragOffset: CGSize = .zero
  @State private var activeFullScreenGestureMode: ActiveFullScreenGestureMode = .none
  @GestureState private var isTouchingInlineControls: Bool = false

  enum ActiveFullScreenGestureMode {
    case none
    case seekingVideo(startingTime: TimeInterval, playAfterCompletion: Bool)
    case dismissFullScreen
  }

  @Environment(\.interfaceOrientation) private var orientation
  @Environment(\.isFullScreen) private var isFullScreen
  @Environment(\.toggleFullScreen) private var toggleFullScreen
  @Environment(\.layoutDirection) private var layoutDirection

  /// A 0-distance DragGesture to tracks if the user is touching the screen in any way
  private var activeTouchGesture: some Gesture {
    DragGesture(minimumDistance: 0)
      .updating($isTouchingInlineControls, body: { _, state, _ in state = true })
  }

  private func hideControlsAfterDelay() {
    autoHideControlsTask = Task {
      try await Task.sleep(for: .seconds(3))
      withAnimation(.linear(duration: 0.1)) {
        isControlsVisible = false
      }
    }
  }

  private func fullScreenGesture(proxy: GeometryProxy) -> some Gesture {
    DragGesture()
      .onChanged { value in
        switch activeFullScreenGestureMode {
        case .none:
          // Check velocity to see which mode to enter
          if abs(value.velocity.height) > abs(value.velocity.width) {
            activeFullScreenGestureMode = .dismissFullScreen
          } else {
            if case .seconds = playerModel.duration {
              activeFullScreenGestureMode = .seekingVideo(
                startingTime: playerModel.currentTime,
                playAfterCompletion: playerModel.isPlaying
              )
              playerModel.pause()
            }
          }
          break
        case .seekingVideo(let startingTime, _):
          if case .seconds(let seconds) = playerModel.duration, proxy.size.width > 0 {
            var timeTranslation = (value.translation.width * (seconds / proxy.size.width))
            if layoutDirection == .rightToLeft {
              timeTranslation *= -1
            }
            Task {
              await playerModel.seek(to: startingTime + timeTranslation, accurately: true)
            }
          }
        case .dismissFullScreen:
          dragOffset = value.translation
        }
      }
      .onEnded { value in
        switch activeFullScreenGestureMode {
        case .none:
          break
        case .seekingVideo(_, let playAfterCompletion):
          if playAfterCompletion {
            playerModel.play()
          }
        case .dismissFullScreen:
          let finalOffset = value.predictedEndTranslation
          if abs(finalOffset.height) > 200 {
            withAnimation(.snappy(duration: 0.3)) {
              toggleFullScreen()
            }
          }
          withAnimation(.snappy) {
            dragOffset = .zero
          }
        }
        activeFullScreenGestureMode = .none
      }
  }

  var body: some View {
    VideoPlayerLayout(aspectRatio: isFullScreen ? nil : 16 / 9) {
      VideoPlayer(playerLayer: playerModel.playerLayer)
    }
    .allowsHitTesting(false)
    // For some reason this is required or the status bar breaks when touching anything on the
    // screen on an iPad...
    .statusBarHidden(isFullScreen && !isControlsVisible)
    .offset(x: isFullScreen ? dragOffset.width : 0, y: isFullScreen ? dragOffset.height : 0.0)
    .scaleEffect(
      x: isFullScreen ? 1 - (abs(dragOffset.height) / 1000) : 1,
      y: isFullScreen ? 1 - (abs(dragOffset.height) / 1000) : 1,
      anchor: .center
    )
    .frame(maxWidth: .infinity, maxHeight: isFullScreen ? .infinity : nil)
    .ignoresSafeArea(
      isFullScreen && (orientation.isLandscape || !playerModel.isPortraitVideo) ? .all : [],
      edges: .vertical
    )
    .accessibilityLabel(isFullScreen ? Strings.Playlist.accessibilityTapToToggleControls : "")
    .accessibilityAddTraits(isFullScreen ? .isButton : [])
    .overlay {
      InlinePlaybackControlsView(model: playerModel)
        .background(
          Color.black.opacity(0.3)
            .allowsHitTesting(false)
            .ignoresSafeArea()
        )
        .simultaneousGesture(activeTouchGesture)
        .opacity(isControlsVisible && isFullScreen ? 1 : 0)
        .accessibilityHidden(isControlsVisible && isFullScreen)
        .background {
          if isFullScreen {
            GeometryReader { proxy in
              Color.clear
                .contentShape(.rect)
                .simultaneousGesture(
                  TapGesture().onEnded { _ in
                    withAnimation(.linear(duration: 0.1)) {
                      isControlsVisible.toggle()
                    }
                  }
                )
                .simultaneousGesture(activeTouchGesture)
                .simultaneousGesture(fullScreenGesture(proxy: proxy))
            }
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
      if newValue {
        hideControlsAfterDelay()
      }
    }
    .onChange(of: isTouchingInlineControls) { newValue in
      if !isControlsVisible {
        return
      }
      if newValue {
        autoHideControlsTask?.cancel()
      } else {
        hideControlsAfterDelay()
      }
    }
  }
}

extension PlayerView {
  /// Controls shown inside of the PlayerView, typically in fullscreen mode
  struct InlinePlaybackControlsView: View {
    @Environment(\.toggleFullScreen) private var toggleFullScreen
    @State private var currentTime: TimeInterval = 0
    @State private var isScrubbing: Bool = false
    @State private var resumePlayingAfterScrub: Bool = false

    @ObservedObject var model: PlayerModel

    var body: some View {
      VStack {
        HStack(spacing: 16) {
          if #available(iOS 18.0, *) {
            // iOS 18 breaks simultaneous gestures on `Menu`, so we'll swap in a the old Button
            // instead for those users for the time being...
            Button {
              model.playbackSpeed.cycle()
            } label: {
              Label(
                Strings.Playlist.accessibilityPlaybackSpeed,
                braveSystemImage: model.playbackSpeed.braveSystemName
              )
              .transition(.opacity.animation(.linear(duration: 0.1)))
              .contentShape(.rect)
            }
          } else {
            PlaybackSpeedPicker(playbackSpeed: $model.playbackSpeed)
          }
          Spacer()
          Toggle(isOn: $model.isShuffleEnabled) {
            if model.isShuffleEnabled {
              Label(
                Strings.Playlist.accessibilityShuffleModeOn,
                braveSystemImage: "leo.shuffle.toggle-on"
              )
              .transition(.opacity.animation(.linear(duration: 0.1)))
            } else {
              Label(
                Strings.Playlist.accessibilityShuffleModeOff,
                braveSystemImage: "leo.shuffle.off"
              )
              .transition(.opacity.animation(.linear(duration: 0.1)))
            }
          }
          .toggleStyle(.button)
          Group {
            if #available(iOS 18.0, *) {
              // iOS 18 breaks simultaneous gestures on `Menu`, so we'll swap in a the old Button
              // instead for those users for the time being...
              Button {
                model.repeatMode.cycle()
              } label: {
                Group {
                  switch model.repeatMode {
                  case .none:
                    Label(
                      Strings.Playlist.accessibilityRepeatModeOff,
                      braveSystemImage: "leo.loop.off"
                    )
                  case .one:
                    Label(
                      Strings.Playlist.accessibilityRepeatModeOne,
                      braveSystemImage: "leo.loop.1"
                    )
                  case .all:
                    Label(
                      Strings.Playlist.accessibilityRepeatModeAll,
                      braveSystemImage: "leo.loop.all"
                    )
                  }
                }
                .transition(.opacity.animation(.linear(duration: 0.1)))
              }
            } else {
              RepeatModePicker(repeatMode: $model.repeatMode)
            }
          }
          .disabled(model.duration.isIndefinite)
          Button {
            Task {
              await model.playNextItem()
            }
          } label: {
            Label(Strings.Playlist.accessibilityNextItem, braveSystemImage: "leo.next.outline")
          }
          .disabled(!model.canPlayNextItem)
        }
        Spacer()
        VStack {
          MediaScrubber(
            currentTime: Binding(
              get: {
                if isScrubbing {
                  return model.currentTime
                }
                return currentTime
              },
              set: { newValue in
                Task { await model.seek(to: newValue, accurately: true) }
              }
            ),
            duration: model.duration,
            isScrubbing: $isScrubbing
          ) {
            HStack {
              CompactMediaScrubberLabel(
                currentTime: currentTime,
                duration: model.duration
              )
              Spacer()
              Button {
                withAnimation(.snappy(duration: 0.3)) {
                  toggleFullScreen()
                }
              } label: {
                Label(
                  Strings.Playlist.accessibilityExitFullscreen,
                  braveSystemImage: "leo.fullscreen.off"
                )
              }
              .tint(Color(braveSystemName: .textPrimary))
            }
            .buttonStyle(.playbackControl)
          }
          .contentShape(.rect)
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
            Label(Strings.Playlist.accessibilityStepBack, braveSystemImage: "leo.rewind.15")
          }
          .buttonStyle(.playbackControl(size: .large))
          Toggle(isOn: $model.isPlaying) {
            if model.isPlaying {
              Label(Strings.Playlist.accessibilityPause, braveSystemImage: "leo.pause.filled")
                .transition(.playButtonTransition)
            } else {
              Label(Strings.Playlist.accessibilityPlay, braveSystemImage: "leo.play.filled")
                .transition(.playButtonTransition)
            }
          }
          .toggleStyle(.button)
          .accessibilityAddTraits(!model.isPlaying ? .startsMediaSession : [])
          .buttonStyle(.playbackControl(size: .extraLarge))
          Button {
            Task { await model.seekForwards() }
          } label: {
            Label(Strings.Playlist.accessibilityStepForwards, braveSystemImage: "leo.forward.15")
          }
          .buttonStyle(.playbackControl(size: .large))
        }
        .ignoresSafeArea(edges: .vertical)
      }
      .buttonStyle(.playbackControl)
      .tint(Color(braveSystemName: .textPrimary))
      .backgroundStyle(Color.white.opacity(0.2))
      .task(id: model.isPlayerInForeground, priority: .low) {
        if !model.isPlayerInForeground { return }
        self.currentTime = model.currentTime
        for await currentTime in model.currentTimeStream {
          self.currentTime = currentTime
        }
      }
    }
  }
}

struct CompactMediaScrubberLabel: View {
  var currentTime: TimeInterval
  var duration: PlayerModel.ItemDuration

  private var currentValueLabel: Text {
    return Text(.seconds(currentTime), format: .time(pattern: .minuteSecond))
  }

  var body: some View {
    HStack {
      currentValueLabel
      if case .seconds(let duration) = duration {
        Text(verbatim: "/")  // FIXME: Does this need some sort of localization?
        Text(.seconds(currentTime - duration), format: .time(pattern: .minuteSecond))
      }
    }
    .font(.caption2)
  }
}

/// A custom Layout which accepts an optional aspect ratio to use when sizing the VideoPlayer.
///
/// If an aspect ratio is supplied then the underlying VideoPlayer will attempt to size itself to
/// fill the width of the parent while maintaining that aspect ratio. In the case where there is not
/// enough vertical space to maintain the supplied aspect ratio, the height will be lowered and
/// the aspect ratio may not be respected and black bars may show up on the video.
///
/// If no aspect ratio is supplied then the underlying VideoPlayer will simply fill the entire
/// available space like it would if no layout was used.
///
/// This is neccessary because there is no way to conditionally apply an `aspectRatio` modifier to
/// a `View`
private struct VideoPlayerLayout: Layout {
  var aspectRatio: CGFloat?

  func sizeThatFits(
    proposal: ProposedViewSize,
    subviews: Subviews,
    cache: inout ()
  ) -> CGSize {
    assert(subviews.count == 1)
    if let aspectRatio {
      let proposedSize = proposal.replacingUnspecifiedDimensions()
      return .init(
        width: proposedSize.width,
        height: min(proposedSize.height, proposedSize.width / aspectRatio)
      )
    }
    return subviews[0].sizeThatFits(proposal)
  }

  func placeSubviews(
    in bounds: CGRect,
    proposal: ProposedViewSize,
    subviews: Subviews,
    cache: inout ()
  ) {
    subviews[0].place(at: bounds.origin, proposal: proposal)
  }
}

private struct VideoPlayer: UIViewRepresentable {
  var playerLayer: AVPlayerLayer

  func makeUIView(context: Context) -> PlayerView {
    PlayerView(playerLayer: playerLayer)
  }

  func updateUIView(_ uiView: PlayerView, context: Context) {
  }

  func sizeThatFits(_ proposal: ProposedViewSize, uiView: PlayerView, context: Context) -> CGSize? {
    proposal.replacingUnspecifiedDimensions()
  }

  class PlayerView: UIView {
    var playerLayer: AVPlayerLayer

    init(playerLayer: AVPlayerLayer) {
      self.playerLayer = playerLayer
      super.init(frame: .zero)
      layer.addSublayer(playerLayer)
      backgroundColor = .black
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }

    override func layoutSubviews() {
      super.layoutSubviews()
      CATransaction.setDisableActions(true)
      playerLayer.frame = bounds
      CATransaction.setDisableActions(false)
    }
  }
}
