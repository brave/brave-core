// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Playlist
import Strings
import SwiftUI

private struct FolderEditActionsModifier: ViewModifier {
  var folder: PlaylistFolder?

  @Binding var isDeletePlaylistConfirmationPresented: Bool
  @Binding var isRenamePlaylistAlertPresented: Bool

  @State private var renameText: String = ""

  func body(content: Content) -> some View {
    content
      .onChange(of: isRenamePlaylistAlertPresented) { newValue in
        if newValue {
          renameText = folder?.title ?? ""
        } else {
          renameText = ""
        }
      }
      .confirmationDialog(
        Strings.Playlist.deletePlaylistConfirmationMessage,
        isPresented: $isDeletePlaylistConfirmationPresented,
        titleVisibility: .visible
      ) {
        Button(role: .destructive) {
          if let folder {
            Task {
              await PlaylistManager.shared.delete(folder: folder)
            }
          }
        } label: {
          Text(Strings.Playlist.deletePlaylist)
        }
        Button(role: .cancel) {
        } label: {
          Text(Strings.CancelString)
        }
        .keyboardShortcut(.cancelAction)
      }
      .alert(Strings.Playlist.renamePlaylist, isPresented: $isRenamePlaylistAlertPresented) {
        TextField("", text: $renameText)
        Button(Strings.CancelString, role: .cancel) {
          // No action
        }
        .keyboardShortcut(.cancelAction)
        Button {
          if let folder {
            PlaylistFolder.updateFolder(folderID: folder.objectID) { result in
              if case .success(let folder) = result {
                folder.title = renameText
              }
            }
          }
        } label: {
          Text(Strings.Playlist.saveButtonTitle)
        }
        .keyboardShortcut(.defaultAction)
      }
  }
}

extension View {
  func editActions(
    for folder: PlaylistFolder?,
    isDeletePlaylistConfirmationPresented: Binding<Bool>,
    isRenamePlaylistAlertPresented: Binding<Bool>
  ) -> some View {
    modifier(
      FolderEditActionsModifier(
        folder: folder,
        isDeletePlaylistConfirmationPresented: isDeletePlaylistConfirmationPresented,
        isRenamePlaylistAlertPresented: isRenamePlaylistAlertPresented
      )
    )
  }
}
