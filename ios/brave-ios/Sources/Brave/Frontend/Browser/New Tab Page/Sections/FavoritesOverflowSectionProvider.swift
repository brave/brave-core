// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Preferences
import Data
import CoreData
import Shared
import UIKit

class FavoritesOverflowButton: SpringButton {
  private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .light)).then {
    $0.clipsToBounds = true
    $0.isUserInteractionEnabled = false
  }

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

class FavoritesOverflowSectionProvider: NSObject, NTPObservableSectionProvider {
  let action: () -> Void
  var sectionDidChange: (() -> Void)?

  private typealias FavoritesOverflowCell = NewTabCenteredCollectionViewCell<FavoritesOverflowButton>

  private var frc: NSFetchedResultsController<Favorite>

  init(action: @escaping () -> Void) {
    self.action = action
    frc = Favorite.frc()
    frc.fetchRequest.fetchLimit = 10
    super.init()
    try? frc.performFetch()
    frc.delegate = self
  }

  @objc private func tappedButton() {
    action()
  }

  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    let width = fittingSizeForCollectionView(collectionView, section: section).width
    let count = frc.fetchedObjects?.count ?? 0
    
    let isShowShowMoreButtonVisible = count > FavoritesSectionProvider.numberOfItems(in: collectionView, availableWidth: width) &&
      Preferences.NewTabPage.showNewTabFavourites.value
    return isShowShowMoreButtonVisible ? 1 : 0
  }

  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(FavoritesOverflowCell.self)
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(for: indexPath) as FavoritesOverflowCell
    cell.view.addTarget(self, action: #selector(tappedButton), for: .touchUpInside)
    return cell
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
    size.height = 24
    return size
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    insetForSectionAt section: Int
  ) -> UIEdgeInsets { .zero }
}

extension FavoritesOverflowSectionProvider: NSFetchedResultsControllerDelegate {
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    try? frc.performFetch()
    DispatchQueue.main.async {
      self.sectionDidChange?()
    }
  }
}
