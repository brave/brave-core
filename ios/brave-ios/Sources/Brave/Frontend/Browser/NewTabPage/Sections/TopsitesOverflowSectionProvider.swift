// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import CoreData
import Data
import Foundation
import Preferences
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

class TopsitesOverflowSectionProvider: NSObject, NTPObservableSectionProvider {
  let action: () -> Void
  var sectionDidChange: (() -> Void)?

  private typealias FavoritesOverflowCell = NewTabCenteredCollectionViewCell<
    FavoritesOverflowButton
  >

  private var frc: NSFetchedResultsController<Favorite>
  private let mostVisitedSites: MostVisitedSites?
  private var mostVisitedObservation: MostVisitedSitesScopedObservation?
  private var mostVisitedTiles: [NTPTile] = []
  private let isPrivateBrowsing: Bool

  var numberOfTiles: Int {
    switch Preferences.NewTabPage.topsitesMode.value {
    case TopsitesMode.none:
      return 0
    case .favourite:
      return frc.fetchedObjects?.count ?? 0
    case .mostVisited:
      return isPrivateBrowsing ? 0 : mostVisitedTiles.count
    }
  }

  init(
    action: @escaping () -> Void,
    mostVisitedSites: MostVisitedSites?,
    isPrivateBrowsing: Bool
  ) {
    self.action = action
    self.mostVisitedSites = mostVisitedSites
    self.isPrivateBrowsing = isPrivateBrowsing
    frc = Favorite.frc()
    frc.fetchRequest.fetchLimit = 20
    super.init()
    try? frc.performFetch()
    frc.delegate = self
    Preferences.NewTabPage.topsitesMode.observe(from: self)
    updateMostVisitedObservation()
  }

  deinit {
    mostVisitedObservation?.invalidate()
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
      numberOfTiles
      > TopsitesSectionProvider.numberOfItems(in: collectionView, availableWidth: width)
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
      maxWidth: TopsitesSectionProvider.maxWidth,
      minimumInset: 16
    )
    return UIEdgeInsets(top: 0, left: insets.left, bottom: 0, right: insets.right)
  }

  private func updateMostVisitedObservation() {
    if Preferences.NewTabPage.topsitesMode.value == .mostVisited, !isPrivateBrowsing {
      guard mostVisitedObservation == nil else { return }
      mostVisitedObservation = mostVisitedSites?.addMostVisitedURLsObserver(self, maxNumSites: 20)
      mostVisitedSites?.enableTopSitesOnlyTileTypes()
    } else {
      mostVisitedObservation?.invalidate()
      mostVisitedObservation = nil
      mostVisitedTiles = []
    }
  }
}

extension TopsitesOverflowSectionProvider: NSFetchedResultsControllerDelegate {
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    try? frc.performFetch()
    DispatchQueue.main.async {
      self.sectionDidChange?()
    }
  }
}

extension TopsitesOverflowSectionProvider: MostVisitedSitesObserver {
  func mostVisitedSitesDidUpdateTiles(_ tiles: [NTPTile]) {
    mostVisitedTiles = tiles
    sectionDidChange?()
  }

  func mostVisitedSitesDidUpdateFavicon(for url: URL?) {
    // no-op. only the number of tiles matter in this provider
  }
}

extension TopsitesOverflowSectionProvider: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    guard key == Preferences.NewTabPage.topsitesMode.key else { return }
    updateMostVisitedObservation()
  }
}
