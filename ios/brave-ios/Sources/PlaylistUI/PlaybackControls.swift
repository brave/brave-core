// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings
import SwiftUI

/// A playback control which lets the user pick a playback speed, either by cycling through speeds
/// with taps or by long-pressing to see the list of options
struct PlaybackSpeedPicker: View {
  @Binding var playbackSpeed: PlayerModel.PlaybackSpeed

  var body: some View {
    Menu {
      Picker("", selection: $playbackSpeed) {
        ForEach(PlayerModel.PlaybackSpeed.supportedSpeeds) { speed in
          Text(verbatim: "\(speed.rate.formatted())×")
            .tag(speed)
        }
      }
    } label: {
      Label(
        Strings.Playlist.accessibilityPlaybackSpeed,
        braveSystemImage: playbackSpeed.braveSystemName
      )
      .transition(.opacity.animation(.linear(duration: 0.1)))
    } primaryAction: {
      playbackSpeed.cycle()
    }
  }
}

/// A playback control which lets the user pick a repeat mode, either by cycling through each mode
/// with taps or by long-pressing to see the list of options
struct RepeatModePicker: View {
  @Binding var repeatMode: PlayerModel.RepeatMode

  var body: some View {
    Menu {
      Picker("", selection: $repeatMode) {
        Label(Strings.Playlist.repeatModeOptionNone, braveSystemImage: "leo.loop.off")
          .tag(PlayerModel.RepeatMode.none)
        Label(Strings.Playlist.repeatModeOptionOne, braveSystemImage: "leo.loop.1")
          .tag(PlayerModel.RepeatMode.one)
        Label(Strings.Playlist.repeatModeOptionAll, braveSystemImage: "leo.loop.all")
          .tag(PlayerModel.RepeatMode.all)
      }
    } label: {
      Group {
        switch repeatMode {
        case .none:
          Label(Strings.Playlist.accessibilityRepeatModeOff, braveSystemImage: "leo.loop.off")
        case .one:
          Label(Strings.Playlist.accessibilityRepeatModeOne, braveSystemImage: "leo.loop.1")
        case .all:
          Label(Strings.Playlist.accessibilityRepeatModeAll, braveSystemImage: "leo.loop.all")
        }
      }
      .transition(.opacity.animation(.linear(duration: 0.1)))
    } primaryAction: {
      repeatMode.cycle()
    }
  }
}
