// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct PlaylistDebugFilesView: View {
  @Environment(\.dismiss) private var dismiss
  @State private var files: [URL] = []
  @State private var errorMessage: String?
  @State private var deleteError: String?
  @State private var shouldDeleteAll = false
  var body: some View {
    NavigationStack {
      Group {
        if let errorMessage {
          ContentUnavailableView(
            "Couldn't load files",
            systemImage: "exclamationmark.triangle",
            description: Text(errorMessage)
          )
        } else if files.isEmpty {
          ContentUnavailableView(
            "No files",
            systemImage: "tray",
            description: Text("The Playlist folder is empty.")
          )
        } else {
          List {
            ForEach(files, id: \.self) { url in
              HStack {
                Image(systemName: "doc")
                VStack(alignment: .leading) {
                  Text(url.lastPathComponent)
                  if let size = fileSize(url) {
                    Text(size)
                      .font(.caption)
                      .foregroundStyle(.secondary)
                  }
                }
              }
              .swipeActions(edge: .trailing, allowsFullSwipe: true) {
                Button(role: .destructive) {
                  delete(url)
                } label: {
                  Label("Delete", systemImage: "trash")
                }
              }
            }
            .onDelete(perform: deleteAt)
          }
        }
      }
      .navigationTitle("Playlist Files")
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItem(placement: .cancellationAction) {
          Button("Done") { dismiss() }
        }
        ToolbarItem(placement: .primaryAction) {
          Button {
            loadFiles()
          } label: {
            Image(systemName: "arrow.clockwise")
          }
        }

        ToolbarItem(placement: .bottomBar) {
          Spacer()
        }
        ToolbarItem(placement: .bottomBar) {
          Button {
            shouldDeleteAll = true
          } label: {
            Label("Delete All", systemImage: "trash")
              .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorIcon))
          }
          .confirmationDialog(
            "Delete all files?",
            isPresented: $shouldDeleteAll,
            titleVisibility: .visible
          ) {
            Button("Delete \(files.count) File\(files.count == 1 ? "" : "s")", role: .destructive) {
              deleteAll()
            }
            Button("Cancel", role: .cancel) {}
          } message: {
            Text(
              "This will permanently remove every file in the Playlist folder. This action cannot be undone."
            )
          }
        }
      }
      .onAppear(perform: loadFiles)
      .alert(
        "Couldn't delete file",
        isPresented: Binding(
          get: { deleteError != nil },
          set: { if !$0 { deleteError = nil } }
        ),
        presenting: deleteError
      ) { _ in
        Button("OK", role: .cancel) {}
      } message: { message in
        Text(message)
      }
    }
  }

  private func loadFiles() {
    do {
      let fm = FileManager.default
      let appSupport = try fm.url(
        for: .applicationSupportDirectory,
        in: .userDomainMask,
        appropriateFor: nil,
        create: true
      )
      let playlistURL = appSupport.appendingPathComponent("Playlist", isDirectory: true)

      if !fm.fileExists(atPath: playlistURL.path) {
        try fm.createDirectory(at: playlistURL, withIntermediateDirectories: true)
      }

      let contents = try fm.contentsOfDirectory(
        at: playlistURL,
        includingPropertiesForKeys: [.isRegularFileKey, .fileSizeKey],
        options: [.skipsHiddenFiles]
      )

      files = contents.sorted {
        $0.lastPathComponent.localizedCompare($1.lastPathComponent) == .orderedAscending
      }
      errorMessage = nil
    } catch {
      errorMessage = error.localizedDescription
      files = []
    }
  }

  private func fileSize(_ url: URL) -> String? {
    guard let size = try? url.resourceValues(forKeys: [.fileSizeKey]).fileSize else { return nil }
    return ByteCountFormatter.string(fromByteCount: Int64(size), countStyle: .file)
  }

  private func delete(_ url: URL) {
    do {
      try FileManager.default.removeItem(at: url)
      files.removeAll { $0 == url }
    } catch {
      deleteError = error.localizedDescription
    }
  }

  private func deleteAt(_ offsets: IndexSet) {
    for index in offsets {
      let url = files[index]
      do {
        try FileManager.default.removeItem(at: url)
      } catch {
        deleteError = error.localizedDescription
        return
      }
    }
    files.remove(atOffsets: offsets)
  }

  private func deleteAll() {
    do {
      let fm = FileManager.default
      let appSupport = try fm.url(
        for: .applicationSupportDirectory,
        in: .userDomainMask,
        appropriateFor: nil,
        create: false
      )
      let playlistURL = appSupport.appendingPathComponent("Playlist", isDirectory: true)
      try fm.removeItem(at: playlistURL)
      try fm.createDirectory(at: playlistURL, withIntermediateDirectories: true)
      files = []
    } catch {
      deleteError = error.localizedDescription
    }
  }
}

#if DEBUG
#Preview {
  PlaylistDebugFilesView()
}
#endif
