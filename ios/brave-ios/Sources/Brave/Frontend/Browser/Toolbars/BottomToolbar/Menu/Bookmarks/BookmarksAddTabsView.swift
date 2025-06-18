// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Data
import DesignSystem
import Shared
import SwiftUI
import Web

struct BookmarksAddTabsView: View {

  @Environment(\.dismiss)
  private var dismiss

  @ObservedObject
  private var model: BookmarkModel

  @State
  var tabs: [any TabState]

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

  init(model: BookmarkModel, tabs: [any TabState], dismiss: (() -> Void)? = nil) {
    self.model = model
    self.manuallyDismiss = dismiss
    self.tabs = tabs
    self.selectedParentFolder = model.mobileBookmarksFolder
    self.title = Strings.savedTabsFolderTitle
  }

  var body: some View {
    List {
      Section {
        TextField("", text: $title, prompt: Text(title))
      }

      Section {
        if isExpanded {
          ForEach(folders, id: \.bookmarkNode.id) { indentedFolder in
            Button {
              selectedParentFolder = indentedFolder.bookmarkNode
              isExpanded.toggle()
            } label: {
              HStack {
                let hasChildFolders = indentedFolder.bookmarkNode.children.contains(where: {
                  $0.isFolder
                })

                Image(
                  braveSystemName: indentedFolder.bookmarkNode.parentNode == nil
                    ? "leo.product.bookmarks" : hasChildFolders ? "leo.folder.open" : "leo.folder"
                )
                Text(indentedFolder.bookmarkNode.title)
              }
              .padding(.leading, CGFloat(indentedFolder.indentationLevel) * 16.0)
            }
            .tint(Color(braveSystemName: .iconDefault))
          }
        } else {
          Button {
            isExpanded.toggle()
          } label: {
            HStack {
              Image(
                braveSystemName: selectedParentFolder == nil
                  ? "leo.product.bookmarks" : "leo.folder"
              )
              Text(
                selectedParentFolder?.title ?? defaultRootFolder?.title
                  ?? Strings.bookmarkRootLevelCellTitle
              )
            }
          }
          .tint(Color(braveSystemName: .iconDefault))
        }
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .listStyle(.grouped)
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.newFolderTitle)
    .toolbar {
      ToolbarItemGroup(placement: .topBarTrailing) {
        Button(Strings.saveButtonTitle) {
          if let newFolder = model.addFolder(
            title: title,
            in: selectedParentFolder ?? defaultRootFolder
          ) {
            for tab in tabs {
              if tab.isPrivate {
                if let url = tab.visibleURL, url.isWebPage(),
                  !(InternalURL(url)?.isAboutHomeURL ?? false)
                {
                  model.addBookmark(url: url, title: tab.title, in: newFolder)
                }
              } else if let fetchedTab = SessionTab.from(tabId: tab.id), let tabURL = fetchedTab.url
              {
                if tabURL.isWebPage(), !(InternalURL(tabURL)?.isAboutHomeURL ?? false) {
                  model.addBookmark(url: tabURL, title: fetchedTab.title, in: newFolder)
                }
              }
            }
          }

          manuallyDismiss?() ?? dismiss()
        }
      }
    }
    .onAppear {
      Task {
        self.folders = await model.folders()
      }
    }
  }
}
