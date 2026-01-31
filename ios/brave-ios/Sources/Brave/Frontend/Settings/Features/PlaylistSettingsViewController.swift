// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStrings
import BraveUI
import Playlist
import Preferences
import SwiftUI

struct PlaylistSettingsView: View {
  @ObservedObject private var enablePlaylistURLBarButton = Preferences.Playlist
    .enablePlaylistURLBarButton
  @ObservedObject private var enableLongPressAddToPlaylist = Preferences.Playlist
    .enableLongPressAddToPlaylist
  @ObservedObject private var firstLoadAutoPlay = Preferences.Playlist.firstLoadAutoPlay
  @ObservedObject private var autoDownloadVideo = Preferences.Playlist.autoDownloadVideo
  @ObservedObject private var playbackLeftOff = Preferences.Playlist.playbackLeftOff
  @State private var isResetConfirmationDialogPresented: Bool = false

  var body: some View {
    Form {
      Section {
        Toggle(Strings.PlayList.urlBarButtonOptionTitle, isOn: $enablePlaylistURLBarButton.value)
          .tint(Color(braveSystemName: .primary40))
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.PlayList.urlBarButtonOptionFooter)
      }
      Section {
        Toggle(
          Strings.PlayList.playlistLongPressSettingsOptionTitle,
          isOn: $enableLongPressAddToPlaylist.value
        )
        .tint(Color(braveSystemName: .primary40))
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.PlayList.playlistLongPressSettingsOptionFooterText)
      }
      Section {
        Toggle(Strings.PlayList.playlistAutoPlaySettingsOptionTitle, isOn: $firstLoadAutoPlay.value)
          .tint(Color(braveSystemName: .primary40))
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.PlayList.playlistAutoPlaySettingsOptionFooterText)
      }
      Section {
        Picker(Strings.PlayList.playlistAutoSaveSettingsTitle, selection: $autoDownloadVideo.value)
        {
          Text(Strings.PlayList.playlistAutoSaveOptionOn)
            .tag(PlayListDownloadType.on.rawValue)
          Text(Strings.PlayList.playlistAutoSaveOptionOff)
            .tag(PlayListDownloadType.off.rawValue)
          Text(Strings.PlayList.playlistAutoSaveOptionOnlyWifi)
            .tag(PlayListDownloadType.wifi.rawValue)
        }
        .pickerStyle(.navigationLink)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        VStack(alignment: .leading, spacing: 4) {
          Text(Strings.PlayList.playlistAutoSaveSettingsFooterText)
          Text(Strings.PlayList.playlistAutoSaveSettingsDescription)
        }
      }
      Section {
        Toggle(
          Strings.PlayList.playlistStartPlaybackSettingsOptionTitle,
          isOn: $playbackLeftOff.value
        )
        .tint(Color(braveSystemName: .primary40))
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        Text(Strings.PlayList.playlistStartPlaybackSettingsFooterText)
      }
      Section {
        Button {
          isResetConfirmationDialogPresented = true
        } label: {
          Text(Strings.PlayList.playlistResetAlertTitle)
            .foregroundStyle(Color(braveSystemName: .textInteractive))
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        .confirmationDialog(
          Strings.PlayList.playlistResetAlertTitle,
          isPresented: $isResetConfirmationDialogPresented,
          titleVisibility: .visible
        ) {
          Button(Strings.PlayList.playlistResetAlertTitle, role: .destructive) {
            Task {
              await PlaylistManager.shared.deleteAllItems(cacheOnly: false)
            }
          }
          Button(Strings.CancelString, role: .cancel) {}
        } message: {
          Text(Strings.PlayList.playlistResetPlaylistOptionFooterText)
        }
      } footer: {
        Text(Strings.PlayList.playlistResetPlaylistOptionFooterText)
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
  }
}

class PlaylistSettingsViewController: UIHostingController<PlaylistSettingsView> {
  init() {
    super.init(rootView: PlaylistSettingsView())
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = Strings.PlayList.playListTitle
  }
}
