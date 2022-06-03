// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

public protocol CollectionViewReusable {}

extension CollectionViewReusable {
  fileprivate static var identifier: String {
    return String(describing: type(of: self))
  }
}

extension UICollectionView {
  /// Register a UICollectionViewCell subclass as a dequeable cell
  public func register<T: UICollectionViewCell & CollectionViewReusable>(_ cellClass: T.Type) {
    register(cellClass, forCellWithReuseIdentifier: cellClass.identifier)
  }
  // swiftlint:disable force_cast
  public func dequeueReusableCell<T: UICollectionViewCell & CollectionViewReusable>(for indexPath: IndexPath) -> T {
    return dequeueReusableCell(withReuseIdentifier: T.identifier, for: indexPath) as! T
  }
  // swiftlint:enable force_cast
}
