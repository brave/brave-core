// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStrings
import Data
import DesignSystem
import Favicon
import Preferences
import Shared
import SwiftUI

/// View Model that holds all the information required to render a list of bookmarks
private class BookmarkViewModel: ObservableObject {
  @ObservedObject
  private var model: BookmarkModel

  private var listener: BookmarkModelListener?

  private(set) var currentSearchQuery: String?

  @Published
  private(set) var folder: Bookmarkv2?

  @Published
  private(set) var items = [Bookmarkv2]()

  @Published
  private(set) var isLoading = false

  init(model: BookmarkModel, folder: Bookmarkv2?) {
    self.model = model
    self.folder = folder

    listener = model.addListener(
      .init({ event in
        Task { [weak self] in
          guard let self = self else { return }
          await self.refresh(query: self.currentSearchQuery)
        }
      })
    )
  }

  deinit {
    listener?.destroy()
  }

  func refresh() async {
    await refresh(query: currentSearchQuery)
  }

  @MainActor
  func refresh(query: String?) async {
    isLoading = true
    currentSearchQuery = query
    items = await model.bookmarks(for: folder, query: query)
    isLoading = false
  }
}

private struct BookmarkItemView: View {
  var bookmark: Bookmarkv2

  var url: URL? {
    if let urlString = bookmark.url, let url = URL(string: urlString) {
      return url
    }
    return nil
  }

  var body: some View {
    Label {
      Text(bookmark.title ?? bookmark.url ?? "")
        .font(bookmark.isFolder ? .subheadline.bold() : .subheadline)
        .lineLimit(1)
        .truncationMode(.tail)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
    } icon: {
      Group {
        if bookmark.isFolder {
          Image(braveSystemName: "leo.folder")
            .resizable()
            .aspectRatio(contentMode: .fit)
            .foregroundStyle(Color(braveSystemName: .iconDefault))
            .padding(6.0)
        } else if let url = url {
          FaviconImage(url: url, isPrivateBrowsing: false)
            .overlay(
              ContainerRelativeShape()
                .strokeBorder(Color(braveSystemName: .dividerSubtle), lineWidth: 1.0)
            )
        } else {
          Image(uiImage: Favicon.defaultImage)
            .resizable()
            .aspectRatio(contentMode: .fit)
            .foregroundStyle(Color(braveSystemName: .iconDefault))
            .padding(6.0)
        }
      }
      .containerShape(RoundedRectangle(cornerRadius: 6.0, style: .continuous))
      .clipShape(RoundedRectangle(cornerRadius: 6.0, style: .continuous))
    }
  }
}

private struct BookmarksListView: View {

  @Environment(\.dismiss)
  private var dismiss

  @ObservedObject
  var model: BookmarkModel

  @ObservedObject
  var viewModel: BookmarkViewModel

  @State private var showCreateFolder = false
  @State private var createFolderName = ""

  @State private var editMode = EditMode.inactive

  @State private var searchText = ""
  @State private var timer: Timer?

  @ViewBuilder
  private func view(for node: Bookmarkv2) -> some View {
    if node.isFolder {
      NavigationLink(value: node) {
        BookmarkItemView(bookmark: node)
      }
    } else {
      Button {
        model.handleBookmarkItemSelection(
          model.isPrivateBrowsing ? .openInNewTab : .openInNewPrivateTab,
          node: node
        )
      } label: {
        BookmarkItemView(bookmark: node)
      }
    }
  }

  var body: some View {
    List {
      ForEach(viewModel.items) { node in
        view(for: node)
          .deleteDisabled(!node.canBeDeleted)
          .moveDisabled(!node.canBeDeleted)
          .swipeActions(edge: .trailing) {
            if node.canBeDeleted {
              Button(role: .destructive) {
                model.delete(nodes: [node])
              } label: {
                Text(Strings.delete)
              }

              NavigationLink {
                if node.isFolder {
                  BookmarksAddEditFolderView(model: model, folder: node)
                } else {
                  BookmarksAddEditBookmarkView(model: model, node: node)
                }
              } label: {
                Text(Strings.edit)
              }
              .tint(.indigo)
            } else {
              EmptyView()
            }
          }
          .listRowBackground(Color.clear)
          .contextMenu {
            Section {
              Button {
                model.handleBookmarkItemSelection(.openInNewTab, node: node)
              } label: {
                Label(Strings.openNewTabButtonTitle, systemImage: "plus.square.on.square")
              }

              if !model.isPrivateBrowsing {
                Button {
                  model.handleBookmarkItemSelection(.openInNewPrivateTab, node: node)
                } label: {
                  Label(
                    Strings.openNewPrivateTabButtonTitle,
                    systemImage: "plus.square.fill.on.square.fill"
                  )
                }
              }
            }
            Section {
              Button {
                model.handleBookmarkItemSelection(.copyLink, node: node)
              } label: {
                Label(Strings.copyLinkActionTitle, systemImage: "doc.on.doc")
              }

              Button {
                model.handleBookmarkItemSelection(.shareLink, node: node)
              } label: {
                Label(Strings.shareLinkActionTitle, systemImage: "square.and.arrow.up")
              }
            }
            Section {
              Button(role: .destructive) {
                withAnimation {
                  model.delete(nodes: [node])
                }
              } label: {
                Label(Strings.History.deleteFromHistory, braveSystemImage: "leo.trash")
              }
            }
          }
      }
      .onMove { (indexSet, newOffset) in
        let nodes = viewModel.items
        let nodesToMove = indexSet.map({ nodes[$0] })
        model.move(nodes: nodesToMove, to: newOffset)
      }
      .onDelete { indexSet in
        withAnimation {
          let nodes = viewModel.items
          let nodesToDelete = indexSet.map({ nodes[$0] })
          model.delete(nodes: nodesToDelete)
        }
      }
    }
    .scrollContentBackground(.hidden)
    .background(Color(.braveGroupedBackground))
    .listStyle(.plain)
    .environment(\.editMode, $editMode)
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(viewModel.folder?.title ?? Strings.bookmarks)
    .toolbarBackground(.visible, for: .navigationBar)
    .overlay {
      if viewModel.items.isEmpty {
        BookmarksEmptyStateView(isSearching: !searchText.isEmpty)
          .transition(.opacity.animation(.default))
      }
    }
    .toolbar {
      ToolbarItemGroup(placement: .topBarTrailing) {
        Button {
          dismiss()
        } label: {
          Text(Strings.done)
        }
      }
    }
    .toolbar {
      ToolbarItemGroup(placement: .bottomBar) {
        HStack {
          Button {
            if viewModel.folder == nil {
              // TODO: Share - Import/Export
            } else {
              showCreateFolder = true
            }
          } label: {
            Label {
              Text("CREATE NEW BOOKMARKS FOLDER")  // TODO: Localize
            } icon: {
              Image(braveSystemName: viewModel.folder == nil ? "leo.share.macos" : "leo.folder.new")
                .foregroundStyle(Color(braveSystemName: .iconInteractive))
            }
            .labelStyle(.iconOnly)
          }

          Spacer()

          if viewModel.folder != nil {
            Button {
              editMode = editMode.isEditing ? .inactive : .active
            } label: {
              Label {
                Text("EDIT BOOKMARKS")  // TODO: Localize
              } icon: {
                Image(braveSystemName: "leo.edit.pencil")
                  .foregroundStyle(Color(braveSystemName: .iconInteractive))
              }
              .labelStyle(.iconOnly)
            }
          }
        }
        .frame(maxWidth: .infinity)
      }
    }
    .alert(
      Strings.newFolder,
      isPresented: $showCreateFolder,
      actions: {
        TextField(Strings.name, text: $createFolderName)

        Button(Strings.CancelString) {
          showCreateFolder = false
          createFolderName = ""
        }

        Button(Strings.OBErrorOkay) {
          model.addFolder(title: createFolderName, in: viewModel.folder)
          createFolderName = ""
        }
        .disabled(createFolderName.isEmpty)
      },
      message: {
        Text(Strings.enterFolderName)
      }
    )
    .onChange(of: searchText) { searchText in
      self.timer?.invalidate()
      self.timer = Timer.scheduledTimer(
        withTimeInterval: 0.1,
        repeats: false,
        block: { timer in
          timer.invalidate()

          Task { [weak viewModel] in
            await viewModel?.refresh(query: searchText)
          }
        }
      )
    }
    .searchable(
      text: $searchText,
      placement: .navigationBarDrawer(displayMode: .always),
      prompt: Strings.searchBookmarksTitle
    )
    .onAppear {
      Task {
        await viewModel.refresh()
      }
    }
  }
}

struct BookmarksView: View {

  @ObservedObject
  var model: BookmarkModel

  @State
  private var currentStack: [Bookmarkv2] = []

  //  private let importExportUtility = BookmarksImportExportUtility()

  var body: some View {
    NavigationStack(path: $currentStack) {
      BookmarksListView(
        model: model,
        viewModel: BookmarkViewModel(model: model, folder: nil)
      )
      .navigationDestination(for: Bookmarkv2.self) { node in
        BookmarksListView(
          model: model,
          viewModel: BookmarkViewModel(model: model, folder: node)
        )
      }
    }
    .onAppear {
      var stack: [Bookmarkv2] = []
      var current = model.lastVisitedFolder?.parent

      while let folder = current {
        stack.append(folder)
        current = folder.parent
      }

      currentStack = stack
    }
  }
}
