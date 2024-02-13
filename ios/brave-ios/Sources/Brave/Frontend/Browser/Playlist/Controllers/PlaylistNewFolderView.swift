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
import Favicon

class PlaylistFolderImageLoader: ObservableObject {
  @Published var image: UIImage?

  private let renderer = PlaylistThumbnailRenderer()
  private var faviconTask: Task<Void, Error>?

  func load(thumbnail: PlaylistItem) {
    guard let assetUrl = URL(string: thumbnail.mediaSrc)
    else {
      image = nil
      return
    }

    loadImage(url: assetUrl, isFavIcon: false)
  }

  func load(favIcon: PlaylistItem) {
    guard let favIconUrl = URL(string: favIcon.pageSrc)
    else {
      image = nil
      return
    }

    loadImage(url: favIconUrl, isFavIcon: true)
  }

  func load(domainUrl: URL) {
    faviconTask?.cancel()
    faviconTask = Task { @MainActor in
      let favicon = try await FaviconFetcher.loadIcon(url: domainUrl, persistent: true)
      self.image = favicon.image
    }
  }

  private func loadImage(url: URL, isFavIcon: Bool) {
    renderer.cancel()
    renderer.loadThumbnail(
      assetUrl: isFavIcon ? nil : url,
      favIconUrl: isFavIcon ? url : nil,
      completion: { [weak self] image in
        self?.image = image
      })
  }
}

private struct PlaylistFolderImage: View {
  let item: PlaylistItem

  static let cornerRadius = 10.0
  private static let favIconSize = 16.0
  private var title: String?

  @StateObject private var thumbnailLoader = PlaylistFolderImageLoader()
  @StateObject private var favIconLoader = PlaylistFolderImageLoader()

  init(item: PlaylistItem) {
    self.item = item
  }

  var body: some View {
    Image(uiImage: thumbnailLoader.image ?? .init())
      .resizable()
      .aspectRatio(contentMode: .fit)
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background(Color.black)
      .overlay(
        LinearGradient(
          colors: [.clear, .black],
          startPoint: .top,
          endPoint: .bottom)
      )
      .overlay(
        VStack(alignment: .leading) {
          Image(uiImage: favIconLoader.image ?? .init())
            .resizable()
            .aspectRatio(1.0, contentMode: .fit)
            .frame(
              width: PlaylistFolderImage.favIconSize,
              height: PlaylistFolderImage.favIconSize
            )
            .clipShape(RoundedRectangle(cornerRadius: 3.0, style: .continuous))

          Spacer()

          Text(item.name)
            .font(.footnote.weight(.semibold))
            .lineLimit(2)
            .foregroundColor(.white)
        }.padding(8.0), alignment: .topLeading
      )
      .clipShape(RoundedRectangle(cornerRadius: PlaylistFolderImage.cornerRadius, style: .continuous))
      .onAppear {
        thumbnailLoader.load(thumbnail: item)
        favIconLoader.load(favIcon: item)
      }
  }
}

struct PlaylistNewFolderView: View {
  @FetchRequest(
    entity: PlaylistItem.entity(),
    sortDescriptors: [
      NSSortDescriptor(keyPath: \PlaylistItem.order, ascending: true),
      NSSortDescriptor(keyPath: \PlaylistItem.dateAdded, ascending: false),
    ],
    predicate: NSPredicate(format: "playlistFolder.uuid == %@", PlaylistFolder.savedFolderUUID)
  ) var items: FetchedResults<PlaylistItem>

  private static let designGridSpacing = 12.0
  private static let designGridItemWidth = 150.0
  private static let designGridItemHeight = 100.0
  private let gridItems = [
    GridItem(
      .adaptive(
        minimum: PlaylistNewFolderView.designGridItemWidth,
        maximum: .infinity),
      spacing: PlaylistNewFolderView.designGridSpacing)
  ]

  @State private var folderName: String = ""
  @State private var selected: [NSManagedObjectID] = []

  var onCancelButtonPressed: (() -> Void)?
  var onCreateFolder: ((_ folderTitle: String, _ selectedItems: [NSManagedObjectID]) -> Void)?

  var body: some View {
    NavigationView {
      List {
        Section {
          TextField(Strings.PlaylistFolders.playlistUntitledFolderTitle, text: $folderName)
            .disableAutocorrection(true)
            .padding()
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        .listRowInsets(.zero)

        if !items.isEmpty {
          Section {
            VStack(alignment: .leading) {
              HStack {
                VStack(alignment: .leading) {
                  Text(Strings.PlaylistFolders.playlistFolderNewFolderSectionTitle)
                    .font(.headline)
                    .foregroundColor(.white)
                    .multilineTextAlignment(.leading)
                  Text(Strings.PlaylistFolders.playlistFolderNewFolderSectionSubtitle)
                    .font(.footnote)
                    .foregroundColor(Color(.secondaryBraveLabel))
                    .multilineTextAlignment(.leading)
                }
              }

              LazyVGrid(
                columns: gridItems,
                alignment: .leading,
                spacing: PlaylistNewFolderView.designGridSpacing
              ) {
                ForEach(items) { item in
                  Button(
                    action: {
                      withAnimation(.linear(duration: 0.1)) {
                        if let index = selected.firstIndex(of: item.objectID) {
                          selected.remove(at: index)
                        } else {
                          selected.append(item.objectID)
                        }
                      }
                    },
                    label: {
                      PlaylistFolderImage(item: item)
                    }
                  )
                  .frame(height: PlaylistNewFolderView.designGridItemHeight)
                  .buttonStyle(.plain)
                  .overlay(
                    Group {
                      if selected.contains(item.objectID) {
                        RoundedRectangle(
                          cornerRadius: PlaylistFolderImage.cornerRadius,
                          style: .continuous
                        )
                        .strokeBorder(Color.blue, lineWidth: 2)
                      }
                    }
                  )
                }
              }
            }
            .listRowBackground(Color(.braveGroupedBackground))
            .listRowInsets(.zero)
          }
        }
      }
      .listStyle(.insetGrouped)
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .navigationTitle(Strings.PlaylistFolders.playlistNewFolderScreenTitle)
      .navigationBarTitleDisplayMode(.inline)
      .navigationViewStyle(.stack)
      .toolbar {
        ToolbarItem(placement: .cancellationAction) {
          Button(Strings.cancelButtonTitle) { onCancelButtonPressed?() }
            .foregroundColor(.white)
        }

        ToolbarItem(placement: .confirmationAction) {
          Button(Strings.PlaylistFolders.playlistCreateNewFolderButtonTitle) { onCreateFolder?(folderName, selected) }
            .foregroundColor(.white)
        }
      }
    }
    .environment(\.colorScheme, .dark)
  }
}

#if DEBUG
struct PlaylistNewFolderView_Previews: PreviewProvider {
  static var previews: some View {
    PlaylistNewFolderView()
      .environment(\.managedObjectContext, DataController.swiftUIContext)
  }
}
#endif
