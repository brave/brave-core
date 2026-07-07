// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Playlist
import Strings
import SwiftUI

struct PlaylistItemList: View {
  var items: [PlaylistItem]
  var folders: [PlaylistFolder]
  var selectedFolderID: PlaylistFolder.ID
  @Binding var selectedItemID: PlaylistItem.ID?
  var isPlaying: Bool
  var onItemSelected: (PlaylistItem) -> Void
  var onPlaylistUpdated: () -> Void

  @Environment(\.openTabURL) private var openTabURL
  private typealias PlaylistItemUUID = String
  @State private var cacheStates: [PlaylistItemUUID: PlaylistDownloadManager.CacheState] = [:]
  @State private var cacheProgress: [PlaylistItemUUID: Double] = [:]

  init(
    items: [PlaylistItem],
    folders: [PlaylistFolder],
    selectedFolderID: PlaylistFolder.ID,
    selectedItemID: Binding<PlaylistItem.ID?>,
    isPlaying: Bool,
    onItemSelected: @escaping (PlaylistItem) -> Void,
    onPlaylistUpdated: @escaping () -> Void
  ) {
    self.items = items
    self.folders = folders
    self.selectedFolderID = selectedFolderID
    self._selectedItemID = selectedItemID
    self.isPlaying = isPlaying
    self.onItemSelected = onItemSelected
    self.onPlaylistUpdated = onPlaylistUpdated
  }

  var body: some View {
    LazyVStack(alignment: .leading, spacing: 0) {
      ForEach(items) { item in
        Button {
          onItemSelected(item)
        } label: {
          PlaylistItemView(
            title: item.name,
            assetURL: URL(string: item.mediaSrc),
            pageURL: URL(string: item.pageSrc),
            duration: .init(item.duration),
            isSelected: selectedItemID == item.id,
            isPlaying: isPlaying,
            cacheState: {
              // Cache-first caches every item automatically, so per-item cache status
              // the "Preparing" progress and the ready checkmark is visual noise.
              if FeatureList.kPlaylistCacheFirstEnabled.enabled {
                return nil
              }
              guard let uuid = item.uuid, let state = cacheStates[uuid] else {
                return nil
              }
              if state == .cached {
                return .completed
              }
              if state == .inProgress {
                // PlaylistDownloadManager reports percent as 0...100 not 0...1
                let percentCompleted = cacheProgress[uuid, default: 0.0] / 100.0
                return .downloading(percentComplete: percentCompleted)
              }
              return nil
            }()
          )
          .padding(.horizontal, 12)
          .padding(.vertical, 8)
        }
        .onAppear {
          // FIXME: Move this logic out of the UI and into PlaylistManager on item add
          // This is currently how the current UI functions but it would be better not to do it
          PlaylistManager.shared.getAssetDuration(item: .init(item: item)) { _ in }
        }
        .contextMenu {
          // Mirror the badge's source of truth: only offer "Remove offline data" when the cached file
          // actually exists on disk. Otherwise we'd offer to remove a phantom download for an item whose bookmark
          // survived but whose file was wiped (e.g. by iOS reclaiming `com.apple.UserManagedAssets*`). Cache-first manages the cache automatically
          if let uuid = item.uuid, cacheStates[uuid] == .cached,
            !FeatureList.kPlaylistCacheFirstEnabled.enabled
          {
            Button {
              Task { @MainActor in
                await PlaylistManager.shared.deleteCache(item: .init(item: item))
              }
            } label: {
              Label(Strings.Playlist.removeOfflineData, braveSystemImage: "leo.cloud.off")
            }
          } else if FeatureList.kPlaylistOfflineCacheEnabled.enabled {
            Button {
              PlaylistManager.shared.download(item: .init(item: item))
            } label: {
              Label(Strings.Playlist.saveOfflineData, braveSystemImage: "leo.cloud.download")
            }
          }
          Divider()
          if let url = URL(string: item.pageSrc) {
            Button {
              openTabURL(url)
            } label: {
              Label(Strings.Playlist.openInNewTab, braveSystemImage: "leo.browser.mobile-tab-new")
            }
            Button {
              openTabURL(url, privateMode: true)
            } label: {
              Label(
                Strings.Playlist.openInNewPrivateTab,
                braveSystemImage: "leo.product.private-window"
              )
            }
            ShareLink(item: url) {
              Label(Strings.Playlist.share, braveSystemImage: "leo.share.macos")
            }
            Divider()
          }
          if folders.count > 1 {
            Picker(
              selection: Binding(
                get: { item.playlistFolder?.id ?? selectedFolderID },
                set: {
                  PlaylistItem.moveItems(items: [item.objectID], to: $0) {
                    onPlaylistUpdated()
                  }
                }
              )
            ) {
              ForEach(folders, id: \.objectID) { folder in
                Label(folder.title ?? "", braveSystemImage: "leo.product.playlist")
                  .tag(folder.id)
              }
            } label: {
              Label(Strings.Playlist.moveMenuItemTitle, braveSystemImage: "leo.folder.exchange")
            }
            .pickerStyle(.menu)
          }
          Button(role: .destructive) {
            Task {
              await PlaylistManager.shared.delete(item: .init(item: item))
              if selectedItemID == item.id {
                selectedItemID = nil
              }
              onPlaylistUpdated()
            }
          } label: {
            Label(Strings.Playlist.deleteItem, braveSystemImage: "leo.trash")
          }
        }
      }
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .task {
      for item in items {
        guard let uuid = item.uuid else { continue }
        cacheStates[uuid] = await PlaylistManager.shared.cacheState(for: uuid)
      }
    }
    .onReceive(PlaylistManager.shared.downloadStateChanged) { output in
      cacheStates[output.id] = output.state
      if output.state == .cached, let item = items.first(where: { $0.uuid == output.id }) {
        PlaylistManager.shared.getAssetDuration(item: .init(item: item)) { _ in }
      }
    }
    .onReceive(PlaylistManager.shared.downloadProgressUpdated) { output in
      cacheProgress[output.id] = output.percentComplete
    }
  }
}
