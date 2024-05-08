// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import Data
import Foundation
import Playlist
import Preferences
import SwiftUI

@available(iOS 16.0, *)
struct PlaylistContentView: View {
  var initialPlaybackInfo: PlaylistRootView.InitialPlaybackInfo?

  @Environment(\.loadMediaStreamingAsset) private var loadMediaStreamingAsset

  // FIXME: Will this have to be an ObservedObject instead to handle PiP?
  @StateObject private var playerModel: PlayerModel = .init()

  @FetchRequest(sortDescriptors: []) private var folders: FetchedResults<PlaylistFolder>
  @State private var selectedFolderID: PlaylistFolder.ID = PlaylistFolder.savedFolderUUID
  @State private var selectedItemID: PlaylistItem.ID?
  // FIXME: OrderedSet?
  @State private var itemQueue: [PlaylistItem.ID] = []
  @State private var seekToInitialTimestamp: TimeInterval?

  @State private var selectedDetent: PlaylistSheetDetent = .small
  @State private var isNewPlaylistAlertPresented: Bool = false
  @State private var newPlaylistName: String = ""
  @State private var isPopulatingNewPlaylist: Bool = false

  @ObservedObject private var firstLoadAutoPlay = Preferences.Playlist.firstLoadAutoPlay
  @ObservedObject private var resumeFromLastTimePlayed = Preferences.Playlist.playbackLeftOff
  @ObservedObject private var lastPlayedItemURL = Preferences.Playlist.lastPlayedItemUrl

  private var selectedFolder: PlaylistFolder? {
    folders.first(where: { $0.id == selectedFolderID })
  }

  private var selectedItem: PlaylistItem? {
    selectedItemID.flatMap { PlaylistItem.getItem(id: $0) }
  }

  // FIXME: Move all selected folder/item queue logic into a testable ObservableObject
  private func makeItemQueue(selectedItemID: PlaylistItem.ID?) {
    var queue: [PlaylistItem.ID] = []
    var items = PlaylistItem.getItems(parentFolder: selectedFolder).map(\.id)
    if playerModel.isShuffleEnabled {
      if let selectedItemID {
        items.removeAll(where: { $0 == selectedItemID })
        queue.append(selectedItemID)
      }
      queue.append(contentsOf: items.shuffled())
    } else {
      if let selectedItemID {
        items = Array(items.drop(while: { $0 != selectedItemID }))
      }
      queue.append(contentsOf: items)
    }
    itemQueue = queue
  }

  private func playNextItem() {
    playerModel.pause()

    let repeatMode = playerModel.repeatMode

    if repeatMode == .one {
      // Replay the current video regardless of shuffle state/queue
      playerModel.play()
      return
    }

    guard let currentItem = selectedItem else {
      // FIXME: What should we do here if nothing is playing, play first item?
      return
    }

    if currentItem.id == itemQueue.last {
      if repeatMode == .all {
        // Last item in the set and repeat mode is on, start from the beginning of the queue
        selectedItemID = itemQueue.first
      }
      // Nothing to play if not repeating
      return
    }

    if let currentItemIndex = itemQueue.firstIndex(of: currentItem.id) {
      // This should be safe as we've already checked if the selected item is the last in the queue
      selectedItemID = itemQueue[currentItemIndex + 1]
    }

    Task {
      await self.prepareToPlaySelectedItem(
        initialOffset: seekToInitialTimestamp,
        playImmediately: true
      )
    }
  }

  @MainActor private func prepareToPlaySelectedItem(
    initialOffset: TimeInterval?,
    playImmediately: Bool
  ) async {
    guard let item = selectedItem else { return }
    // Shrink it down?
    withAnimation(.snappy) {
      selectedDetent = .small
    }
    // FIXME: Need a possible loading state here
    // Loading a media streaming asset may take time, need some sort of intermediate state where
    // we stop the current playback and show a loader even if the selected item is not fully ready
    // yet
    let playerItemToReplace: AVPlayerItem? = await {
      if let cachedData = item.cachedData {
        do {
          var isStale: Bool = false
          let url = try URL(resolvingBookmarkData: cachedData, bookmarkDataIsStale: &isStale)
          if FileManager.default.fileExists(atPath: url.path) {
            return .init(url: url)
          }
        } catch {
        }
      }
      if let newItem = try? await loadMediaStreamingAsset(item: .init(item: item)),
        let url = URL(string: newItem.src)
      {
        return .init(asset: AVURLAsset(url: url))
      }
      return nil
    }()
    if let playerItem = playerItemToReplace {
      playerModel.item = playerItem
      if let initialOffset {
        await playerModel.seek(to: initialOffset, accurately: true)
      } else if resumeFromLastTimePlayed.value {
        await playerModel.seek(to: item.lastPlayedOffset, accurately: true)
      }
      if playImmediately {
        playerModel.play()
      }
    }
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
            makeItemQueue(selectedItemID: newValue)
            if selectedItemID == newValue {
              // Already selected, restart it (based on prior behaviour)
              withAnimation(.snappy) {
                selectedDetent = .small
              }
              Task {
                await playerModel.seek(to: 0, accurately: true)
                playerModel.play()
              }
              return
            }
            selectedItemID = newValue
            Task { await prepareToPlaySelectedItem(initialOffset: nil, playImmediately: true) }
          }
        ),
        isPlaying: playerModel.isPlaying
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
              selectedFolderID = $0.id
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
                  selectedItemID = itemQueue.first
                } else {
                  playerModel.play()
                }
              } else {
                playerModel.pause()
              }
            }
          ),
          isNewPlaylistAlertPresented: $isNewPlaylistAlertPresented
        )
      }
    } content: {
      ZStack {
        if let selectedItem {
          // FIXME: Swap out for some sort of container for the selected item (shows different views if its webpage TTS for instance)
          MediaContentView(model: playerModel, selectedItem: selectedItem)
            .playlistSheetDetents([.small, .anchor(.mediaPlayer), .large])
        } else {
          PlaylistContentUnavailableView(isPlaylistEmpty: PlaylistItem.count() == 0)
            .playlistSheetDetents([.anchor(.emptyPlaylistContent), .large])
        }
      }
    }
    .task(priority: .medium) {
      // FIXME: Will have to adjust this in the future to handle end of TTS
      for await _ in playerModel.didPlayToEndStream {
        playNextItem()
      }
    }
    .onAppear {
      // Make an initial queue assuming nothing selected
      makeItemQueue(selectedItemID: nil)
      // Possibly update the selected folder based on the last item played
      let lastPlayedItem: PlaylistItem? = lastPlayedItemURL.value
        .map { PlaylistItem.getItems(pageSrc: $0) }?.first
      if let initialPlaybackInfo,
        let item = PlaylistItem.getItem(uuid: initialPlaybackInfo.itemUUID)
      {
        // If we're coming from a tab with some initial video data update the folder & item
        seekToInitialTimestamp = initialPlaybackInfo.timestamp
        selectedItemID = item.id
        if let folderID = selectedItem?.playlistFolder?.id {
          selectedFolderID = folderID
        }
        // Remake the item queue with the newly selected item
        makeItemQueue(selectedItemID: selectedItemID)
      } else {
        // Just opening playlist without inherited item data, so restore the previously watched item
        // or just auto-play the first item if the pref is on
        if let lastPlayedItem {
          if let lastPlayedItemParentFolder = lastPlayedItem.playlistFolder {
            selectedFolderID = lastPlayedItemParentFolder.id
          }
          selectedItemID = lastPlayedItem.id
        } else {
          // Start the first video in the queue if auto-play is enabled
          if resumeFromLastTimePlayed.value, let selectedItem {
            seekToInitialTimestamp = selectedItem.lastPlayedOffset
          }
          selectedItemID = itemQueue.first
        }
      }
      Task { @MainActor in
        // If we need to stream the item we need to wait until `loadMediaStreamingAsset` is valid
        if selectedItem?.cachedData == nil, !loadMediaStreamingAsset.isPrepared {
          try await Task.sleep(for: .seconds(0.5))
        }
        await self.prepareToPlaySelectedItem(
          initialOffset: seekToInitialTimestamp,
          playImmediately: initialPlaybackInfo != nil || firstLoadAutoPlay.value
        )
      }
      if selectedItem != nil {
        selectedDetent = .small
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
      playerModel.stop()
    }
    .alert("New Playlist", isPresented: $isNewPlaylistAlertPresented) {
      TextField("My New Playlist", text: $newPlaylistName)
      Button(role: .cancel) {
        newPlaylistName = ""
      } label: {
        Text("Cancel")
      }
      .keyboardShortcut(.cancelAction)
      Button {
        defer { newPlaylistName = "" }
        if newPlaylistName.isEmpty {
          // See comment below about .disabled modifier
          return
        }
        PlaylistFolder.addFolder(title: newPlaylistName) { uuid in
          selectedFolderID = uuid
          if let defaultFolderItems = PlaylistFolder.getFolder(
            uuid: PlaylistFolder.savedFolderUUID
          )?.playlistItems, !defaultFolderItems.isEmpty {
            isPopulatingNewPlaylist = true
          }
        }
      } label: {
        Text("Create")
      }
      .keyboardShortcut(.defaultAction)
      // Unfortunately we can't disable this button due to _many_ SwiftUI bugs.
      // - On iOS 16, the button simply does not get added to the alert at all.
      // - On iOS 17 (tested to 17.4), the button shows up but tapping it does not execute the
      //   action even when the button is enabled.
      // .disabled(newPlaylistName.isEmpty)
    }
    .sheet(isPresented: $isPopulatingNewPlaylist) {
      PopulateNewPlaylistView(destinationFolder: selectedFolderID)
        .presentationDetents([.medium, .large])
        .environment(\.colorScheme, .dark)
        .preferredColorScheme(.dark)
    }
    .onChange(of: selectedItemID) {
      [selectedItem, currentTime = playerModel.currentTime, duration = playerModel.duration]
      newValue in
      if let priorSelectedItem = selectedItem {
        PlaylistManager.shared.updateLastPlayed(
          item: .init(item: priorSelectedItem),
          playTime: currentTime == duration ? 0.0 : currentTime
        )
      }
      if newValue == nil {
        withAnimation(.snappy) {
          selectedDetent = .anchor(.emptyPlaylistContent)
        }
        playerModel.stop()
        return
      }
    }
    .onChange(of: playerModel.isShuffleEnabled) { _ in
      makeItemQueue(selectedItemID: selectedItemID)
    }
    .onChange(of: Array(folders)) { newValue in
      if !newValue.map(\.id).contains(selectedFolderID) {
        // Reset the selected folder if the user deletes the folder they had selected
        selectedFolderID = PlaylistFolder.savedFolderUUID
      }
    }
  }
}

extension PlaylistSheetDetent.DetentAnchorID {
  static let emptyPlaylistContent: Self = .init(id: "empty-playlist")
}

/// Shown when the use has no playlist item selected
@available(iOS 16.0, *)
struct PlaylistContentUnavailableView: View {
  var isPlaylistEmpty: Bool

  var body: some View {
    VStack(spacing: 24) {
      Image(.emptyPlaylist)
      Text(
        isPlaylistEmpty
          ? "Add videos to the playlist and play them here!"
          : "Tap videos from your playlist to play."
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
@available(iOS 16.0, *)
#Preview {
  VStack {
    PlaylistContentUnavailableView(isPlaylistEmpty: false)
    Divider()
    PlaylistContentUnavailableView(isPlaylistEmpty: true)
  }
}
#endif
