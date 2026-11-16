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
  enum Action {
    case ungroup
    case closeAllTabs
  }

  var viewModel: TabGridGroupItemViewModel
  var actionHandler: (Action) -> Void

  var browserColors: BrowserColors {
    viewModel.isPrivateBrowsing ? .privateMode : .standard
  }

  var body: some View {
    VStack(spacing: 0) {
      HStack(spacing: 6) {
        Text(viewModel.title)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .lineLimit(1)
          .accessibilityValue(
            String.localizedStringWithFormat(Strings.TabGrid.tabsCountFormat, viewModel.tabCount)
          )
        Spacer()
        optionsMenu
      }
      .padding(8)
      .font(.caption2.weight(.semibold))
      thumbnailContainer
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
    .dynamicTypeSize(.xSmall..<DynamicTypeSize.accessibility1)
  }

  private var optionsMenu: some View {
    Menu {
      Button {
        actionHandler(.ungroup)
      } label: {
        Text(Strings.TabGrid.ungroupMenuTitle)
      }
      Button(role: .destructive) {
        actionHandler(.closeAllTabs)
      } label: {
        Label(Strings.TabGrid.closeGroupTabsMenuTitle, braveSystemImage: "leo.close")
      }
    } label: {
      Label(Strings.TabGrid.groupOptionsAccessibilityLabel, braveSystemImage: "leo.close")
        .labelStyle(.iconOnly)
        .imageScale(.large)
        .foregroundStyle(Color(braveSystemName: .iconDefault))
    }
  }

  private var thumbnailContainer: some View {
    VStack(spacing: 4) {
      HStack(spacing: 4) {
        thumbnail(at: 0)
        thumbnail(at: 1)
      }
      HStack(spacing: 4) {
        thumbnail(at: 2)
        thumbnail(at: 3)
      }
    }
    .padding(4)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(Color(braveSystemName: .containerDisabled))
    .clipShape(.rect(cornerRadius: 12, style: .continuous))
  }

  @ViewBuilder
  private func thumbnail(at index: Int) -> some View {
    if index < viewModel.tabCount {
      Color(braveSystemName: .containerBackground)
        .overlay(alignment: .top) {
          if let screenshot = viewModel.thumbnails[safe: index] ?? nil {
            Image(uiImage: screenshot)
              .resizable()
              .aspectRatio(contentMode: .fill)
          }
        }
        .clipShape(.rect(cornerRadius: 6, style: .continuous))
    } else {
      Color.clear
    }
  }
}

class TabGridGroupCell: UICollectionViewCell {
  let model: TabGridGroupItemViewModel = .init()
  var actionHandler: ((TabGridGroupItemView.Action) -> Void)?
  var isPrivateBrowsing: Bool = false {
    didSet {
      model.isPrivateBrowsing = isPrivateBrowsing
    }
  }

  override init(frame: CGRect) {
    super.init(frame: .zero)

    contentConfiguration = UIHostingConfiguration {
      TabGridGroupItemView(
        viewModel: model,
        actionHandler: { [unowned self] in
          actionHandler?($0)
        }
      )
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
