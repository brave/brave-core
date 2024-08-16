// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import Data
import Foundation
import Strings
import SwiftUI

extension PlaylistSheetDetent.DetentAnchorID {
  static let mediaPlayer: Self = .init(id: "player")
  static let mediaControls: Self = .init(id: "controls")
}

/// The view shown when the user is playing video or audio
struct MediaContentView: View {
  @ObservedObject var model: PlayerModel
  var selectedItem: PlaylistItem

  @Environment(\.interfaceOrientation) private var interfaceOrientation
  @Environment(\.isFullScreen) private var isFullScreen
  @Environment(\.toggleFullScreen) private var toggleFullScreen
  @Environment(\.requestGeometryUpdate) private var requestGeometryUpdate

  @State private var ignoreFollowingFullScreenOrientationChange: Bool = false

  var body: some View {
    VStack {
      PlayerView(playerModel: model)
        .zIndex(1)
        .playlistSheetDetentAnchor(id: .mediaPlayer)
        .overlay {
          if model.isLoadingStreamingURL {
            ProgressView()
              .progressViewStyle(.circular)
              .transition(.opacity.animation(.default))
          }
        }
        .overlay {
          if !isFullScreen {
            PlayerGestureShortcutsView(model: model)
          }
        }
      if !isFullScreen {
        PlaybackControlsView(model: model, selectedItemTitle: selectedItem.name)
          .padding(24)
          .playlistSheetDetentAnchor(id: .mediaControls)
      }
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: isFullScreen ? .center : .top)
    .background {
      // For some reason (likely a SwiftUI bug) we need to ensure that this background doesn't allow
      // hit testing otherwise it eats touches around the toolbar buttons and makes it harder to hit
      // the PiP button
      (isFullScreen ? .black : Color(braveSystemName: .containerBackground))
        .ignoresSafeArea()
        .allowsHitTesting(false)
    }
    .onChange(of: isFullScreen) { newValue in
      handleFullScreenOrientationChanges(
        expectedFullScreen: newValue,
        expectedOrientation: interfaceOrientation
      )
    }
    .onChange(of: interfaceOrientation) { newValue in
      handleFullScreenOrientationChanges(
        expectedFullScreen: isFullScreen,
        expectedOrientation: newValue
      )
    }
    .persistentSystemOverlays(isFullScreen ? .hidden : .automatic)
    .defersSystemGestures(on: isFullScreen ? .all : [])
    .ignoresSafeArea(.keyboard, edges: .bottom)
  }

  private func handleFullScreenOrientationChanges(
    expectedFullScreen: Bool,
    expectedOrientation: UIInterfaceOrientation
  ) {
    if UIDevice.current.userInterfaceIdiom != .phone {
      // Full screen and orientations don't affect iPads
      return
    }

    // Orientation and full screen mode are linked, in the sense that changing one may trigger a
    // change in the other so we need to make sure that we ignore the follow-up change that would
    // trigger this method.
    defer { ignoreFollowingFullScreenOrientationChange = false }
    if ignoreFollowingFullScreenOrientationChange {
      return
    }

    let isPortraitVideo = model.isPortraitVideo
    let isFullScreenModeChanging = expectedFullScreen != isFullScreen
    let isOrientationChanging = expectedOrientation != interfaceOrientation

    if !isFullScreenModeChanging && !isOrientationChanging {
      // Nothing to do
      return
    }

    switch (expectedFullScreen, expectedOrientation.isLandscape) {
    case (true, false):
      // When a user taps full screen mode we specifically want to shift into landscape orientation
      // Likewise if the user was in full screen mode already and is no longer in landscape
      // orientation we want to _exit_ full screen mode. This is all explicit to non-portrait video
      // since potrait video can display fullscreen without being in landscape
      if !isPortraitVideo {
        ignoreFollowingFullScreenOrientationChange = true
        if isFullScreenModeChanging {
          requestGeometryUpdate(orientation: .landscapeLeft)
        } else {  // isOrientationChanging
          toggleFullScreen(explicitFullScreenMode: false)
        }
      }
    case (false, true):
      // When a user is exiting full screen we always want to shift back to portrait orientation.
      // Likewise if the user is shifting into landscape orientation we want to enter full screen mode
      ignoreFollowingFullScreenOrientationChange = true
      if isFullScreenModeChanging {
        requestGeometryUpdate(orientation: .portrait)
      } else {  // isOrientationChanging
        toggleFullScreen(explicitFullScreenMode: true)
      }
    default:
      break
    }
  }

}

extension MediaContentView {
  /// A media scrubber that controls the current player
  ///
  /// This is a separate view since it is constantly updating the current time, so its better to
  /// keep that state isolated and only update this View when time changes instead of the entire
  /// PlaybackControlsView
  private struct PlaybackScrubber: View {
    @ObservedObject var model: PlayerModel

    @State private var currentTime: TimeInterval = 0
    @State private var isScrubbing: Bool = false
    @State private var resumePlayingAfterScrub: Bool = false

    var body: some View {
      MediaScrubber(
        currentTime: Binding(
          get: {
            if isScrubbing {
              return model.currentTime
            }
            return self.currentTime
          },
          set: { newValue in
            Task { await model.seek(to: newValue, accurately: true) }
          }
        ),
        duration: model.duration,
        isScrubbing: $isScrubbing
      )
      .tint(Color(braveSystemName: .iconInteractive))
      .onChange(of: isScrubbing) { newValue in
        if newValue {
          resumePlayingAfterScrub = model.isPlaying
          model.pause()
        } else {
          if resumePlayingAfterScrub {
            model.play()
          }
        }
      }
      .task(priority: .low) {
        self.currentTime = model.currentTime
        for await currentTime in model.currentTimeStream {
          self.currentTime = currentTime
        }
      }
    }
  }
  struct PlaybackControlsView: View {
    @ObservedObject var model: PlayerModel
    var selectedItemTitle: String

    @Environment(\.toggleFullScreen) private var toggleFullScreen
    @Environment(\.dynamicTypeSize) private var dynamicTypeSize

    private var verticalStackSpacing: CGFloat {
      dynamicTypeSize >= .xxxLarge ? 14 : 28
    }

    var body: some View {
      VStack(spacing: verticalStackSpacing) {
        HStack(alignment: .firstTextBaseline) {
          Text(selectedItemTitle)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .font(.headline)
            .frame(maxWidth: .infinity, alignment: .leading)
            .multilineTextAlignment(.leading)
            .lineLimit(1)
          if model.allowsExternalPlayback {
            RoutePickerView()
          }
        }
        PlaybackScrubber(model: model)
        VStack(spacing: verticalStackSpacing) {
          HStack {
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
            .labelStyle(.iconOnly)
            .toggleStyle(.button)
            Spacer()
            Button {
              Task { await model.seekBackwards() }
            } label: {
              Label(Strings.Playlist.accessibilityStepBack, braveSystemImage: "leo.rewind.15")
            }
            .buttonStyle(.playbackControl(size: .large))
            .tint(Color(braveSystemName: .textPrimary))
            Spacer()
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
            .tint(Color(braveSystemName: .textPrimary))
            .buttonStyle(.playbackControl(size: .extraLarge))
            Spacer()
            Button {
              Task { await model.seekForwards() }
            } label: {
              Label(Strings.Playlist.accessibilityStepForwards, braveSystemImage: "leo.forward.15")
            }
            .buttonStyle(.playbackControl(size: .large))
            .tint(Color(braveSystemName: .textPrimary))
            Spacer()
            RepeatModePicker(repeatMode: $model.repeatMode)
          }
          .buttonStyle(.playbackControl)
          .tint(Color(braveSystemName: .textSecondary))
          HStack {
            PlaybackSpeedPicker(playbackSpeed: $model.playbackSpeed)
              .disabled(model.duration.isIndefinite)
            Spacer()
            Menu {
              Section {
                let timeOptions = [10.minutes, 20.minutes, 30.minutes, 60.minutes]
                ForEach(timeOptions, id: \.self) { option in
                  Button {
                    withAnimation(.snappy) {
                      model.sleepTimerCondition = .date(.now.addingTimeInterval(option))
                    }
                  } label: {
                    Text(
                      Duration.seconds(option),
                      format: .units(
                        allowed: [.hours, .minutes],
                        width: .wide,
                        maximumUnitCount: 1
                      )
                    )
                  }
                }
                Button {
                  withAnimation(.snappy) {
                    model.sleepTimerCondition = .itemPlaybackCompletion
                  }
                } label: {
                  Text(Strings.Playlist.sleepTimerEndOfItem)
                }
              } header: {
                Text(Strings.Playlist.sleepTimerStopPlaybackAfter)
              }
              if model.sleepTimerCondition != nil {
                Divider()
                Button(Strings.Playlist.sleepTimerCancelTimer) {
                  withAnimation(.snappy) {
                    model.sleepTimerCondition = nil
                  }
                }
              }
            } label: {
              HStack {
                // FIXME: iOS 16 - Menu doesn't apply button styles
                Label(Strings.Playlist.sleepTimer, braveSystemImage: "leo.sleep.timer")
                  .labelStyle(.iconOnly)
                if case .date(let sleepTimerFireDate) = model.sleepTimerCondition {
                  Text(timerInterval: .now...sleepTimerFireDate, countsDown: true)
                    .font(.callout.weight(.semibold))
                    .transition(.opacity)
                }
              }
            }
            .menuOrder(.fixed)
            .tint(
              model.sleepTimerCondition != nil
                ? Color(braveSystemName: .textInteractive) : Color(braveSystemName: .textSecondary)
            )
            Spacer()
            Button {
              withAnimation(.snappy(duration: 0.3, extraBounce: 0.1)) {
                toggleFullScreen()
              }
            } label: {
              Label(
                Strings.Playlist.accessibilityEnterFullscreen,
                braveSystemImage: "leo.fullscreen.on"
              )
            }
          }
          .buttonStyle(.playbackControl)
          .tint(Color(braveSystemName: .textSecondary))
        }
        .font(.title3)
        .backgroundStyle(Color(braveSystemName: .containerHighlight))
      }
      // FIXME: Figure out what to do in AX sizes, maybe second row in PlaybackControls?
      // XXXL may even have issues with DisplayZoom on
      .dynamicTypeSize(.xSmall...DynamicTypeSize.xxxLarge)
    }
  }
}

#if DEBUG
// swift-format-ignore
#Preview {
  MediaContentView(
    model: .preview,
    selectedItem: .init(
      context: DataController.swiftUIContext,
      name: "Test",
      pageTitle: "Test",
      pageSrc: "https://youtube.com",
      cachedData: Data(),
      duration: 100,
      mimeType: "",
      mediaSrc: ""
    )
  )
  .environment(\.managedObjectContext, DataController.swiftUIContext)
  .preparePlaylistEnvironment()
  .environment(\.colorScheme, .dark)
}
#endif
