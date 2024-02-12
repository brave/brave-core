// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Data
import CoreData
import Shared
import BraveShared
import BraveUI

struct PlaylistEditFolderView: View {
  @State private var folderName: String
  private var currentFolder: NSManagedObjectID
  private let currentFolderTitle: String
  private var isEditDisabled: Bool {
    folderName.isEmpty || currentFolderTitle == folderName
  }

  var onCancelButtonPressed: (() -> Void)?
  var onEditFolder: ((_ folderTitle: String) -> Void)?

  init(currentFolder: NSManagedObjectID, currentFolderTitle: String) {
    self.currentFolder = currentFolder
    self.currentFolderTitle = currentFolderTitle
    self.folderName = currentFolderTitle
  }

  var body: some View {
    NavigationView {
      List {
        Section {
          TextField(currentFolderTitle, text: $folderName)
            .disableAutocorrection(true)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .listStyle(.insetGrouped)
      .navigationTitle(Strings.PlaylistFolders.playlistEditFolderScreenTitle)
      .navigationBarTitleDisplayMode(.inline)
      .navigationViewStyle(.stack)
      .toolbar {
        ToolbarItem(placement: .cancellationAction) {
          Button(Strings.cancelButtonTitle) { onCancelButtonPressed?() }
            .foregroundColor(.white)
        }

        ToolbarItem(placement: .confirmationAction) {
          Button(Strings.saveButtonTitle) { onEditFolder?(folderName) }
            .foregroundColor(isEditDisabled ? Color(.braveDisabled) : .white)
            .disabled(isEditDisabled)
        }
      }
    }
    .environment(\.colorScheme, .dark)
  }
}

#if DEBUG
struct PlaylistEditFolderView_Previews: PreviewProvider {
  static var previews: some View {
    PlaylistEditFolderView(currentFolder: .init(), currentFolderTitle: "Folder")
  }
}
#endif
