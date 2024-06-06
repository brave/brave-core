// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import Data
import Foundation
import SwiftUI

extension PlaylistSheetDetent.DetentAnchorID {
  static let mediaPlayer: Self = .init(id: "player")
}

/// The view shown when the user is playing video or audio
struct MediaContentView: View {
  @ObservedObject var model: PlayerModel
  var selectedItem: PlaylistItem
  @Binding var selectedDetent: PlaylistSheetDetent

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
        .gesture(
          isFullScreen
            ? nil
            : TapGesture().onEnded {
              withAnimation(.snappy) {
                selectedDetent = .small
              }
            }
        )
      if !isFullScreen {
        PlaybackControlsView(model: model, selectedItemTitle: selectedItem.name)
          .padding(24)
      }
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: isFullScreen ? .center : .top)
    .background(isFullScreen ? .black : Color(braveSystemName: .containerBackground))
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

    var body: some View {
      VStack(spacing: 28) {
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
        // FIXME: Handle live video better
        PlaybackScrubber(model: model)
        VStack(spacing: 28) {
          HStack {
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
            Spacer()
            Button {
              Task { await model.seekBackwards() }
            } label: {
              Label("Step Back", braveSystemImage: "leo.rewind.15")
            }
            .buttonStyle(.playbackControl(size: .large))
            .tint(Color(braveSystemName: .textPrimary))
            Spacer()
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
            .tint(Color(braveSystemName: .textPrimary))
            .buttonStyle(.playbackControl(size: .extraLarge))
            Spacer()
            Button {
              Task { await model.seekForwards() }
            } label: {
              Label("Step Forward", braveSystemImage: "leo.forward.15")
            }
            .buttonStyle(.playbackControl(size: .large))
            .tint(Color(braveSystemName: .textPrimary))
            Spacer()
            Button {
              model.repeatMode.cycle()
            } label: {
              // FIXME: Better accessibility labels
              Group {
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
          .buttonStyle(.playbackControl)
          .tint(Color(braveSystemName: .textSecondary))
          HStack {
            Button {
              model.playbackSpeed.cycle()
            } label: {
              Label("Playback Speed", braveSystemImage: model.playbackSpeed.braveSystemName)
                .transition(.opacity.animation(.linear(duration: 0.1)))
            }
            Spacer()
            Menu {
              Section {
                let timeOptions = [10.minutes, 20.minutes, 30.minutes, 60.minutes]
                ForEach(timeOptions, id: \.self) { option in
                  Button {
                    withAnimation(.snappy) {
                      model.sleepTimerFireDate = .now.addingTimeInterval(option)
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
              } header: {
                Text("Stop Playback In…")
              }
              if model.sleepTimerFireDate != nil {
                Divider()
                Button("Cancel Timer") {
                  withAnimation(.snappy) {
                    model.sleepTimerFireDate = nil
                  }
                }
              }
            } label: {
              HStack {
                Label("Sleep Timer", braveSystemImage: "leo.sleep.timer")
                if let sleepTimerFireDate = model.sleepTimerFireDate {
                  Text(timerInterval: .now...sleepTimerFireDate, countsDown: true)
                    .font(.callout.weight(.semibold))
                    .transition(.opacity)
                }
              }
            }
            .tint(
              model.sleepTimerFireDate != nil
                ? Color(braveSystemName: .textInteractive) : Color(braveSystemName: .textSecondary)
            )
            Spacer()
            Button {
              withAnimation(.snappy(duration: 0.3, extraBounce: 0.1)) {
                toggleFullScreen()
              }
            } label: {
              Label("Fullscreen", braveSystemImage: "leo.fullscreen.on")
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
    ),
    selectedDetent: .constant(.small)
  )
  .environment(\.managedObjectContext, DataController.swiftUIContext)
  .preparePlaylistEnvironment()
  .environment(\.colorScheme, .dark)
}
#endif
