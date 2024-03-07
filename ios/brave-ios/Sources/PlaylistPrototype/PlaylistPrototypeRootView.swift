// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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
  @State private var selectedDetent: PlaylistSheetDetent = .small

  private var selectedFolder: PlaylistFolder? {
    folders.first(where: { $0.id == selectedFolderID })
  }

  public var body: some View {
    PlaylistSplitView(selectedDetent: $selectedDetent) {
      PlaylistSidebarList(folderID: selectedFolderID)
    } sidebarHeader: {
      if let selectedFolder = selectedFolder {
        PlaylistSidebarListHeader(
          playerModel: playerModel,
          folders: Array(folders),
          selectedFolder: Binding(
            get: {
              selectedFolder
            },
            set: {
              selectedFolderID = $0.id
            }
          )
        )
      }
    } content: {
      // FIXME: Swap out for some sort of container for the selected item (shows different views if its webpage TTS for instance)
      MediaContentView(model: playerModel)
        .playlistSheetDetents([.small, .anchor(.mediaPlayer), .large])
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
