//
//  BookmarksAddEditBookmarkView.swift
//  Brave
//
//  Created by Brandon T on 2025-05-12.
//

import BraveCore
import BraveStrings
import Data
import DesignSystem
import Favicon
import Introspect
import SwiftUI

private enum BookmarksSaveFolder {
  case folder(Bookmarkv2)
  case favorites
  case mobileBookmarks
}

struct BookmarksAddEditBookmarkView: View {
  @Environment(\.dismiss)
  private var dismiss

  @ObservedObject
  private var model: BookmarkModel

  @State
  private var node: Bookmarkv2?

  @State
  private var title: String

  @State
  private var urlString: String

  @State
  private var saveFolder: BookmarksSaveFolder = .mobileBookmarks

  @FocusState
  private var isURLFieldFocused: Bool

  @State
  private var isExpanded = false

  @State
  private var folders: [(folder: Bookmarkv2, indentationLevel: Int)] = []

  @State
  private var urlTextField: UITextField?

  private var bookmarkURL: URL? {
    guard let urlString = node?.url else {
      return nil
    }

    return URL(string: urlString) ?? URL.bookmarkletURL(from: urlString)
  }

  private var defaultRootFolder: Bookmarkv2? {
    model.mobileBookmarksFolder
  }

  init(model: BookmarkModel, node: Bookmarkv2? = nil) {
    self.model = model
    self.node = node
    self.title = node?.title ?? ""
    self.urlString = node?.url ?? ""
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
              .onChange(of: isURLFieldFocused) { isFocused in
                guard let urlTextField = urlTextField else {
                  return
                }

                // When the URL field gains focus, move the cursor to position 0.
                if isFocused {
                  urlString = bookmarkURL?.absoluteString ?? ""

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

          ForEach(folders, id: \.folder.id) { indentedFolder in
            Button {
              if indentedFolder.folder.id == defaultRootFolder?.id {
                saveFolder = .mobileBookmarks
              } else {
                saveFolder = .folder(indentedFolder.folder)
              }

              isExpanded.toggle()
            } label: {
              HStack {
                let hasChildFolders = indentedFolder.folder.children.contains(where: {
                  $0.isFolder
                })

                Image(
                  braveSystemName: indentedFolder.folder.parent == nil
                    ? "leo.product.bookmarks" : hasChildFolders ? "leo.folder.open" : "leo.folder"
                )
                Text(indentedFolder.folder.title ?? "")
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

          dismiss()
        }
      }
    }
    .onAppear {
      self.urlString = URLFormatter.formatURLOrigin(
        forDisplayOmitSchemePathAndTrivialSubdomains: bookmarkURL?.strippingBlobURLAuth
          .absoluteString ?? urlString
      )

      Task {
        self.folders = await model.folders().map({
          return ($0, $0.indentationLevel)
        })
      }
    }
  }

  private func locationView() -> some View {
    switch saveFolder {
    case .folder(let folder):
      return HStack {
        Image(braveSystemName: "leo.folder")
        Text(folder.title ?? "")
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

  private func addOrUpdateBookmark(parent: Bookmarkv2?) {
    if let url = URL(string: urlString) ?? URL.bookmarkletURL(from: urlString) {
      if let node = node {
        node.bookmarkNode.setTitle(title)
        node.bookmarkNode.url = url

        if let parent = parent, node.parent?.id != parent.id {
          node.bookmarkNode.move(toParent: parent.bookmarkNode)
        }
      } else {
        model.addBookmark(url: url, title: title, in: parent)
      }
    }
  }
}
