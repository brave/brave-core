/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

/// The base of all the sections in the settings view
class SettingsSectionView: UIView {
  
  /// Add subviews to this view if they need to be clipped
  /// correctly based on the rounded corners
  lazy var clippedContentView = UIView().then {
    $0.clipsToBounds = true
    $0.layer.cornerRadius = self.layer.cornerRadius
    $0.preservesSuperviewLayoutMargins = true
    self.addSubview($0)
    $0.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = .white
    
    layer.cornerRadius = 8.0
    layer.shadowColor = UIColor.black.cgColor
    layer.shadowOpacity = 0.1
    layer.shadowRadius = 1.5
    layer.shadowOffset = CGSize(width: 0, height: 1)
    
    layoutMargins = SettingsUX.layoutMargins
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
