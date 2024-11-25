// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import Data
import Foundation
import Playlist
import Preferences
import Strings
import SwiftUI

struct PlaylistContentView: View {
  @ObservedObject var playerModel: PlayerModel

  @Environment(\.dismiss) private var dismiss
  @Environment(\.isFullScreen) private var isFullScreen
  @Environment(\.toggleFullScreen) private var toggleFullScreen

  @FetchRequest(
    sortDescriptors: [
      .init(keyPath: \PlaylistFolder.order, ascending: true),
      .init(keyPath: \PlaylistFolder.dateAdded, ascending: false),
    ],
    predicate: .init(format: "sharedFolderUrl == nil")
  ) private var folders: FetchedResults<PlaylistFolder>

  @State private var selectedDetent: PlaylistSheetDetent = .anchor(.mediaControls)
  @State private var isNewPlaylistAlertPresented: Bool = false
  @State private var isEditModePresented: Bool = false
  @State private var newPlaylistName: String = ""
  @State private var isPopulatingNewPlaylist: Bool = false

  private var selectedItemID: PlaylistItem.ID? {
    playerModel.selectedItemID
  }

  private var selectedFolderID: PlaylistFolder.ID {
    playerModel.selectedFolderID
  }

  private var selectedFolder: PlaylistFolder? {
    folders.first(where: { $0.id == playerModel.selectedFolderID })
  }

  private var selectedItem: PlaylistItem? {
    playerModel.selectedItemID.flatMap { PlaylistItem.getItem(id: $0) }
  }

  public var body: some View {
    PlaylistSplitView(selectedDetent: $selectedDetent) {
      PlaylistSidebarList(
        folders: Array(folders),
        folderID: selectedFolderID,
        selectedItemID: Binding(
          get: {
            selectedItemID
          },
          set: { newValue in
            // Make the item queue prior to setting the `selectedItemID`, which will start playing
            // said item
            withAnimation(.snappy) {
              selectedDetent = .anchor(.mediaControls)
            }
            // FIXME: Move this into PlayerModel
            playerModel.makeItemQueue(selectedItemID: newValue)
            if selectedItemID == newValue {
              // Already selected, restart it (based on prior behaviour)
              Task {
                await playerModel.seek(to: 0, accurately: true)
                playerModel.play()
              }
              return
            }
            playerModel.selectedItemID = newValue
            Task {
              await playerModel.prepareToPlaySelectedItem(initialOffset: nil, playImmediately: true)
            }
          }
        ),
        isPlaying: playerModel.isPlaying,
        onPlaylistUpdated: {
          // Update the queue to reflect updated playlist
          playerModel.makeItemQueue(selectedItemID: selectedItemID)
        }
      )
    } sidebarHeader: {
      if let selectedFolder = selectedFolder {
        PlaylistSidebarListHeader(
          folders: Array(folders),
          selectedFolder: Binding(
            get: {
              selectedFolder
            },
            set: {
              playerModel.selectedFolderID = $0.id
            }
          ),
          selectedItemID: selectedItemID,
          isPlaying: Binding(
            get: {
              playerModel.isPlaying
            },
            set: { newValue in
              if newValue {
                if selectedItemID == nil {
                  playerModel.selectedItemID = playerModel.itemQueue.first
                } else {
                  playerModel.play()
                }
              } else {
                playerModel.pause()
              }
            }
          ),
          isNewPlaylistAlertPresented: $isNewPlaylistAlertPresented,
          isEditModePresented: $isEditModePresented
        )
      }
    } content: {
      ZStack {
        if let selectedItem {
          MediaContentView(
            model: playerModel,
            selectedItem: selectedItem
          )
          .playlistSheetDetents([.anchor(.mediaControls), .anchor(.mediaPlayer), .large])
        } else {
          PlaylistContentUnavailableView(isPlaylistEmpty: PlaylistItem.count() == 0)
            .playlistSheetDetents([.anchor(.emptyPlaylistContent), .large])
        }
      }
    } toolbar: {
      Button(Strings.Playlist.doneButtonTitle) {
        dismiss()
      }
      .fontWeight(.semibold)
      Spacer()
      if playerModel.isPictureInPictureSupported {
        Button {
          playerModel.startPictureInPicture()
          dismiss()
        } label: {
          Label(
            Strings.Playlist.accessibilityEnterPictureInPicture,
            braveSystemImage: "leo.picture.in-picture"
          )
        }
        .labelStyle(.iconOnly)
        .transition(.opacity.animation(.default))
      }
    }
    .task {
      await playerModel.prepareItemQueue()
      if selectedItem != nil {
        selectedDetent = .anchor(.mediaControls)
      } else {
        selectedDetent = .anchor(.emptyPlaylistContent)
      }
    }
    .onDisappear {
      if let selectedItem {
        PlaylistManager.shared.updateLastPlayed(
          item: .init(item: selectedItem),
          playTime: playerModel.currentTime
        )
      }
    }
    .sheet(isPresented: $isEditModePresented) {
      if let selectedFolder {
        EditFolderView(folder: selectedFolder, folders: Array(folders)) {
          playerModel.makeItemQueue(selectedItemID: playerModel.selectedItemID)
        }
      }
    }
    .alert(Strings.Playlist.newPlaylistButtonTitle, isPresented: $isNewPlaylistAlertPresented) {
      TextField(Strings.Playlist.newPlaylistPlaceholder, text: $newPlaylistName)
      Button(role: .cancel) {
        newPlaylistName = ""
      } label: {
        Text(Strings.CancelString)
      }
      .keyboardShortcut(.cancelAction)
      Button {
        defer { newPlaylistName = "" }
        if newPlaylistName.isEmpty {
          // See comment below about .disabled modifier
          return
        }
        PlaylistFolder.addFolder(title: newPlaylistName) { uuid in
          playerModel.selectedFolderID = uuid
          if let defaultFolderItems = PlaylistFolder.getFolder(
            uuid: PlaylistFolder.savedFolderUUID
          )?.playlistItems, !defaultFolderItems.isEmpty {
            isPopulatingNewPlaylist = true
          }
        }
      } label: {
        Text(Strings.Playlist.createNewPlaylistButtonTitle)
      }
      .keyboardShortcut(.defaultAction)
      // Unfortunately we can't disable this button due to _many_ SwiftUI bugs.
      // - On iOS 16, the button simply does not get added to the alert at all.
      // - On iOS 17 (tested to 17.4), the button shows up but tapping it does not execute the
      //   action even when the button is enabled.
      // .disabled(newPlaylistName.isEmpty)
    }
    .alert(
      isPresented: playerModel.isErrorAlertPresented,
      error: playerModel.error,
      actions: { error in
        Button {
          error.handler?()
        } label: {
          Text(Strings.OKString)
        }
      },
      message: { error in
        if let reason = error.failureReason {
          Text(reason)
        }
      }
    )
    .sheet(isPresented: $isPopulatingNewPlaylist) {
      PopulateNewPlaylistView(destinationFolder: selectedFolderID)
        .presentationDetents([.medium, .large])
        .environment(\.colorScheme, .dark)
        .preferredColorScheme(.dark)
    }
    .onChange(of: selectedItemID) { newValue in
      if newValue == nil {
        withAnimation(.snappy) {
          selectedDetent = .anchor(.emptyPlaylistContent)
        }
        playerModel.stop()
        toggleFullScreen(explicitFullScreenMode: false)
        return
      }
    }
    .onChange(of: Array(folders)) { newValue in
      if !newValue.map(\.id).contains(selectedFolderID) {
        // Reset the selected folder if the user deletes the folder they had selected
        playerModel.selectedFolderID = PlaylistFolder.savedFolderUUID
      }
    }
  }
}

extension PlaylistSheetDetent.DetentAnchorID {
  static let emptyPlaylistContent: Self = .init(id: "empty-playlist")
}

/// Shown when the use has no playlist item selected
struct PlaylistContentUnavailableView: View {
  var isPlaylistEmpty: Bool

  var body: some View {
    VStack(spacing: 24) {
      Image(.emptyPlaylist)
      Text(
        isPlaylistEmpty
          ? Strings.Playlist.noMediaToPickFromEmptyState
          : Strings.Playlist.noMediaSelectedEmptyState
      )
      .multilineTextAlignment(.center)
      .foregroundStyle(Color(braveSystemName: .textTertiary))
      .frame(maxWidth: .infinity)
    }
    .padding(40)
    .playlistSheetDetentAnchor(id: .emptyPlaylistContent)
    .frame(maxHeight: .infinity, alignment: .top)
    .background(Color(braveSystemName: .containerBackground))
  }
}

#if DEBUG
// swift-format-ignore
#Preview {
  VStack {
    PlaylistContentUnavailableView(isPlaylistEmpty: false)
    Divider()
    PlaylistContentUnavailableView(isPlaylistEmpty: true)
  }
}
#endif
