// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import DesignSystem
import Shared
import SwiftUI
import UIKit
import Web

@Observable
class TabGridGroupItemViewModel {
  /// How many tab previews a group tile shows
  static let thumbnailCount = 4

  private(set) var title: String = ""
  private(set) var tabCount: Int = 0
  private(set) var thumbnails: [UIImage?] = []
  private(set) var isVisible: Bool = false
  var isPrivateBrowsing: Bool = false

  func configure(with group: TabGroup, tabs: [any TabState]) {
    title = group.title
    tabCount = tabs.count
    thumbnails = tabs.prefix(Self.thumbnailCount).map(\.screenshot)
    isVisible = tabs.contains(where: \.isVisible)
  }

  func reset() {
    title = ""
    tabCount = 0
    thumbnails = []
    isVisible = false
  }
}

struct TabGridGroupItemView: View {
  var viewModel: TabGridGroupItemViewModel

  var browserColors: BrowserColors {
    viewModel.isPrivateBrowsing ? .privateMode : .standard
  }

  var body: some View {
    VStack(spacing: 0) {
      HStack(spacing: 6) {
        Image(braveSystemName: "leo.window.tabs")
          .foregroundStyle(Color(braveSystemName: .iconDefault))
        Text(viewModel.title)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .lineLimit(1)
        Spacer()
        Text(viewModel.tabCount, format: .number)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
          .accessibilityLabel(
            String.localizedStringWithFormat(Strings.TabGrid.tabsCountFormat, viewModel.tabCount)
          )
      }
      .padding(8)
      .font(.caption2.weight(.semibold))
      thumbnailGrid
    }
    .padding(4)
    .overlay {
      if viewModel.isVisible {
        ContainerRelativeShape().strokeBorder(
          Color(uiColor: browserColors.tabSwitcherSelectedCellBorder),
          lineWidth: 1.5
        )
      }
    }
    .background(
      Color(uiColor: browserColors.tabSwitcherCellBackground)
        .shadow(.drop(color: Color(braveSystemName: .elevationSecondary), radius: 4, x: 0, y: 1))
        .shadow(.drop(color: Color(braveSystemName: .elevationPrimary), radius: 0, x: 0, y: 1)),
      in: .containerRelative
    )
    .containerShape(.rect(cornerRadius: 16, style: .continuous))
    .accessibilityElement(children: .combine)
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility1)
  }

  private var thumbnailGrid: some View {
    let columns = Array(repeating: GridItem(.flexible(), spacing: 4), count: 2)
    return LazyVGrid(columns: columns, spacing: 4) {
      ForEach(0..<TabGridGroupItemViewModel.thumbnailCount, id: \.self) { index in
        Color(braveSystemName: .containerDisabled)
          .aspectRatio(1, contentMode: .fit)
          .overlay(alignment: .top) {
            if let screenshot = viewModel.thumbnails[safe: index] ?? nil {
              Image(uiImage: screenshot)
                .resizable()
                .aspectRatio(contentMode: .fill)
            }
          }
          .clipShape(.rect(cornerRadius: 6, style: .continuous))
      }
    }
    .frame(maxHeight: .infinity, alignment: .top)
    .clipShape(.rect(cornerRadius: 12, style: .continuous))
  }
}

class TabGridGroupCell: UICollectionViewCell {
  let model: TabGridGroupItemViewModel = .init()
  var isPrivateBrowsing: Bool = false {
    didSet {
      model.isPrivateBrowsing = isPrivateBrowsing
    }
  }

  override init(frame: CGRect) {
    super.init(frame: .zero)

    contentConfiguration = UIHostingConfiguration {
      TabGridGroupItemView(viewModel: model)
    }
    .margins(.all, 0)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    model.reset()
  }

  func configure(with group: TabGroup, tabs: [any TabState]) {
    model.configure(with: group, tabs: tabs)
  }
}
