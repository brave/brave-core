// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

extension TabTrayController {

  class TabTrayContainerView: UIView {
    private struct UX {
      static let regularCellHeight = 192.0
      static let largeCellHeight = 256.0
      static let itemInset = 6.0
      static let sectionInset = 4.0
      static let buttonEdgeInset = 10.0
    }

    private func generateLayout(
      numberOfColumns: Int,
      cellHeight: CGFloat = UX.regularCellHeight
    ) -> UICollectionViewLayout {
      let itemSize = NSCollectionLayoutSize(
        widthDimension: .fractionalWidth(1.0),
        heightDimension: .fractionalHeight(1.0))
      let item = NSCollectionLayoutItem(layoutSize: itemSize)
      item.contentInsets = .init(top: UX.itemInset, leading: UX.itemInset, bottom: UX.itemInset, trailing: UX.itemInset)

      let groupSize = NSCollectionLayoutSize(
        widthDimension: .fractionalWidth(1.0),
        heightDimension: .absolute(cellHeight))
      let group = NSCollectionLayoutGroup.horizontal(
        layoutSize: groupSize,
        subitem: item,
        count: numberOfColumns)

      let section = NSCollectionLayoutSection(group: group)
      section.contentInsets = .init(top: UX.sectionInset, leading: UX.sectionInset, bottom: UX.sectionInset, trailing: UX.sectionInset)
      let layout = UICollectionViewCompositionalLayout(section: section)
      return layout
    }

    lazy var collectionView = UICollectionView(frame: .zero, collectionViewLayout: generateLayout(numberOfColumns: numberOfColumns)).then {
      $0.setContentHuggingPriority(.defaultLow, for: .vertical)
      $0.backgroundColor = .clear
      $0.register(TabCell.self, forCellWithReuseIdentifier: TabCell.identifier)
    }

    private(set) lazy var privateModeInfo = TabTrayPrivateModeInfoView().then {
      $0.learnMoreButton.addTarget(
        self, action: #selector(privateModeLearnMoreAction),
        for: .touchUpInside)
      $0.setContentHuggingPriority(.defaultLow, for: .vertical)
      $0.isHidden = true
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      accessibilityLabel = Strings.tabTrayAccessibilityLabel

      let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = 0
        $0.addStackViewItems(
          .view(privateModeInfo),
          .view(collectionView))
        $0.isAccessibilityElement = false
      }

      addSubview(stackView)
      stackView.snp.makeConstraints {
        $0.top.left.right.equalToSuperview()
        $0.bottom.equalTo(safeAreaLayoutGuide.snp.bottom)
      }

      privateModeInfo.isHidden = true
    }

    var numberOfColumns: Int {
      let compactWidthRegularHeight = traitCollection.horizontalSizeClass == .compact && traitCollection.verticalSizeClass == .regular
      return compactWidthRegularHeight ? 2 : 3
    }

    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }

    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
      super.traitCollectionDidChange(previousTraitCollection)

      if previousTraitCollection != traitCollection {
        let regularWidthHeight = traitCollection.horizontalSizeClass == .regular && traitCollection.verticalSizeClass == .regular

        let cellHeight = regularWidthHeight ? UX.largeCellHeight : UX.regularCellHeight

        let layout = generateLayout(numberOfColumns: numberOfColumns, cellHeight: cellHeight)

        // FIXME: with `animated: true` it glitches out.
        collectionView.setCollectionViewLayout(layout, animated: false)
      }
    }

    @objc func privateModeLearnMoreAction() {
      privateModeInfo.detailsLabel.alpha = 0.0
      UIView.animate(
        withDuration: 0.15,
        animations: {
          self.privateModeInfo.learnMoreButton.isHidden = true
          self.privateModeInfo.detailsLabel.isHidden = false
        },
        completion: { _ in
          UIView.animate(withDuration: 0.15) {
            self.privateModeInfo.detailsLabel.alpha = 1.0
          }
        })
      // Needs to be done in a separate call so the stack view's height is updated properly based on the hidden
      // status of learnMoreButton/detailsLabel
      UIView.animate(withDuration: 0.2) {
        self.privateModeInfo.updateContentInset()
      }
    }

    func showPrivateModeInfo() {
      // Private mode always uses dark style.
      // Setting it explicitly here makes a smoother transition.
      overrideUserInterfaceStyle = .dark
      privateModeInfo.isHidden = false
      collectionView.isHidden = true
    }

    func hidePrivateModeInfo() {
      overrideUserInterfaceStyle = .unspecified
      privateModeInfo.isHidden = true
      collectionView.isHidden = false
    }

    var isPrivateModeInfoShowing: Bool {
      !privateModeInfo.isHidden
    }
  }
}
