// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Strings
import SwiftUI

struct QuickSearchView: View {
  var isAiChatAvailable: Bool
  var items: [OpenSearchEngine]
  var onAiChat: () -> Void
  var onSelection: (OpenSearchEngine) -> Void
  var onOpenMore: () -> Void

  @Environment(\.horizontalSizeClass) private var horizontalSizeClass

  /// Each button pads its icon by 8 for a 44pt hit area, which already yields a 16pt visual gap on
  /// iPhone. iPad has the room to spread them out further.
  private var engineSpacing: CGFloat {
    horizontalSizeClass == .regular ? 16 : 0
  }

  var body: some View {
    HStack(spacing: 0) {
      if isAiChatAvailable {
        Button {
          onAiChat()
        } label: {
          Label(
            String(
              format: Strings.searchEngineFormatText,
              Strings.quickSearchAskLeoAccessibility
            ),
            braveSystemImage: "leo.product.brave-leo"
          )
          .labelStyle(.iconOnly)
          .font(.system(size: 20))
          .foregroundStyle(.white)
          .frame(width: 28.0, height: 28.0)
          .background(LinearGradient(braveSystemName: .iconsActive), in: .rect(cornerRadius: 8))
          .padding(8)
          .contentShape(.rect)
        }
        .padding(.trailing, 8)
      }
      ScrollView(.horizontal, showsIndicators: false) {
        HStack(spacing: engineSpacing) {
          ForEach(items, id: \.shortName) { item in
            Button {
              onSelection(item)
            } label: {
              Image(uiImage: item.image)
                .resizable()
                .scaledToFit()
                .frame(width: 28.0, height: 28.0)
                .clipShape(.rect(cornerRadius: 8))
                .padding(8)
                .contentShape(.rect)
            }
            .accessibilityLabel(
              String(format: Strings.searchEngineFormatText, item.shortName)
            )
          }
        }
      }
      .mask {
        HStack(spacing: 0) {
          Rectangle()
          LinearGradient(
            stops: [
              .init(color: .black, location: 0),
              .init(color: .black, location: 0.2),
              .init(color: .clear, location: 1),
            ],
            startPoint: .leading,
            endPoint: .trailing
          )
          .frame(width: 48)
        }
      }
      Spacer()
      Button {
        onOpenMore()
      } label: {
        Label(Strings.quickSearchEditAccessibility, braveSystemImage: "leo.more.horizontal")
          .labelStyle(.iconOnly)
          .font(.title3)
          .foregroundStyle(Color(braveSystemName: .iconDefault))
          .padding(12)
          .contentShape(.rect)
      }
      .padding(.horizontal, 4)
    }
    .padding(.horizontal, 8)
  }
}
