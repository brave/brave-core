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
  @GestureState private var isTouchingInlineControls: Bool = false

  @Environment(\.interfaceOrientation) private var orientation
  @Environment(\.isFullScreen) private var isFullScreen
  @Environment(\.toggleFullScreen) private var toggleFullScreen

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

  private var isVideoAmbianceBackgroundEnabled: Bool {
    !isFullScreen && playerModel.isPlayerInForeground
      && !ProcessInfo.processInfo.isLowPowerModeEnabled
  }

  var body: some View {
    VideoPlayerLayout(aspectRatio: isFullScreen ? nil : 16 / 9) {
      VideoPlayer(playerLayer: playerModel.playerLayer)
    }
    .background {
      if isVideoAmbianceBackgroundEnabled {
        VideoAmbianceBackground(playerModel: playerModel)
          .transition(.opacity.animation(.snappy))
          .opacity(playerModel.isPlaying ? 1 : 0.5)
          .animation(.default, value: playerModel.isPlaying)
      }
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
              .simultaneousGesture(
                DragGesture()
                  .onChanged { value in
                    dragOffset = value.translation
                  }
                  .onEnded { value in
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
          PlaybackSpeedPicker(playbackSpeed: $model.playbackSpeed)
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
          RepeatModePicker(repeatMode: $model.repeatMode)
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

struct VideoAmbianceBackground: View {
  var playerModel: PlayerModel

  @State private var videoAmbianceDecorationImage: UIImage?

  var body: some View {
    VStack {
      if let videoAmbianceDecorationImage {
        Image(uiImage: videoAmbianceDecorationImage)
          .resizable()
          .blur(radius: 30)
          .id(videoAmbianceDecorationImage)
          .transition(.opacity)
      }
    }
    .task(priority: .medium) {
      for await image in playerModel.videoAmbianceImageStream {
        withAnimation {
          videoAmbianceDecorationImage = image.size == .zero ? nil : image
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
