// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import DesignSystem
import Favicon
import SwiftUI
import Web

struct BookmarksAddEditFolderView: View {
  @Environment(\.dismiss)
  private var dismiss

  private var model: BookmarkModel

  private var action: Action

  @State
  private var selectedParentFolder: BookmarkNode?

  @State
  private var title: String

  @State
  private var isExpanded = false

  @State
  private var folders: [BookmarkFolder] = []

  private var defaultRootFolder: BookmarkNode? {
    model.mobileBookmarksFolder
  }

  private var manuallyDismiss: (() -> Void)?

  enum Action {
    case modifyFolder(_ folder: BookmarkNode?)
    case addTabs(_ tabs: [any TabState])

    var title: String {
      switch self {
      case .modifyFolder(let folder):
        return folder?.title ?? Strings.newFolderDefaultName
      case .addTabs:
        return Strings.savedTabsFolderTitle
      }
    }

    var navigationBarTitle: String {
      switch self {
      case .modifyFolder(let folder):
        return folder == nil ? Strings.newFolderTitle : Strings.editFolderTitle
      case .addTabs:
        return Strings.newFolderTitle
      }
    }

    func defaultSelectedParentFolder(for model: BookmarkModel) -> BookmarkNode? {
      switch self {
      case .modifyFolder(let folder):
        return folder?.parent
      case .addTabs:
        return model.mobileBookmarksFolder
      }
    }
  }

  init(model: BookmarkModel, action: Action, dismiss: (() -> Void)? = nil) {
    self.model = model
    self.action = action
    self.manuallyDismiss = dismiss
    self._selectedParentFolder = .init(initialValue: action.defaultSelectedParentFolder(for: model))
    self._title = .init(initialValue: action.title)
  }

  var body: some View {
    List {
      Section {
        if case .modifyFolder = action {
          TextField("", text: $title, prompt: Text(Strings.bookmarkTitlePlaceholderText))
        } else {
          TextField("", text: $title, prompt: Text(title))
        }
      }

      Section {
        if isExpanded {
          ForEach(folders, id: \.bookmarkNode.id) { indentedFolder in
            Button {
              selectedParentFolder = indentedFolder.bookmarkNode
              isExpanded.toggle()
            } label: {
              Label {
                Text(indentedFolder.bookmarkNode.title)
              } icon: {
                let hasChildFolders = indentedFolder.bookmarkNode.children.contains(where: {
                  $0.isFolder
                })

                Image(
                  braveSystemName: indentedFolder.bookmarkNode.parentNode == nil
                    ? "leo.product.bookmarks" : hasChildFolders ? "leo.folder.open" : "leo.folder"
                )
              }
              .padding(.leading, CGFloat(indentedFolder.indentationLevel) * 16.0)
            }
            .foregroundColor(Color(braveSystemName: .iconDefault))
          }
        } else {
          Button {
            isExpanded.toggle()
          } label: {
            Label {
              Text(
                selectedParentFolder?.title ?? defaultRootFolder?.title
                  ?? Strings.bookmarkRootLevelCellTitle
              )
            } icon: {
              Image(
                braveSystemName: selectedParentFolder == nil
                  ? "leo.product.bookmarks" : "leo.folder"
              )
            }
          }
          .foregroundColor(Color(braveSystemName: .iconDefault))
        }
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .listStyle(.grouped)
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(action.navigationBarTitle)
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        Button(Strings.saveButtonTitle) {
          switch action {
          case .modifyFolder(let folder):
            if folder == nil {
              model.addFolder(title: title, in: selectedParentFolder ?? defaultRootFolder)
            } else {
              folder?.title = title
            }

          case .addTabs(let tabs):
            if let newFolder = model.addFolder(
              title: title,
              in: selectedParentFolder ?? defaultRootFolder
            ) {
              for tab in tabs {
                if let url = tab.visibleURL, url.isWebPage() {
                  model.addBookmark(url: url, title: tab.title, in: newFolder)
                }
              }
            }
          }

          manuallyDismiss?() ?? dismiss()
        }
      }
    }
    .task {
      switch action {
      case .modifyFolder(let folder):
        self.folders = await model.folders(excluding: folder)
      case .addTabs:
        self.folders = await model.folders()
      }
    }
  }
}
