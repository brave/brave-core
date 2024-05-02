// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import Data
import Foundation
import Playlist
import SwiftUI

@available(iOS 16.0, *)
public class PlaylistHostingController: UIHostingController<PlaylistRootView> {
  public init(delegate: PlaylistRootView.Delegate) {
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

  deinit {
    // FIXME: Remove in the future
    print("PlaylistHostingController deinit")
  }
}

@available(iOS 16.0, *)
public struct PlaylistRootView: View {
  /// Methods for handling playlist related actions that the browser should handle
  public struct Delegate {
    /// Open a URL in a tab (optionally in private mode)
    public var openTabURL: (URL, _ isPrivate: Bool) -> Void
    /// Returns the neccessary web loader for handling reloading streams
    public var webLoaderFactory: () -> PlaylistWebLoaderFactory
    /// Called when playlist is dismissed
    public var onDismissal: () -> Void

    public init(
      openTabURL: @escaping (URL, _ isPrivate: Bool) -> Void,
      webLoaderFactory: @escaping () -> PlaylistWebLoaderFactory,
      onDismissal: @escaping () -> Void
    ) {
      self.openTabURL = openTabURL
      self.webLoaderFactory = webLoaderFactory
      self.onDismissal = onDismissal
    }
  }

  private var delegate: Delegate

  public init(delegate: Delegate) {
    self.delegate = delegate
  }

  public var body: some View {
    PlaylistContentView()
      .preparePlaylistEnvironment()
      .prepareMediaStreamer(webLoaderFactory: delegate.webLoaderFactory())
      .environment(\.managedObjectContext, DataController.swiftUIContext)
      .environment(\.colorScheme, .dark)
      .preferredColorScheme(.dark)
      .environment(
        \.openTabURL,
        .init { url, isPrivate in
          delegate.openTabURL(url, isPrivate)
        }
      )
      .onDisappear {
        delegate.onDismissal()
      }
  }
}

@available(iOS 16.0, *)
struct PlaylistContentView: View {
  @Environment(\.loadMediaStreamingAsset) private var loadMediaStreamingAsset

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

  @MainActor private func playItem(_ item: PlaylistItem) async {
    // Shrink it down?
    withAnimation(.snappy) {
      selectedDetent = .small
    }
    // FIXME: Need a possible loading state here
    // Loading a media streaming asset may take time, need some sort of intermediate state where
    // we stop the current playback and show a loader even if the selected item is not fully ready
    // yet
    let itemToReplace: AVPlayerItem? = await {
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
    if let item = itemToReplace {
      playerModel.item = item
      playerModel.play()
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
            if selectedItemID == newValue {
              // Already selected, restart it (based on prior behaviour)
              withAnimation(.snappy) {
                selectedDetent = .small
              }
              Task {
                await playerModel.seek(to: 0, accurately: true)
                playerModel.play()
              }
            }
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
      makeItemQueue(selectedItemID: nil)
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
      Task {
        await playItem(item)
      }
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
class PreviewWebLoaderFactory: PlaylistWebLoaderFactory {
  class PreviewWebLoader: UIView, PlaylistWebLoader {
    func load(url: URL) async -> PlaylistInfo? { return nil }
    func stop() {}
  }
  func makeWebLoader() -> any PlaylistWebLoader {
    PreviewWebLoader()
  }
}
// swift-format-ignore
@available(iOS 16.0, *)
#Preview {
  PlaylistRootView(
    delegate: .init(
      openTabURL: { _, _ in },
      webLoaderFactory: { PreviewWebLoaderFactory() },
      onDismissal: { }
    )
  )
}
#endif
