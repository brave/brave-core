// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Data
import DesignSystem
import Foundation
import Playlist
import Strings
import SwiftUI

struct PlaylistSidebarList: View {
  var folders: [PlaylistFolder]
  var selectedFolderID: PlaylistFolder.ID
  @Binding var selectedItemID: PlaylistItem.ID?
  var isPlaying: Bool
  var onPlaylistUpdated: () -> Void

  @FetchRequest private var items: FetchedResults<PlaylistItem>

  init(
    folders: [PlaylistFolder],
    folderID: PlaylistFolder.ID,
    selectedItemID: Binding<PlaylistItem.ID?>,
    isPlaying: Bool,
    onPlaylistUpdated: @escaping () -> Void
  ) {
    self.folders = folders
    self.selectedFolderID = folderID
    self._items = FetchRequest<PlaylistItem>(
      sortDescriptors: [
        .init(keyPath: \PlaylistItem.order, ascending: true),
        .init(keyPath: \PlaylistItem.dateAdded, ascending: false),
      ],
      predicate: .init(format: "playlistFolder.uuid == %@", folderID)
    )
    self._selectedItemID = selectedItemID
    self.isPlaying = isPlaying
    self.onPlaylistUpdated = onPlaylistUpdated
  }

  var body: some View {
    LazyVStack(spacing: 0) {
      if items.isEmpty {
        PlaylistSidebarContentUnavailableView()
      }
      PlaylistItemList(
        items: Array(items),
        folders: folders,
        selectedFolderID: selectedFolderID,
        selectedItemID: $selectedItemID,
        isPlaying: isPlaying,
        onItemSelected: { item in
          selectedItemID = item.id
        },
        onPlaylistUpdated: onPlaylistUpdated
      )
    }
    .frame(maxWidth: .infinity, alignment: .leading)
  }
}

struct PlaylistSidebarListHeader: View {
  var folders: [PlaylistFolder]
  @Binding var selectedFolder: PlaylistFolder
  var selectedItemID: PlaylistItem.ID?
  @Binding var isPlaying: Bool
  @Binding var isNewPlaylistAlertPresented: Bool
  @Binding var isEditModePresented: Bool

  @State private var totalSizeOnDisk = Measurement<UnitInformationStorage>(value: 0, unit: .bytes)
  @State private var isDeletePlaylistConfirmationActive: Bool = false
  @State private var isRenamePlaylistAlertPresented: Bool = false

  @FetchRequest private var selectedFolderItems: FetchedResults<PlaylistItem>

  init(
    folders: [PlaylistFolder],
    selectedFolder: Binding<PlaylistFolder>,
    selectedItemID: PlaylistItem.ID?,
    isPlaying: Binding<Bool>,
    isNewPlaylistAlertPresented: Binding<Bool>,
    isEditModePresented: Binding<Bool>
  ) {
    self.folders = folders
    self._selectedFolder = selectedFolder
    self.selectedItemID = selectedItemID
    self._isPlaying = isPlaying
    self._isNewPlaylistAlertPresented = isNewPlaylistAlertPresented
    self._isEditModePresented = isEditModePresented
    self._selectedFolderItems = .init(
      sortDescriptors: [],
      predicate: .init(format: "playlistFolder.uuid == %@", selectedFolder.id)
    )
  }

  @MainActor private func calculateTotalSizeOnDisk(for folder: PlaylistFolder) async {
    // Since we're consuming CoreData we need to make sure those accesses happen on main
    let items = selectedFolderItems
    var totalSize: Int = 0
    for item in items {
      guard let cachedData = item.cachedData else { continue }
      totalSize += await Task.detached {
        guard let cachedDataURL = await PlaylistItem.resolvingCachedData(cachedData) else {
          return 0
        }
        // No CoreData usage allowed in here
        do {
          let values = try cachedDataURL.resourceValues(forKeys: [.fileSizeKey, .isDirectoryKey])
          if values.isDirectory == true {
            // This item is an HLS stream saved as a movpkg, get the size of the directory instead
            return Int(try await AsyncFileManager.default.sizeOfDirectory(at: cachedDataURL))
          }
          return values.fileSize ?? 0
        } catch {
          return 0
        }
      }.value
    }
    totalSizeOnDisk = .init(value: Double(totalSize), unit: .bytes)
  }

  var body: some View {
    HStack(spacing: 16) {
      Button {
        isPlaying.toggle()
      } label: {
        if isPlaying {
          Label(
            Strings.Playlist.accessibilityPause,
            braveSystemImage: "leo.pause.circle"
          )
          .transition(.playButtonTransition)
        } else {
          Label(
            Strings.Playlist.accessibilityPlay,
            braveSystemImage: "leo.play.circle"
          )
          .transition(.playButtonTransition)
        }
      }
      .imageScale(.large)
      .font(.title2)
      .labelStyle(.iconOnly)
      .tint(Color(braveSystemName: .textPrimary))
      .disabled(selectedItemID == nil && selectedFolderItems.isEmpty)
      VStack(alignment: .leading) {
        Menu {
          Picker("", selection: $selectedFolder) {
            ForEach(folders, id: \.objectID) { folder in
              Label(folder.title ?? "", braveSystemImage: "leo.product.playlist")
                .tag(folder)
            }
          }
          .pickerStyle(.inline)
          Divider()
          Button {
            isNewPlaylistAlertPresented = true
          } label: {
            Label(Strings.Playlist.newPlaylistButtonTitle, braveSystemImage: "leo.plus.add")
          }
        } label: {
          Text("\(selectedFolder.title ?? "") \(Image(braveSystemName: "leo.carat.down"))")
            .fontWeight(.semibold)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .contentShape(.rect)
        }
        HStack {
          let items = selectedFolderItems
          let totalDuration = Duration.seconds(
            items.reduce(
              0,
              { total, item in
                if !item.duration.isFinite || item.duration == .greatestFiniteMagnitude {
                  // Skip infinite and max values (live videos)
                  return total
                }
                return total + item.duration
              }
            )
          )
          let totalSize = totalSizeOnDisk
          Text(
            items.count == 1
              ? Strings.Playlist.itemCountSingular
              : String.localizedStringWithFormat(Strings.Playlist.itemCountPlural, items.count)
          )
          if !items.isEmpty {
            Text(totalDuration, format: .units(width: .narrow, maximumUnitCount: 2))
          }
          if !totalSize.value.isZero {
            Text(totalSizeOnDisk, format: .byteCount(style: .file))
          }
        }
        .font(.caption2)
      }
      Spacer()
      Menu {
        if selectedFolder.uuid != PlaylistFolder.savedFolderUUID {
          Button {
            isRenamePlaylistAlertPresented = true
          } label: {
            Label(Strings.Playlist.renamePlaylist, braveSystemImage: "leo.edit.pencil")
          }
          Button(role: .destructive) {
            isDeletePlaylistConfirmationActive = true
          } label: {
            Label(Strings.Playlist.deletePlaylist, braveSystemImage: "leo.trash")
          }
        }
      } label: {
        Text(Strings.Playlist.editPlaylistShortTitle)
          .fontWeight(.semibold)
      } primaryAction: {
        isEditModePresented = true
      }
      .foregroundStyle(Color(braveSystemName: .textPrimary))
      .disabled(
        selectedFolder.uuid == PlaylistFolder.savedFolderUUID
          && selectedFolder.playlistItems?.isEmpty == true
      )
    }
    .padding()
    .task {
      await calculateTotalSizeOnDisk(for: selectedFolder)
    }
    .onChange(of: selectedFolderItems.map(\.cachedData)) { _ in
      Task {
        await calculateTotalSizeOnDisk(for: selectedFolder)
      }
    }
    .editActions(
      for: selectedFolder,
      isDeletePlaylistConfirmationPresented: $isDeletePlaylistConfirmationActive,
      isRenamePlaylistAlertPresented: $isRenamePlaylistAlertPresented
    )
  }
}

struct PlaylistSidebarContentUnavailableView: View {
  var body: some View {
    VStack(spacing: 24) {
      HStack(spacing: 12) {
        Image("social-youtube", bundle: .module)
        Image("social-facebook", bundle: .module)
        Image("social-vimeo", bundle: .module)
        Image("social-instagram", bundle: .module)
      }
      Text(Strings.Playlist.noMediaToPickFromEmptyState)
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textTertiary))
        .frame(maxWidth: .infinity)
    }
    .padding(40)
  }
}

#if DEBUG
#Preview {
  PlaylistSidebarList(
    folders: [],
    folderID: "",
    selectedItemID: .constant(nil),
    isPlaying: false,
    onPlaylistUpdated: {}
  )
}
#endif
