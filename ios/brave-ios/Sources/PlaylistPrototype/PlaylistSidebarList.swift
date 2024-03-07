// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Playlist
import SwiftUI

@available(iOS 16.0, *)
struct PlaylistSidebarList: View {
  @FetchRequest private var items: FetchedResults<PlaylistItem>

  init(folderID: PlaylistFolder.ID) {
    self._items = FetchRequest<PlaylistItem>(
      sortDescriptors: [
        .init(keyPath: \PlaylistItem.order, ascending: true),
        .init(keyPath: \PlaylistItem.dateAdded, ascending: false),
      ],
      predicate: .init(format: "playlistFolder.uuid == %@", folderID)
    )
  }

  var body: some View {
    LazyVStack(alignment: .leading, spacing: 0) {
      if items.isEmpty {
        // FIXME: Would be better as an overaly on the ScrollView itself, so it could grow to the height of the drawer but would have to get the PlaylistDrawerScrollView out of PlaylistSplitView somehow
        PlaylistSidebarContentUnavailableView()
      } else {
        ForEach(items) { item in
          PlaylistItemView(
            title: item.name,
            assetURL: URL(string: item.mediaSrc),
            duration: .seconds(item.duration),
            isItemPlaying: false
          )
          .onAppear {
            // FIXME: Move this logic out of the UI and into PlaylistManager on item add
            // This is currently how the current UI functions but it would be better not to do it
            PlaylistManager.shared.getAssetDuration(item: .init(item: item)) { _ in }
          }
        }
      }
    }
    .frame(maxWidth: .infinity, alignment: .leading)
  }
}

@available(iOS 16.0, *)
struct PlaylistSidebarListHeader: View {
  @ObservedObject var playerModel: PlayerModel
  var folders: [PlaylistFolder]
  @Binding var selectedFolder: PlaylistFolder

  @State private var isPresentingNewPlaylistAlert = false
  @State private var newPlaylistName: String = ""

  var body: some View {
    HStack(spacing: 16) {
      Button {
        if playerModel.isPlaying {
          playerModel.pause()
        } else {
          playerModel.play()
        }
      } label: {
        Label(
          playerModel.isPlaying ? "Pause" : "Play",
          braveSystemImage: playerModel.isPlaying ? "leo.play.pause" : "leo.play.circle"
        )
        .imageScale(.large)
        .font(.title2)
      }
      .labelStyle(.iconOnly)
      .foregroundStyle(Color(braveSystemName: .textPrimary))
      VStack(alignment: .leading) {
        Menu {
          Picker("", selection: $selectedFolder) {
            ForEach(folders) { folder in
              Label(folder.title ?? "", braveSystemImage: "leo.product.playlist")
                .tag(folder)
            }
          }
          .pickerStyle(.inline)
          Divider()
          Button {
            isPresentingNewPlaylistAlert = true
          } label: {
            Label("New Playlist", braveSystemImage: "leo.plus.add")
          }
        } label: {
          Text("\(selectedFolder.title ?? "") \(Image(braveSystemName: "leo.carat.down"))")
            .fontWeight(.semibold)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .contentShape(.rect)
        }
        .alert("New Playlist", isPresented: $isPresentingNewPlaylistAlert) {
          TextField("My New Playlist", text: $newPlaylistName)
          Button(role: .cancel) {
            newPlaylistName = ""
          } label: {
            Text("Cancel")
          }
          Button {
            // FIXME: Create playlist
            newPlaylistName = ""
          } label: {
            Text("Create")
          }
          .disabled(newPlaylistName.isEmpty)
        }
        HStack {
          // FIXME: Pluralization
          Text("\(selectedFolder.playlistItems?.count ?? 0) items")
          Text("1h 35m")  // FIXME: Pull duration of folder somehow
          Text("245 MB")  // FIXME: Pull on-device storage usage somehow
        }
        .font(.caption2)
      }
      Spacer()
      Button {

      } label: {
        Text("Edit")
          .fontWeight(.semibold)
      }
      .foregroundStyle(Color(braveSystemName: .textPrimary))
    }
    .padding()
  }
}

@available(iOS 16.0, *)
struct PlaylistSidebarContentUnavailableView: View {
  var body: some View {
    VStack(spacing: 24) {
      Image(.emptyPlaylist)
      Text(
        "Fun starts when you have media to play. Go find your favorite media and add them to this playlist."
      )
      .multilineTextAlignment(.center)
      .foregroundStyle(Color(braveSystemName: .textTertiary))
      .frame(maxWidth: .infinity)
    }
    .padding(40)
  }
}

#if DEBUG
// swift-format-ignore
@available(iOS 16.0, *)
#Preview {
  // FIXME: Set up CoreData mock for Previews
  PlaylistSidebarList(folderID: "")
}
#endif
