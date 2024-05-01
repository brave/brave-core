// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import Data
import Foundation
import SwiftUI

@available(iOS 16.0, *)
public class PlaylistHostingController: UIHostingController<PlaylistRootView> {
  // FIXME: Remove optional after refactoring creation to be in one place
  public init(delegate: PlaylistRootView.Delegate? = nil) {
    super.init(rootView: PlaylistRootView(delegate: delegate))
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
public struct PlaylistRootView: View {
  /// Methods for handling playlist related actions that the browser should handle
  public struct Delegate {
    /// Open a URL in a tab (optionally in private mode)
    var openTabURL: (URL, _ isPrivate: Bool) -> Void
  }

  private var delegate: Delegate?

  public init(delegate: Delegate?) {
    self.delegate = delegate
  }

  public var body: some View {
    PlaylistContentView()
      .preparePlaylistEnvironment()
      .environment(\.managedObjectContext, DataController.swiftUIContext)
      .environment(\.colorScheme, .dark)
      .preferredColorScheme(.dark)
      .environment(
        \.openTabURL,
        .init { url, isPrivate in
          delegate?.openTabURL(url, isPrivate)
        }
      )
  }
}

@available(iOS 16.0, *)
struct PlaylistContentView: View {
  // FIXME: Will this have to be an ObservedObject instead to handle PiP?
  @StateObject private var playerModel: PlayerModel = .init()

  @FetchRequest(sortDescriptors: []) private var folders: FetchedResults<PlaylistFolder>
  @State private var selectedFolderID: PlaylistFolder.ID = PlaylistFolder.savedFolderUUID
  @State private var selectedItemID: PlaylistItem.ID?
  // FIXME: OrderedSet?
  @State private var itemQueue: [PlaylistItem.ID] = []

  @State private var selectedDetent: PlaylistSheetDetent = .small
  @State private var isNewPlaylistAlertPresented: Bool = false
  @State private var newPlaylistName: String = ""

  private var selectedFolder: PlaylistFolder? {
    folders.first(where: { $0.id == selectedFolderID })
  }

  private var selectedItem: PlaylistItem? {
    // FIXME: When PlaylistItem.ID is no longer the uuid, this will have to change
    selectedItemID.flatMap { PlaylistItem.getItem(uuid: $0) }
  }

  // FIXME: Move all selected folder/item queue logic into a testable ObservableObject
  private func makeItemQueue(selectedItemID: String?) {
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
      playerModel.pause()
      return
    }

    if let currentItemIndex = itemQueue.firstIndex(of: currentItem.id) {
      // This should be safe as we've already checked if the selected item is the last in the queue
      selectedItemID = itemQueue[currentItemIndex + 1]
    }
  }

  private func playItem(_ item: PlaylistItem) {
    let itemToReplace: AVPlayerItem? = {
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
      // FIXME: Handle streaming for blob URLs
      if let url = URL(string: item.mediaSrc) {
        return .init(asset: AVURLAsset(url: url))
      }
      return nil
    }()
    if let item = itemToReplace {
      playerModel.item = item
      playerModel.play()
      // Shrink it down?
      withAnimation(.snappy) {
        selectedDetent = .small
      }
    } else {
      withAnimation(.snappy) {
        selectedDetent = .anchor(.mediaPlayer)
      }
    }
  }

  public var body: some View {
    PlaylistSplitView(selectedDetent: $selectedDetent) {
      PlaylistSidebarList(
        folderID: selectedFolderID,
        selectedItemID: Binding(
          get: {
            selectedItemID
          },
          set: { newValue in
            // Make the item queue prior to setting the `selectedItemID`, which will start playing
            // said item
            makeItemQueue(selectedItemID: newValue)
            selectedItemID = newValue
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
            .playlistSheetDetents([.small, .anchor(.mediaPlayer), .large])
        } else {
          PlaylistContentUnavailableView(isPlaylistEmpty: PlaylistItem.count() == 0)
            .playlistSheetDetents([.anchor(.emptyPlaylistContent), .large])
        }
      }
    }
    .task {
      // FIXME: Will have to adjust this in the future to handle end of TTS
      for await _ in playerModel.didPlayToEndStream {
        playNextItem()
      }
    }
    .onAppear {
      // FIXME: Handle Auto-Play and launching with a selected item here
      if selectedItem != nil {
        selectedDetent = .small
      } else {
        selectedDetent = .anchor(.emptyPlaylistContent)
      }
    }
    .onDisappear {
      playerModel.stop()
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
      guard let id = newValue, let item = PlaylistItem.getItem(uuid: id) else {
        withAnimation(.snappy) {
          selectedDetent = .anchor(.emptyPlaylistContent)
        }
        playerModel.stop()
        return
      }
      playItem(item)
    }
    .onChange(of: playerModel.isShuffleEnabled) { _ in
      makeItemQueue(selectedItemID: selectedItemID)
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
  PlaylistRootView(
    delegate: .init(
      openTabURL: { _, _ in }
    )
  )
}
#endif
