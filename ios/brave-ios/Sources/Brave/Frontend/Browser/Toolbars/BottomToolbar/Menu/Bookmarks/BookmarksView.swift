// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveStrings
import Data
import DataImporter
import DesignSystem
import Favicon
import Preferences
import Shared
import SwiftUI
import UniformTypeIdentifiers

private class BookmarkFile: ReferenceFileDocument {
  typealias Snapshot = BookmarkFile
  static var readableContentTypes: [UTType] { [.html] }
  static var writableContentTypes: [UTType] { [.html] }

  let fileURL: URL

  init(fileURL: URL) {
    self.fileURL = fileURL
  }

  required init(configuration: ReadConfiguration) throws {
    throw CocoaError(.fileReadUnsupportedScheme)
  }

  func snapshot(contentType: UTType) throws -> Snapshot {
    return self
  }

  func fileWrapper(snapshot: Snapshot, configuration: WriteConfiguration) throws -> FileWrapper {
    return try FileWrapper(url: snapshot.fileURL, options: [])
  }
}

/// View Model that holds all the information required to render a list of bookmarks
private class BookmarkViewModel: ObservableObject {

  private var model: BookmarkModel

  private var listener: BookmarkModelListener?

  private(set) var currentSearchQuery: String?

  @Published
  private(set) var folder: BookmarkNode?

  @Published
  private(set) var items = [BookmarkNode]()

  @Published
  private(set) var isLoading = false

  init(model: BookmarkModel, folder: BookmarkNode?) {
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
  var bookmark: BookmarkNode

  var body: some View {
    Label {
      Text(bookmark.title)
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
        } else if let url = bookmark.url {
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

  var model: BookmarkModel

  @StateObject
  var viewModel: BookmarkViewModel

  @Binding
  var dismiss: Bool

  @State private var showShare = false
  @State private var showImport = false
  @State private var showExport = false
  @State private var importExportResult: ImportExportResult = .init(
    showAlert: false,
    title: "",
    message: ""
  )
  @State private var showCreateFolder = false
  @State private var createFolderName = ""

  @State private var editMode = EditMode.inactive

  @State private var searchText = ""
  @State private var timer: Timer?

  @ViewBuilder
  private func view(for node: BookmarkNode) -> some View {
    if node.isFolder {
      NavigationLink(value: node) {
        BookmarkItemView(bookmark: node)
      }
    } else {
      Button {
        model.handleBookmarkItemSelection(
          model.isPrivateBrowsing ? .openInNewPrivateTab : .openInNewTab,
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
                  BookmarksAddEditFolderView(model: model, action: .modifyFolder(node))
                } else {
                  BookmarksAddEditBookmarkView(model: model, action: .existing(node))
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
            if node.isFolder {
              if node.children.isEmpty {
                EmptyView()
              } else {
                Button {
                  model.openAllBookmarks(node: node)
                } label: {
                  Label(
                    String(format: Strings.openAllBookmarks, model.bookmarks(in: node).count),
                    systemImage: "plus.square.on.square"
                  )
                }
              }
            } else {
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
      }
    }
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        Button {
          dismiss = true
        } label: {
          Text(Strings.done)
        }
      }

      ToolbarItemGroup(placement: .bottomBar) {
        Button {
          if viewModel.folder == nil {
            showShare = true
          } else {
            showCreateFolder = true
          }
        } label: {
          Label {
            Text(Strings.newFolderTitle)
          } icon: {
            Image(braveSystemName: viewModel.folder == nil ? "leo.share.macos" : "leo.folder.new")
              .foregroundStyle(Color(braveSystemName: .iconInteractive))
          }
          .labelStyle(.iconOnly)
        }

        Spacer()

        if viewModel.folder != nil && !viewModel.items.isEmpty {
          Button {
            editMode = editMode.isEditing ? .inactive : .active
          } label: {
            Label {
              Text(Strings.editBookmarkTitle)
            } icon: {
              Image(braveSystemName: "leo.edit.pencil")
                .foregroundStyle(Color(braveSystemName: .iconInteractive))
            }
            .labelStyle(.iconOnly)
          }
        }
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
    .confirmationDialog(
      "",
      isPresented: $showShare,
      actions: {
        Button {
          showImport = true
        } label: {
          Text(Strings.bookmarksImportAction)
        }

        Button {
          Task { @MainActor in
            let exporter = BookmarksImportExportUtility()
            if await exporter.exportBookmarks(to: bookmarkFile.fileURL) {
              showExport = true
            } else {

            }
          }
        } label: {
          Text(Strings.bookmarksExportAction)
        }
      }
    )
    .fileImporter(
      isPresented: $showImport,
      allowedContentTypes: [.html],
      allowsMultipleSelection: false,
      onCompletion: {
        switch $0 {
        case .success(let urls):
          Task { @MainActor in
            let importer = BookmarksImportExportUtility()
            if let url = urls.first, await importer.importBookmarks(from: url) {
              importExportResult = .init(
                showAlert: true,
                title: Strings.Bookmarks.bookmarksImportExportSuccessTitle,
                message: Strings.Bookmarks.bookmarksImportSuccessMessage
              )
            } else {
              importExportResult = .init(
                showAlert: true,
                title: Strings.Bookmarks.bookmarksImportExportErrorTitle,
                message: Strings.Bookmarks.bookmarksImportErrorMessage
              )
            }
          }
        case .failure(let error):
          print(error)
          importExportResult = .init(
            showAlert: true,
            title: Strings.Bookmarks.bookmarksImportExportErrorTitle,
            message: Strings.Bookmarks.bookmarksImportErrorMessage
          )
        }
      }
    )
    .fileExporter(
      isPresented: $showExport,
      document: bookmarkFile,
      contentType: .html,
      onCompletion: {
        switch $0 {
        case .success(_):
          importExportResult = .init(
            showAlert: true,
            title: Strings.Bookmarks.bookmarksImportExportSuccessTitle,
            message: Strings.Bookmarks.bookmarksExportSuccessMessage
          )
        case .failure(let error):
          print(error)
          importExportResult = .init(
            showAlert: true,
            title: Strings.Bookmarks.bookmarksImportExportErrorTitle,
            message: Strings.Bookmarks.bookmarksExportErrorMessage
          )
        }
      }
    )
    .alert(
      importExportResult.title,
      isPresented: $importExportResult.showAlert,
      actions: {
        Button {

        } label: {
          Text(Strings.OBErrorOkay)
        }
      },
      message: {
        Text(importExportResult.message)
      }
    )
    .onChange(of: searchText) { _, searchText in
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

  private var bookmarkFile: BookmarkFile {
    BookmarkFile(
      fileURL: FileManager.default.temporaryDirectory
        .appendingPathComponent(
          Strings.bookmarks.addingPercentEncoding(withAllowedCharacters: .urlPathAllowed)
            ?? "Bookmarks"
        )
        .appendingPathExtension("html")
    )
  }

  private struct ImportExportResult {
    var showAlert: Bool
    var title: String
    var message: String
  }
}

struct BookmarksView: View {

  @Environment(\.dismiss)
  private var dismiss

  var model: BookmarkModel

  @State
  private var currentStack: [BookmarkNode] = []

  @State
  private var dismissOnPop = false

  var body: some View {
    NavigationStack(path: $currentStack) {
      BookmarksListView(
        model: model,
        viewModel: BookmarkViewModel(model: model, folder: nil),
        dismiss: $dismissOnPop
      )
      .navigationDestination(for: BookmarkNode.self) { node in
        BookmarksListView(
          model: model,
          viewModel: BookmarkViewModel(model: model, folder: node),
          dismiss: $dismissOnPop
        )
      }
    }
    .task {
      var stack: [BookmarkNode] = []
      var current = model.lastVisitedFolder

      while let folder = current {
        stack.append(folder)
        current = folder.parentNode
      }

      currentStack = stack.reversed()
    }
    .onChange(of: dismissOnPop) { _, newValue in
      if newValue {
        currentStack = []
        dismiss()
      }
    }
    .onChange(of: currentStack) { _, currentStack in
      if !currentStack.isEmpty {
        Preferences.Chromium.lastBookmarksFolderNodeId.value = currentStack.last?.id ?? -1
      }
    }
  }
}
