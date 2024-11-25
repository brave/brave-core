// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import BraveUI
import CoreData
import Data
import DesignSystem
import Playlist
import Strings
import SwiftUI

/// A screen that presents the user with a search bar to search their local playlist items
struct SearchView: View {
  var folders: [PlaylistFolder]
  @Binding var selectedItemID: PlaylistItem.ID?
  @Binding var selectedFolderID: PlaylistFolder.ID
  var isPlaying: Bool
  var onPlaylistUpdated: () -> Void

  @State private var query: String = ""
  @State private var isSearchFocused: Bool = false
  @FetchRequest private var queryResults: FetchedResults<PlaylistItem>

  @Environment(\.dismiss) private var dismiss

  private var queryPredicate: NSPredicate {
    .init(format: "name CONTAINS[c] %@", query)
  }

  init(
    folders: [PlaylistFolder],
    selectedItemID: Binding<PlaylistItem.ID?>,
    selectedFolderID: Binding<PlaylistFolder.ID>,
    isPlaying: Bool,
    onPlaylistUpdated: @escaping () -> Void
  ) {
    self.folders = folders
    self._selectedItemID = selectedItemID
    self._selectedFolderID = selectedFolderID
    self.isPlaying = isPlaying
    self.onPlaylistUpdated = onPlaylistUpdated
    self._queryResults = FetchRequest<PlaylistItem>(
      sortDescriptors: [
        .init(keyPath: \PlaylistItem.order, ascending: true),
        .init(keyPath: \PlaylistItem.dateAdded, ascending: false),
      ],
      predicate: .init(format: "name CONTAINS[c] %@", "")
    )
  }

  var body: some View {
    NavigationStack {
      ScrollView {
        PlaylistItemList(
          items: Array(queryResults),
          folders: folders,
          selectedFolderID: selectedFolderID,
          selectedItemID: $selectedItemID,
          isPlaying: isPlaying,
          onItemSelected: { item in
            selectedItemID = item.id
            if let folder = item.playlistFolder, folder.id != selectedFolderID {
              selectedFolderID = folder.id
            }
            dismiss()
          },
          onPlaylistUpdated: {
            // For some reason when an item is updated (specifically moved to another folder)
            // the fetched item results will empty out completely and not repopulate when the
            // saved background context is merged into the view context. This only occurs when
            // there is a query predicate involved for some reason, so this sets the predicate again
            // to force @FetchRequest to update
            DispatchQueue.main.async {
              queryResults.nsPredicate = queryPredicate
            }
            onPlaylistUpdated()
          }
        )
        .animation(.default, value: Array(queryResults))
      }
      .overlay {
        if queryResults.isEmpty {
          SearchUnavailableView(query: query)
        }
      }
      .navigationTitle(Strings.Playlist.searchTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .principal) {
          SearchBar(
            text: $query,
            placeholder: Strings.Playlist.searchTitle,
            isFocused: $isSearchFocused
          )
        }
        ToolbarItemGroup(placement: .confirmationAction) {
          Button {
            dismiss()
          } label: {
            Text(Strings.done)
          }
          .tint(Color.white)
        }
      }
      .toolbarBackground(Color(braveSystemName: .neutral10), for: .navigationBar)
    }
    .onAppear {
      isSearchFocused = true
    }
    .onChange(of: query) { query in
      queryResults.nsPredicate = queryPredicate
    }
  }
}

struct SearchUnavailableView: View {
  var query: String

  var body: some View {
    VStack(spacing: 12) {
      Image(braveSystemName: "leo.search")
        .font(.title)
      VStack(spacing: 8) {
        Text(
          query.isEmpty
            ? Strings.Playlist.emptySearchQueryTitle : Strings.Playlist.noSearchResultsFoundTitle
        )
        .font(.headline)
        if query.isEmpty {
          Text(Strings.Playlist.emptySearchQuerySubtitle)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
      }
    }
    .foregroundStyle(Color(braveSystemName: .textPrimary))
    .multilineTextAlignment(.center)
    .padding()
  }
}
