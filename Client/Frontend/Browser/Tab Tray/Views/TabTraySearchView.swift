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

  override func sizeThatFits(_ size: CGSize) -> CGSize {
    // This is done to adjust the frame of a UISearchBar inside of UISearchController
    // Adjusting the bar frame directly doesnt work so had to create a custom view with UISearchBar
    .init(width: size.width + 16, height: size.height)
  }
}

