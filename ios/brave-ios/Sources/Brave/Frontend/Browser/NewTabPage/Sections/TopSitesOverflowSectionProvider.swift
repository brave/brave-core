// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Observation
import Shared
import UIKit

class FavoritesOverflowButton: SpringButton {
  private let backgroundView: UIVisualEffectView = {
    let view = UIVisualEffectView()
    view.clipsToBounds = true
    view.isUserInteractionEnabled = false
    if #available(iOS 26.0, *) {
      view.effect = UIGlassEffect(style: .regular)
    } else {
      view.effect = UIBlurEffect(style: .systemThinMaterial)
    }
    view.overrideUserInterfaceStyle = .dark
    return view
  }()

  override init(frame: CGRect) {
    super.init(frame: frame)

    let label = UILabel().then {
      $0.text = Strings.NTP.showMoreFavorites
      $0.textColor = .white
      $0.font = UIFont.systemFont(ofSize: 12.0, weight: .medium)
    }

    backgroundView.layer.cornerCurve = .continuous

    addSubview(backgroundView)
    backgroundView.contentView.addSubview(label)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    label.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(UIEdgeInsets(top: 5, left: 10, bottom: 5, right: 10))
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    backgroundView.layer.cornerRadius = bounds.height / 2.0  // Pill shape
  }
}

class TopSitesOverflowSectionProvider: NSObject, NTPObservableSectionProvider {
  let action: () -> Void
  var sectionDidChange: (() -> Void)?

  private typealias FavoritesOverflowCell = NewTabCenteredCollectionViewCell<
    FavoritesOverflowButton
  >

  private let tileSource: TopSitesTileSource
  /// The tiles the top sites section is currently showing, whose count decides whether there is
  /// any overflow to reveal. Kept separate from the source for the same reason as the top sites
  /// section: so that it can't change underneath the collection view while a reload is pending.
  private var tiles: [TopSiteTile] = []

  init(
    action: @escaping () -> Void,
    tileSource: TopSitesTileSource
  ) {
    self.action = action
    self.tileSource = tileSource

    super.init()

    updateTiles()
  }

  /// Snapshots the source's tiles and re-arms tracking for the next change.
  private func updateTiles() {
    tiles = withObservationTracking {
      tileSource.tiles
    } onChange: { [weak self] in
      DispatchQueue.main.async {
        self?.updateTiles()
        self?.sectionDidChange?()
      }
    }
  }

  @objc private func tappedButton() {
    action()
  }

  func collectionView(
    _ collectionView: UICollectionView,
    numberOfItemsInSection section: Int
  ) -> Int {
    let width = fittingSizeForCollectionView(collectionView, section: section).width

    let isShowShowMoreButtonVisible =
      tiles.count
      > TopSitesSectionProvider.numberOfItems(in: collectionView, availableWidth: width)
    return isShowShowMoreButtonVisible ? 1 : 0
  }

  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(FavoritesOverflowCell.self)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(for: indexPath) as FavoritesOverflowCell
    cell.view.addTarget(self, action: #selector(tappedButton), for: .touchUpInside)
    return cell
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
    size.height = 24
    return size
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    insetForSectionAt section: Int
  ) -> UIEdgeInsets {
    let insets = horizontalInsets(
      for: collectionView,
      maxWidth: TopSitesSectionProvider.maxWidth,
      minimumInset: 16
    )
    return UIEdgeInsets(top: 0, left: insets.left, bottom: 0, right: insets.right)
  }
}
