// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import DesignSystem
import Favicon
import SwiftUI

struct BookmarkItemView: View {
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
