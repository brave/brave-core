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
@available(iOS 16.0, *)
struct MediaContentView: View {
  @ObservedObject var model: PlayerModel
  var selectedItem: PlaylistItem

  @Environment(\.interfaceOrientation) private var interfaceOrientation
  @Environment(\.isFullScreen) private var isFullScreen
  @Environment(\.toggleFullScreen) private var toggleFullScreen
  @Environment(\.requestGeometryUpdate) private var requestGeometryUpdate

  var body: some View {
    VStack {
      PlayerView(playerModel: model)
        .zIndex(1)
        .playlistSheetDetentAnchor(id: .mediaPlayer)
      if !isFullScreen {
        PlaybackControlsView(model: model, selectedItemTitle: selectedItem.name)
          .padding(24)
      }
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: isFullScreen ? .center : .top)
    .background(isFullScreen ? .black : Color(braveSystemName: .containerBackground))
    .onChange(of: isFullScreen) { newValue in
      // Automatically rotate the device orientation on iPhones when the video is not portrait
      if UIDevice.current.userInterfaceIdiom == .phone, !model.isPortraitVideo {
        requestGeometryUpdate(orientation: newValue ? .landscapeLeft : .portrait)
      }
    }
    .onChange(of: interfaceOrientation) { newValue in
      if UIDevice.current.userInterfaceIdiom == .phone {
        if newValue.isLandscape {
          // Always toggle fullscreen on landscape iPhone
          toggleFullScreen(explicitFullScreenMode: true)
        } else {
          toggleFullScreen(explicitFullScreenMode: model.isPortraitVideo)
        }
      }
    }
    .persistentSystemOverlays(isFullScreen ? .hidden : .automatic)
    .defersSystemGestures(on: isFullScreen ? .all : [])
    .ignoresSafeArea(.keyboard, edges: .bottom)
  }
}

@available(iOS 16.0, *)
extension MediaContentView {
  struct PlaybackControlsView: View {
    @ObservedObject var model: PlayerModel
    var selectedItemTitle: String

    @State private var currentTime: TimeInterval = 0
    @State private var isScrubbing: Bool = false
    @State private var resumePlayingAfterScrub: Bool = false

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
        MediaScrubber(
          currentTime: Binding(
            get: {
              if isScrubbing {
                return model.currentTime
              } else {
                return self.currentTime
              }
            },
            set: { newValue in
              Task { await model.seek(to: newValue, accurately: true) }
            }
          ),
          duration: model.duration,
          isScrubbing: $isScrubbing
        )
        .tint(Color(braveSystemName: .iconInteractive))
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
            .foregroundStyle(Color(braveSystemName: .textPrimary))
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
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .buttonStyle(.playbackControl(size: .extraLarge))
            Spacer()
            Button {
              Task { await model.seekForwards() }
            } label: {
              Label("Step Forward", braveSystemImage: "leo.forward.15")
            }
            .buttonStyle(.playbackControl(size: .large))
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            Spacer()
            Button {
              model.repeatMode.cycle()
            } label: {
              // FIXME: Switch to Label's for VoiceOver
              Group {
                switch model.repeatMode {
                case .none:
                  Image(braveSystemName: "leo.loop.off")
                case .one:
                  Image(braveSystemName: "leo.loop.1")
                case .all:
                  Image(braveSystemName: "leo.loop.all")
                }
              }
              .transition(.opacity.animation(.linear(duration: 0.1)))
            }
          }
          .buttonStyle(.playbackControl)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
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
                Button {
                  //                  stopPlaybackDate = .now.addingTimeInterval(10 * 60)
                } label: {
                  Text("10 minutes")
                }
                Button {
                  //                  stopPlaybackDate = .now.addingTimeInterval(20 * 60)
                } label: {
                  Text("20 minutes")
                }
                Button {
                  //                  stopPlaybackDate = .now.addingTimeInterval(30 * 60)
                } label: {
                  Text("30 minutes")
                }
                Button {
                  //                  stopPlaybackDate = .now.addingTimeInterval(60 * 60)
                } label: {
                  Text("1 hour")
                }
              } header: {
                Text("Stop Playback In…")
              }
            } label: {
              Label("Sleep Timer", braveSystemImage: "leo.sleep.timer")
            }
            Spacer()
            Button {
              withAnimation(.snappy(duration: 0.3)) {
                toggleFullScreen()
              }
            } label: {
              Label("Fullscreen", braveSystemImage: "leo.fullscreen.on")
            }
          }
          .buttonStyle(.playbackControl)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
        .font(.title3)
        .backgroundStyle(Color(braveSystemName: .containerHighlight))
      }
      // FIXME: Figure out what to do in AX sizes, maybe second row in PlaybackControls?
      // XXXL may even have issues with DisplayZoom on
      .dynamicTypeSize(.xSmall...DynamicTypeSize.xxxLarge)
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
      .task {
        for await currentTime in model.currentTimeStream {
          self.currentTime = currentTime
        }
      }
    }
  }
}
