// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Data
import DesignSystem
import Favicon
import Introspect
import Shared
import SwiftUI

private enum BookmarksSaveFolder {
  case folder(BookmarkNode)
  case favorites
  case mobileBookmarks
}

struct BookmarksAddEditBookmarkView: View {

  @Environment(\.dismiss)
  private var dismiss

  @ObservedObject
  private var model: BookmarkModel

  @State
  private var action: Action

  @State
  private var title: String

  @State
  private var urlString: String

  @State
  private var oldUrlString: String

  @State
  private var saveFolder: BookmarksSaveFolder = .mobileBookmarks

  @FocusState
  private var isURLFieldFocused: Bool

  @State
  private var isExpanded = false

  @State
  private var folders: [BookmarkFolder] = []

  @State
  private var urlTextField: UITextField?

  private var bookmarkURL: URL? {
    return URL(string: oldUrlString) ?? URL.bookmarkletURL(from: oldUrlString)
  }

  private var defaultRootFolder: BookmarkNode? {
    model.mobileBookmarksFolder
  }

  private var manuallyDismiss: (() -> Void)?

  enum Action {
    case new(_ title: String, _ url: URL)
    case existing(_ bookmark: BookmarkNode)

    var title: String {
      switch self {
      case .new(let newTitle, _):
        return newTitle
      case .existing(let bookmark):
        return bookmark.title
      }
    }

    var url: URL? {
      switch self {
      case .new(_, let newURL):
        return newURL
      case .existing(let bookmark):
        return bookmark.url
      }
    }
  }

  init(model: BookmarkModel, action: Action, dismiss: (() -> Void)? = nil) {
    self.model = model
    self.manuallyDismiss = dismiss
    self._action = .init(initialValue: action)
    self._title = .init(initialValue: action.title)

    let formattedURL = URLFormatter.formatURLOrigin(
      forDisplayOmitSchemePathAndTrivialSubdomains: action.url?.absoluteString ?? ""
    )
    self._urlString = .init(
      initialValue: formattedURL.isEmpty ? action.url?.absoluteString ?? "" : formattedURL
    )
    self._oldUrlString = .init(initialValue: action.url?.absoluteString ?? "")
  }

  var body: some View {
    List {
      Section {
        HStack {
          Group {
            if let url = bookmarkURL {
              FaviconImage(url: url, isPrivateBrowsing: false)
            } else {
              Color.clear
            }
          }
          .overlay(
            ContainerRelativeShape()
              .strokeBorder(Color(braveSystemName: .dividerSubtle), lineWidth: 1.0)
          )
          .frame(width: 64.0, height: 64.0)
          .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
          .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))

          VStack {
            TextField("", text: $title, prompt: Text(Strings.bookmarkTitlePlaceholderText))

            Color(braveSystemName: .dividerSubtle)
              .frame(height: 1.0)

            TextField("", text: $urlString, prompt: Text(Strings.bookmarkUrlPlaceholderText))
              .focused($isURLFieldFocused)
              .introspectTextField { textField in
                self.urlTextField = textField
              }
              .onChange(of: isURLFieldFocused) { _, isFocused in
                guard let urlTextField = urlTextField else {
                  return
                }

                // When the URL field gains focus, move the cursor to position 0.
                if isFocused {
                  urlString = oldUrlString

                  DispatchQueue.main.asyncAfter(deadline: .now() + 0.25) {
                    let start = urlTextField.beginningOfDocument

                    UIView.animate(withDuration: 0.25) {
                      urlTextField.selectedTextRange = urlTextField.textRange(
                        from: start,
                        to: start
                      )
                    }
                  }
                } else {
                  // When the URL field loses focus, format the URL
                  let urlText = urlTextField.text ?? ""
                  oldUrlString = urlText

                  let suggestedText = URLFormatter.formatURLOrigin(
                    forDisplayOmitSchemePathAndTrivialSubdomains: urlText
                  )

                  if !suggestedText.isEmpty {
                    urlString = suggestedText
                  }
                }
              }
          }
        }
      }

      Section {
        if isExpanded {
          NavigationLink {
            BookmarksAddEditFolderView(model: model)
          } label: {
            HStack {
              Image(braveSystemName: "leo.folder.new")
              Text(Strings.newFolderTitle)
            }
          }
          .tint(Color(braveSystemName: .iconDefault))

          Button {
            saveFolder = .favorites
            isExpanded.toggle()
          } label: {
            HStack {
              Image(braveSystemName: "leo.heart.outline")
              Text(Strings.favoritesRootLevelCellTitle)
            }
          }
          .tint(Color(braveSystemName: .iconDefault))

          ForEach(folders, id: \.bookmarkNode.id) { indentedFolder in
            Button {
              if indentedFolder.bookmarkNode.id == defaultRootFolder?.id {
                saveFolder = .mobileBookmarks
              } else {
                saveFolder = .folder(indentedFolder.bookmarkNode)
              }

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
            locationView()
          }
          .tint(Color(braveSystemName: .iconDefault))
        }
      } header: {
        Text(Strings.editBookmarkTableLocationHeader)
      } footer: {
        if case .favorites = saveFolder {
          Text(Strings.favoritesLocationFooterText)
        }
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .listStyle(.grouped)
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.editBookmarkTitle)
    .toolbar {
      ToolbarItemGroup(placement: .topBarTrailing) {
        Button(Strings.saveButtonTitle) {
          switch saveFolder {
          case .folder(let folder):
            addOrUpdateBookmark(parent: folder)
          case .favorites:
            if let url = bookmarkURL {
              Favorite.add(url: url, title: title)
            }
          case .mobileBookmarks:
            addOrUpdateBookmark(parent: defaultRootFolder)
          }

          manuallyDismiss?() ?? dismiss()
        }
        .disabled(!isBookmarkValid())
      }
    }
    .onAppear {
      Task {
        self.folders = await model.folders()
      }
    }
  }

  private func locationView() -> some View {
    switch saveFolder {
    case .folder(let folder):
      return HStack {
        Image(braveSystemName: "leo.folder")
        Text(folder.title)
      }

    case .favorites:
      return HStack {
        Image(braveSystemName: "leo.heart.outline")
        Text(Strings.favoritesRootLevelCellTitle)
      }

    case .mobileBookmarks:
      return HStack {
        Image(braveSystemName: "leo.folder")
        Text(defaultRootFolder?.title ?? Strings.bookmarkRootLevelCellTitle)
      }
    }
  }

  private func addOrUpdateBookmark(parent: BookmarkNode?) {
    if let url = bookmarkURL {
      if case .existing(let node) = action {
        node.title = title
        node.url = url

        if let parent = parent, node.parentNode?.id != parent.id {
          node.move(toParent: parent)
        }
      } else {
        model.addBookmark(url: url, title: title, in: parent)
      }
    }
  }

  private func isBookmarkValid() -> Bool {
    let url = isURLFieldFocused ? urlString : oldUrlString

    if URL.bookmarkletURL(from: url) != nil {
      return BookmarkValidation.validateBookmarklet(title: title, url: url)
    }

    return BookmarkValidation.validateBookmark(title: title, url: url)
  }
}
