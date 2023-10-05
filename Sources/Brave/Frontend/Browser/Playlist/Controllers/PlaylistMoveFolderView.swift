// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Combine
import Data
import CoreData
import Shared
import BraveShared
import BraveUI
import Playlist

private struct PlaylistFolderImage: View {
  let item: PlaylistItem

  static let cornerRadius = 5.0
  private static let favIconSize = 16.0
  @StateObject private var thumbnailLoader = PlaylistFolderImageLoader()
  @StateObject private var favIconLoader = PlaylistFolderImageLoader()

  init(item: PlaylistItem) {
    self.item = item
  }

  var body: some View {
    Image(uiImage: thumbnailLoader.image ?? .init())
      .resizable()
      .aspectRatio(contentMode: .fit)
      .frame(width: 100.0, height: 60.0)
      .background(Color.black)
      .clipShape(RoundedRectangle(cornerRadius: PlaylistFolderImage.cornerRadius, style: .continuous))
      .overlay(
        Image(uiImage: favIconLoader.image ?? .init())
          .resizable()
          .aspectRatio(1.0, contentMode: .fit)
          .frame(
            width: PlaylistFolderImage.favIconSize,
            height: PlaylistFolderImage.favIconSize
          )
          .clipShape(RoundedRectangle(cornerRadius: 3.0, style: .continuous))
          .padding(8.0), alignment: .topLeading
      )
      .onAppear {
        thumbnailLoader.load(thumbnail: item)
        favIconLoader.load(favIcon: item)
      }
  }
}

private struct PlaylistFolderView: View {
  let isSourceFolder: Bool
  let folder: PlaylistFolder?
  @Binding var selectedFolder: PlaylistFolder?
  @ScaledMetric private var imageSize = 28.0

  var body: some View {
    HStack {
      HStack(spacing: 10.0) {
        Image(braveSystemName: "leo.folder")
          .foregroundColor(isSourceFolder ? Color(.braveDisabled.resolvedColor(with: .init(userInterfaceStyle: .light))) : Color(.braveBlurpleTint))
          .frame(width: imageSize)

        VStack(alignment: .leading) {
          if let folder = folder {
            let itemCount = folder.playlistItems?.count ?? 0

            Text(folder.title ?? "")
              .font(.body)
              .foregroundColor(.white)
            Text("\(itemCount == 1 ? Strings.PlaylistFolders.playlistFolderSubtitleItemSingleCount : String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderSubtitleItemCount, itemCount))")
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
          }
        }
      }
      .opacity(isSourceFolder ? 0.5 : 1.0)

      Spacer()

      if selectedFolder?.uuid == folder?.uuid {
        Image(systemName: "checkmark")
          .foregroundColor(.white)
      }
    }
    .padding(.vertical, 6.0)
  }
}

struct PlaylistMoveFolderView: View {
  @FetchRequest(
    entity: PlaylistFolder.entity(),
    sortDescriptors: [
      NSSortDescriptor(keyPath: \PlaylistFolder.order, ascending: true),
      NSSortDescriptor(keyPath: \PlaylistFolder.dateAdded, ascending: false),
    ],
    predicate: NSPredicate(format: "uuid != %@", PlaylistFolder.savedFolderUUID)
  ) var folders: FetchedResults<PlaylistFolder>

  @FetchRequest(
    entity: PlaylistFolder.entity(),
    sortDescriptors: [],
    predicate: NSPredicate(format: "uuid == %@", PlaylistFolder.savedFolderUUID)
  ) var savedFolder: FetchedResults<PlaylistFolder>

  @State private var selectedFolder = PlaylistManager.shared.currentFolder

  private var isMoveDisabled: Bool {
    return selectedFolder?.uuid == PlaylistManager.shared.currentFolder?.uuid
  }

  private var itemDescription: String {
    if selectedItems.count == 1 {
      return selectedItems[0].name
    }

    let title = selectedItems[0].name
    if selectedItems.count == 2 {
      return String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderMoveItemDescription, title)
    }

    return String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderMoveMultipleItemDescription, title, selectedItems.count - 1)
  }

  var selectedItems: [PlaylistItem]
  var onCancelButtonPressed: (() -> Void)?
  var onDoneButtonPressed: (([PlaylistItem], PlaylistFolder?) -> Void)?

  var body: some View {
    NavigationView {
      List {
        Section(
          header: {
            HStack(spacing: 12.0) {
              if selectedItems.count > 1,
                let firstItem = selectedItems[safe: 0],
                let secondItem = selectedItems[safe: 1] {
                ZStack {
                  PlaylistFolderImage(item: firstItem)
                    .rotationEffect(.degrees(5.0))
                  PlaylistFolderImage(item: secondItem).rotationEffect(.degrees(-5.0))
                }

                Text(itemDescription)
                  .lineLimit(2)
                  .font(.body)
                  .foregroundColor(.white)
                  .textCase(.none)
                  .fixedSize(horizontal: false, vertical: true)
              } else if let item = selectedItems.first {
                PlaylistFolderImage(item: item)
                Text(item.name ?? Strings.PlaylistFolders.playlistFolderMoveItemWithNoNameTitle)
                  .lineLimit(2)
                  .font(.body)
                  .foregroundColor(.white)
                  .textCase(.none)
              }
            }
            .padding(.top)
          }()
        ) {}
        .listRowInsets(.zero)
        .listRowBackground(Color.clear)

        Section(
          header: Text(Strings.PlaylistFolders.playlistFolderMoveFolderCurrentSectionTitle)
            .font(.footnote)
            .textCase(.none)
            .foregroundColor(Color(.secondaryBraveLabel))
            .multilineTextAlignment(.leading)
        ) {

          Button(action: {
            selectedFolder = PlaylistManager.shared.currentFolder
          }) {
            PlaylistFolderView(
              isSourceFolder: true,
              folder: PlaylistManager.shared.currentFolder,
              selectedFolder: $selectedFolder)
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))

        let sectionTitle = selectedItems.count == 1 ? Strings.PlaylistFolders.playlistFolderSelectASingleFolderTitle : String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderSelectAFolderTitle, selectedItems.count)
        Section(
          header: Text(sectionTitle)
            .font(.footnote)
            .textCase(.none)
            .foregroundColor(Color(.secondaryBraveLabel))
            .multilineTextAlignment(.leading)
        ) {

          // Show the "Saved" folder
          if PlaylistManager.shared.currentFolder?.uuid != savedFolder.first?.uuid {
            Button(action: {
              selectedFolder = savedFolder.first
            }) {
              PlaylistFolderView(
                isSourceFolder: false,
                folder: savedFolder.first,
                selectedFolder: $selectedFolder)
            }
          }

          // Show all folders except the current one
          ForEach(
            folders.filter { $0.uuid != PlaylistManager.shared.currentFolder?.uuid }
          ) { folder in
            Button(action: {
              selectedFolder = folder
            }) {
              PlaylistFolderView(
                isSourceFolder: false,
                folder: folder,
                selectedFolder: $selectedFolder)
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .listStyle(.insetGrouped)
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .navigationTitle(Strings.PlaylistFolders.playlistFolderMoveFolderScreenTitle)
      .navigationBarTitleDisplayMode(.inline)
      .navigationViewStyle(.stack)
      .toolbar {
        ToolbarItem(placement: .cancellationAction) {
          Button(Strings.cancelButtonTitle) { onCancelButtonPressed?() }
            .foregroundColor(.white)
        }

        ToolbarItem(placement: .confirmationAction) {
          Button(Strings.done) { onDoneButtonPressed?(selectedItems, selectedFolder) }
            .foregroundColor(isMoveDisabled ? Color(.braveDisabled) : .white)
            .disabled(isMoveDisabled)
        }
      }
    }
    .environment(\.colorScheme, .dark)
  }
}

#if DEBUG
struct PlaylistMoveFolderView_Previews: PreviewProvider {
  static var previews: some View {
    PlaylistMoveFolderView(selectedItems: [])
      .environment(\.managedObjectContext, DataController.swiftUIContext)
  }
}
#endif
