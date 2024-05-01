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
  @Environment(\.openTabURL) private var openTabURL

  @FetchRequest private var items: FetchedResults<PlaylistItem>
  @Binding var selectedItemID: PlaylistItem.ID?
  var isPlaying: Bool

  private typealias PlaylistItemUUID = String
  @State private var downloadStates: [PlaylistItemUUID: PlaylistDownloadManager.DownloadState] = [:]
  @State private var downloadProgress: [PlaylistItemUUID: Double] = [:]

  init(
    folderID: PlaylistFolder.ID,
    selectedItemID: Binding<PlaylistItem.ID?>,
    isPlaying: Bool
  ) {
    self._items = FetchRequest<PlaylistItem>(
      sortDescriptors: [
        .init(keyPath: \PlaylistItem.order, ascending: true),
        .init(keyPath: \PlaylistItem.dateAdded, ascending: false),
      ],
      predicate: .init(format: "playlistFolder.uuid == %@", folderID)
    )
    self._selectedItemID = selectedItemID
    self.isPlaying = isPlaying
  }

  var body: some View {
    LazyVStack(alignment: .leading, spacing: 0) {
      if items.isEmpty {
        // FIXME: Would be better as an overaly on the ScrollView itself, so it could grow to the height of the drawer but would have to get the PlaylistDrawerScrollView out of PlaylistSplitView somehow
        PlaylistSidebarContentUnavailableView()
      } else {
        // SwiftUI needs to be able to access the ID keypath off main for animation purposes, so
        // we must identify CoreData objects by `objectID` in `ForEach` containers which are not
        // context/thread bound.
        //
        // FIXME: Remove custom implementations of Identifable on PlaylistFolder & PlaylistItem
        // PlaylistFolder.ID is used on old playlist design so will have to refactor that or wait
        // until after its removed.
        ForEach(items, id: \.objectID) { item in
          Button {
            selectedItemID = item.id
          } label: {
            PlaylistItemView(
              title: item.name,
              assetURL: URL(string: item.mediaSrc),
              pageURL: URL(string: item.pageSrc),
              duration: .seconds(item.duration),
              isItemPlaying: isPlaying && selectedItemID == item.id,
              downloadState: {
                if let uuid = item.uuid, let state = downloadStates[uuid] {
                  if state == .downloaded {
                    return .completed
                  }
                  if state == .inProgress {
                    // PlaylistDownloadManager reports percent as 0...100 not 0...1
                    let percentCompleted = downloadProgress[uuid, default: 0.0] / 100.0
                    return .downloading(percentComplete: percentCompleted)
                  }
                }
                if let cachedData = item.cachedData, !cachedData.isEmpty {
                  return .completed
                }
                return nil
              }()
            )
          }
          .onAppear {
            // FIXME: Move this logic out of the UI and into PlaylistManager on item add
            // This is currently how the current UI functions but it would be better not to do it
            PlaylistManager.shared.getAssetDuration(item: .init(item: item)) { _ in }
          }
          .contextMenu {
            if let cachedData = item.cachedData, !cachedData.isEmpty {
              Button {
                PlaylistManager.shared.deleteCache(item: .init(item: item))
              } label: {
                Label("Remove Offline Data", braveSystemImage: "leo.cloud.off")
              }
            } else {
              Button {
                PlaylistManager.shared.download(item: .init(item: item))
              } label: {
                Label("Save Offline Data", braveSystemImage: "leo.cloud.download")
              }
            }
            Divider()
            if let url = URL(string: item.pageSrc) {
              Button {
                openTabURL(url)
              } label: {
                Label("Open In New Tab", braveSystemImage: "leo.plus.add")
              }
              Button {
                openTabURL(url, privateMode: true)
              } label: {
                Label("Open In New Private Tab", braveSystemImage: "leo.product.private-window")
              }
              ShareLink(item: url) {
                Label("Share", braveSystemImage: "leo.share.macos")
              }
              Divider()
            }
            Button(role: .destructive) {
              PlaylistManager.shared.delete(item: .init(item: item))
              if selectedItemID == item.id {
                selectedItemID = nil
              }
            } label: {
              Label("Delete", braveSystemImage: "leo.trash")
            }
          }
        }
      }
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .onAppear {
      for item in items {
        guard let uuid = item.uuid else { continue }
        downloadStates[uuid] = PlaylistManager.shared.state(for: uuid)
      }
    }
    .onReceive(PlaylistManager.shared.downloadStateChanged) { output in
      downloadStates[output.id] = output.state
    }
    .onReceive(PlaylistManager.shared.downloadProgressUpdated) { output in
      downloadProgress[output.id] = output.percentComplete
    }
  }
}

@available(iOS 16.0, *)
struct PlaylistSidebarListHeader: View {
  var folders: [PlaylistFolder]
  @Binding var selectedFolder: PlaylistFolder
  @Binding var isPlaying: Bool
  @Binding var isNewPlaylistAlertPresented: Bool

  @State private var totalSizeOnDisk = Measurement<UnitInformationStorage>(value: 0, unit: .bytes)

  @MainActor private func calculateTotalSizeOnDisk(for folder: PlaylistFolder) async {
    // Since we're consuming CoreData we need to make sure those accesses happen on main
    let items = folder.playlistItems ?? []
    var totalSize: Int = 0
    for item in items {
      guard let cachedData = item.cachedData else { continue }
      totalSize += await Task.detached {
        // No CoreData usage allowed in here
        do {
          var isStale: Bool = false
          let url = try URL(resolvingBookmarkData: cachedData, bookmarkDataIsStale: &isStale)
          let values = try url.resourceValues(forKeys: [.fileSizeKey, .isDirectoryKey])
          if values.isDirectory == true {
            // This item is an HLS stream saved as a movpkg, get the size of the directory instead
            return Int(try FileManager.default.directorySize(at: url) ?? 0)
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
            "Pause",
            braveSystemImage: "leo.pause.circle"
          )
          .transition(.playButtonTransition)
        } else {
          Label(
            "Play",
            braveSystemImage: "leo.play.circle"
          )
          .transition(.playButtonTransition)
        }
      }
      .imageScale(.large)
      .font(.title2)
      .labelStyle(.iconOnly)
      .foregroundStyle(Color(braveSystemName: .textPrimary))
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
            Label("New Playlist", braveSystemImage: "leo.plus.add")
          }
        } label: {
          Text("\(selectedFolder.title ?? "") \(Image(braveSystemName: "leo.carat.down"))")
            .fontWeight(.semibold)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
            .contentShape(.rect)
        }
        HStack {
          let items = selectedFolder.playlistItems ?? []
          let totalDuration = Duration.seconds(items.reduce(0, { $0 + $1.duration }))
          let totalSize = totalSizeOnDisk
          Text("\(items.count) items")  // FIXME: Pluralization
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
      Button {

      } label: {
        Text("Edit")
          .fontWeight(.semibold)
      }
      .foregroundStyle(Color(braveSystemName: .textPrimary))
    }
    .padding()
    .task {
      await calculateTotalSizeOnDisk(for: selectedFolder)
    }
    .onChange(of: selectedFolder) { newValue in
      Task {
        await calculateTotalSizeOnDisk(for: newValue)
      }
    }
  }
}

@available(iOS 16.0, *)
struct PlaylistSidebarContentUnavailableView: View {
  var body: some View {
    VStack(spacing: 24) {
      // FIXME: Add social media icons here
      Text("Add media from your favorite sites and it’ll play here.")
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
  PlaylistSidebarList(folderID: "", selectedItemID: .constant(nil), isPlaying: false)
}
#endif
