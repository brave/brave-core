// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import Data
import Foundation
import SwiftUI

@available(iOS 16.0, *)
public class PlaylistHostingController: UIHostingController<PlaylistPrototypeRootView> {
  public init() {
    super.init(rootView: PlaylistPrototypeRootView())
    modalPresentationStyle = .fullScreen
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    // SwiftUI will handle the background
    view.backgroundColor = .clear
  }
}

@available(iOS 16.0, *)
public struct PlaylistPrototypeRootView: View {
  public init() {}

  public var body: some View {
    PlaylistContentView()
      .preparePlaylistEnvironment()
      .environment(\.managedObjectContext, DataController.swiftUIContext)
      .environment(\.colorScheme, .dark)
      .preferredColorScheme(.dark)
  }
}

@available(iOS 16.0, *)
struct PlaylistContentView: View {
  // FIXME: Will this have to be an ObservedObject instead to handle PiP?
  @StateObject private var playerModel: PlayerModel = .init()

  @FetchRequest(sortDescriptors: []) private var folders: FetchedResults<PlaylistFolder>
  @State private var selectedFolderID: PlaylistFolder.ID = PlaylistFolder.savedFolderUUID
  @State private var selectedItemID: PlaylistItem.ID?

  @State private var selectedDetent: PlaylistSheetDetent = .small
  @State private var isNewPlaylistAlertPresented: Bool = false
  @State private var newPlaylistName: String = ""

  private var selectedFolder: PlaylistFolder? {
    folders.first(where: { $0.id == selectedFolderID })
  }

  private var selectedItem: PlaylistItem? {
    selectedItemID.flatMap { PlaylistItem.getItem(uuid: $0) }
  }

  private func playNextItem() {
    let repeatMode = playerModel.repeatMode
    let isShuffleEnabled = playerModel.isShuffleEnabled
    let currentItem = selectedItem

    if repeatMode == .one {
      // Replay the current video regardless of shuffle state/queue
      playerModel.play()
      return
    }

    // FIXME: Handle the rest of the cases
  }

  public var body: some View {
    PlaylistSplitView(selectedDetent: $selectedDetent) {
      PlaylistSidebarList(
        folderID: selectedFolderID,
        selectedItemID: $selectedItemID
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
          isPlaying: Binding(
            get: {
              playerModel.isPlaying
            },
            set: { newValue in
              if newValue {
                playerModel.play()
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
        } else {
          // FIXME: Empty State
        }
      }
      .playlistSheetDetents([.small, .anchor(.mediaPlayer), .large])
    }
    .task {
      for await _ in playerModel.didPlayToEndStream {
        playNextItem()
      }
    }
    .alert("New Playlist", isPresented: $isNewPlaylistAlertPresented) {
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
    .onChange(of: selectedItemID) { newValue in
      if let id = newValue, let item = PlaylistItem.getItem(uuid: id) {
        if let cachedData = item.cachedData {
          do {
            var isStale: Bool = false
            let url = try URL(resolvingBookmarkData: cachedData, bookmarkDataIsStale: &isStale)
            if FileManager.default.fileExists(atPath: url.path) {
              playerModel.player.replaceCurrentItem(with: .init(url: url))
            }
          } catch {
          }
        } else {
          if let url = URL(string: item.mediaSrc) {
            playerModel.player.replaceCurrentItem(with: .init(asset: AVURLAsset(url: url)))
          }
        }
        playerModel.play()
        // Shrink it down?
        selectedDetent = .small
      }
    }
  }
}

@available(iOS 16.0, *)
extension View {
  /// Adds playlist-specific environment variables
  ///
  /// Use this if you need access to playlist environment variables such as `isFullScreen`,
  /// `requestGeometryUpdate` or `interfaceOrientation` in a SwiftUI Preview
  func preparePlaylistEnvironment() -> some View {
    self
      .observingInterfaceOrientation()
      .creatingRequestGeometryUpdateAction()
      .setUpFullScreenEnvironment()
  }
}

#if DEBUG
// swift-format-ignore
@available(iOS 16.0, *)
#Preview {
  PlaylistPrototypeRootView()
}
#endif
