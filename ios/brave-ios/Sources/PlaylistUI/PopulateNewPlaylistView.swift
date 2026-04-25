// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import Data
import DesignSystem
import Foundation
import Playlist
import Strings
import SwiftUI

/// A list of videos that the user can select to populate a newly created playlist
struct PopulateNewPlaylistView: View {
  var destinationFolder: PlaylistFolder.ID

  @Environment(\.dismiss) private var dismiss

  @FetchRequest(
    sortDescriptors: [
      .init(keyPath: \PlaylistItem.order, ascending: true),
      .init(keyPath: \PlaylistItem.dateAdded, ascending: false),
    ],
    predicate: .init(format: "playlistFolder.uuid == %@", PlaylistFolder.savedFolderUUID)
  )
  private var items: FetchedResults<PlaylistItem>

  @State private var selectedItems: Set<PlaylistItem.ID> = []

  var body: some View {
    ScrollView(.vertical) {
      VStack {
        VStack(alignment: .leading) {
          Text(
            LocalizedStringKey(
              String.localizedStringWithFormat(
                Strings.Playlist.fillNewPlaylistTitle,
                Strings.Playlist.defaultPlaylistTitle
              )
            )
          )
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          Text(Strings.Playlist.fillNewPlaylistSubtitle)
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textTertiary))
        }
        .padding(.vertical)
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
        LazyVGrid(
          columns: [.init(), .init(), .init()],
          spacing: 8
        ) {
          ForEach(items) { item in
            ItemToggle(
              item: item,
              isSelected: Binding(
                get: { selectedItems.contains(item.id) },
                set: { isOn in
                  if isOn {
                    selectedItems.insert(item.id)
                  } else {
                    selectedItems.remove(item.id)
                  }
                }
              )
            )
          }
        }
      }
      .frame(maxWidth: .infinity)
      .padding()
    }
    .background(Color(braveSystemName: .containerBackground))
    .safeAreaInset(edge: .bottom, spacing: 0) {
      Button {
        if !selectedItems.isEmpty {
          PlaylistItem.moveItems(items: Array(selectedItems), to: destinationFolder)
        }
        dismiss()
      } label: {
        Text(Strings.Playlist.saveButtonTitle)
          .font(.subheadline.weight(.semibold))
          .frame(maxWidth: .infinity)
          .padding(.vertical, 12)
          .padding(.horizontal)
          .background(Color(braveSystemName: .buttonBackground))
          .containerShape(.rect(cornerRadius: 10, style: .continuous))
      }
      .padding()
      .tint(Color(braveSystemName: .schemesOnPrimary))
      .background(
        LinearGradient(
          stops: [
            .init(color: Color(braveSystemName: .containerBackground).opacity(0.0), location: 0),
            .init(color: Color(braveSystemName: .containerBackground), location: 0.15),
            .init(color: Color(braveSystemName: .containerBackground), location: 1),
          ],
          startPoint: .top,
          endPoint: .bottom
        )
      )
    }
  }
}

private struct ItemToggle: View {
  var item: PlaylistItem
  @Binding var isSelected: Bool

  var body: some View {
    Button {
      isSelected.toggle()
    } label: {
      VStack(spacing: 4) {
        Color.clear
          .aspectRatio(1.333, contentMode: .fit)
          .overlay {
            if let assetURL = URL(string: item.mediaSrc), let pageURL = URL(string: item.pageSrc) {
              MediaThumbnail(assetURL: assetURL, pageURL: pageURL)
            }
          }
          .clipShape(ContainerRelativeShape())
        Text(item.name)
          .font(.caption2.weight(.medium))
          .lineLimit(1)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .padding(4)
      }
      .padding(4)
      .background(Color(braveSystemName: .containerHighlight))
      .contentShape(.rect)
      .overlay {
        if isSelected {
          ContainerRelativeShape()
            .strokeBorder(Color(braveSystemName: .primitivePrimary40), lineWidth: 2)
        }
      }
      .containerShape(.rect(cornerRadius: 8, style: .continuous))
    }
    .buttonStyle(.spring)
    .backgroundStyle(.clear)
    .accessibilityRepresentation {
      Toggle(isOn: $isSelected) {
        Text(item.name)
      }
    }
  }
}
