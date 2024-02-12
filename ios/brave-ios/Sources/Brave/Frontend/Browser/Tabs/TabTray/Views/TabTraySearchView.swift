// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

class TabTraySearchBar: UIView {
  let searchBar: UISearchBar

  init(searchBar: UISearchBar) {
    self.searchBar = searchBar
    super.init(frame: .zero)
    addSubview(searchBar)
    translatesAutoresizingMaskIntoConstraints = false
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    
    var adjustedFrame = bounds
    
    // Adjusting search bar bounds here for landscape iPhones, needs padding from top and bottom
    if traitCollection.horizontalSizeClass == .compact && traitCollection.verticalSizeClass == .compact {
      adjustedFrame = CGRect(
        x: adjustedFrame.origin.x,
        y: adjustedFrame.origin.y + 2,
        width: adjustedFrame.size.width,
        height: adjustedFrame.size.height - 4)
    }
    
    searchBar.frame = adjustedFrame
  }

  override var intrinsicContentSize: CGSize {
    return UIView.layoutFittingExpandedSize
  }
}

