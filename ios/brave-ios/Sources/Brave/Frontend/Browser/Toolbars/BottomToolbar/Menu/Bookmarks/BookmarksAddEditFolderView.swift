//
//  BookmarksAddEditFolderView.swift
//  Brave
//
//  Created by Brandon T on 2025-05-12.
//

import BraveStrings
import DesignSystem
import Favicon
import SwiftUI

struct BookmarksAddEditFolderView: View {
  @Environment(\.dismiss)
  private var dismiss

  @ObservedObject
  private var model: BookmarkModel

  @State
  private var folder: Bookmarkv2?

  @State
  private var selectedParentFolder: Bookmarkv2?

  @State
  private var title: String

  @State
  private var isExpanded = false

  @State
  private var folders: [(folder: Bookmarkv2, indentationLevel: Int)] = []

  private var defaultRootFolder: Bookmarkv2? {
    model.mobileBookmarksFolder
  }

  init(model: BookmarkModel, folder: Bookmarkv2? = nil) {
    self.model = model
    self.folder = folder
    self.selectedParentFolder = folder?.parent
    self.title = folder?.title ?? Strings.newFolderDefaultName
  }

  var body: some View {
    List {
      Section {
        TextField("", text: $title, prompt: Text(Strings.bookmarkTitlePlaceholderText))
      }

      Section {
        if isExpanded {
          ForEach(folders, id: \.folder.id) { indentedFolder in
            Button {
              selectedParentFolder = indentedFolder.folder
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
    .navigationTitle(folder == nil ? Strings.newFolderTitle : Strings.editFolderTitle)
    .toolbar {
      ToolbarItemGroup(placement: .topBarTrailing) {
        Button(Strings.saveButtonTitle) {
          if folder == nil {
            model.addFolder(title: title, in: selectedParentFolder ?? defaultRootFolder)
          } else {
            folder?.bookmarkNode.setTitle(title)
          }

          dismiss()
        }
      }
    }
    .onAppear {
      Task {
        self.folders = await model.folders(excluding: self.folder).map({
          return ($0, $0.indentationLevel)
        })
      }
    }
  }
}
