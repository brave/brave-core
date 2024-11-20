// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Playlist
import Strings
import SwiftUI

struct EditFolderView: View {
  @ObservedObject var folder: PlaylistFolder
  var folders: [PlaylistFolder]

  @FetchRequest private var items: FetchedResults<PlaylistItem>

  @State private var selectedItems: Set<PlaylistItem.ID> = []
  @State private var isDeletePlaylistConfirmationActive: Bool = false
  @State private var isRenamePlaylistAlertPresented: Bool = false

  @Environment(\.dismiss) private var dismiss

  init(folder: PlaylistFolder, folders: [PlaylistFolder]) {
    self.folder = folder
    self.folders = folders
    self._items = FetchRequest<PlaylistItem>(
      sortDescriptors: [
        .init(keyPath: \PlaylistItem.order, ascending: true),
        .init(keyPath: \PlaylistItem.dateAdded, ascending: false),
      ],
      predicate: .init(format: "playlistFolder.uuid == %@", folder.uuid ?? ""),
      animation: .default
    )
  }

  private func deleteSelectedItems() async {
    let items = selectedItems.compactMap { id in self.items.first(where: { $0.id == id }) }
    for item in items {
      await PlaylistManager.shared.delete(item: .init(item: item))
    }
    selectedItems = []
  }

  var body: some View {
    NavigationStack {
      List(selection: $selectedItems) {
        ForEach(items) { item in
          PlaylistItemView(
            title: item.name,
            assetURL: URL(string: item.mediaSrc),
            pageURL: URL(string: item.pageSrc),
            duration: .init(item.duration),
            isSelected: false,
            isPlaying: false,
            downloadState: nil
          )
        }
        .onMove { indexSet, offset in
          PlaylistItem.reorderItems(in: folder, fromOffsets: indexSet, toOffset: offset)
        }
        .listRowBackground(Color.clear)
        .listRowSeparator(.hidden)
      }
      .listStyle(.plain)
      .environment(\.editMode, .constant(.active))
      .scrollContentBackground(.hidden)
      .background(Color(braveSystemName: .neutral10))
      .toolbarBackground(
        Color(braveSystemName: .containerBackground),
        for: .navigationBar,
        .bottomBar
      )
      .toolbarBackground(.visible, for: .navigationBar, .bottomBar)
      .navigationTitle(folder.title ?? Strings.Playlist.editPlaylist)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            dismiss()
          } label: {
            Text(Strings.Playlist.doneButtonTitle)
              .fontWeight(.semibold)
          }
          .tint(Color(braveSystemName: .textInteractive))
        }
        ToolbarItemGroup(placement: .topBarTrailing) {
          if folder.uuid != PlaylistFolder.savedFolderUUID {
            Menu {
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
            } label: {
              Label(
                Strings.Playlist.accessibilityMoreMenuButtonTitle,
                systemImage: "ellipsis.circle.fill"
              )
            }
            .tint(Color(braveSystemName: .textInteractive))
          }
        }
        ToolbarItemGroup(placement: .bottomBar) {
          Menu {
            Picker(
              "",
              selection: Binding(
                get: { folder.id },
                set: {
                  PlaylistItem.moveItems(items: Array(selectedItems), to: $0)
                  selectedItems = []
                }
              )
            ) {
              ForEach(folders, id: \.objectID) { folder in
                Label(folder.title ?? "", braveSystemImage: "leo.product.playlist")
                  .tag(folder.id)
              }
            }
            .pickerStyle(.inline)
          } label: {
            Text(Strings.Playlist.moveItem)
          }
          .tint(Color(braveSystemName: .textInteractive))
          .disabled(selectedItems.isEmpty || folders.count < 2)
          Spacer()
          Button(role: .destructive) {
            Task {
              await deleteSelectedItems()
            }
          } label: {
            Text(Strings.Playlist.deleteItem)
          }
          .tint(Color(braveSystemName: .textInteractive))
          .disabled(selectedItems.isEmpty)
        }
      }
    }
    .preferredColorScheme(.dark)
    .environment(\.colorScheme, .dark)
    .editActions(
      for: folder,
      isDeletePlaylistConfirmationPresented: $isDeletePlaylistConfirmationActive,
      isRenamePlaylistAlertPresented: $isRenamePlaylistAlertPresented
    )
  }
}
