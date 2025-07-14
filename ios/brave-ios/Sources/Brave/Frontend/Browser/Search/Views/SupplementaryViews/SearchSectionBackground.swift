// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

class SearchSectionBackgroundLayoutAttribute: UICollectionViewLayoutAttributes {
  var backgroundColour = UIColor(braveSystemName: .materialThin)
  var groupBackgroundColour = UIColor(braveSystemName: .containerBackground)

  override func copy(with zone: NSZone? = nil) -> Any {
    let copy = super.copy(with: zone) as! SearchSectionBackgroundLayoutAttribute
    copy.backgroundColour = self.backgroundColour
    copy.groupBackgroundColour = self.groupBackgroundColour
    return copy
  }

  override func isEqual(_ object: Any?) -> Bool {
    guard let other = object as? SearchSectionBackgroundLayoutAttribute else { return false }
    return super.isEqual(object)
      && backgroundColour == other.backgroundColour
      && groupBackgroundColour == other.groupBackgroundColour
  }
}

class SearchSectionBackgroundView: UICollectionReusableView {

  let sectionGroupBackground = UIView()

  override init(frame: CGRect) {
    super.init(frame: frame)

    self.do {
      $0.backgroundColor = UIColor(braveSystemName: .materialThin)
      $0.layer.cornerRadius = 16
    }

    addSubview(sectionGroupBackground)
    sectionGroupBackground.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(4)
    }

    sectionGroupBackground.do {
      $0.backgroundColor = UIColor(braveSystemName: .containerBackground)
      $0.layer.cornerRadius = 12
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func apply(_ layoutAttributes: UICollectionViewLayoutAttributes) {
    super.apply(layoutAttributes)

    guard let customAttrs = layoutAttributes as? SearchSectionBackgroundLayoutAttribute else {
      return
    }
    self.backgroundColor = customAttrs.backgroundColour
    self.sectionGroupBackground.backgroundColor = customAttrs.groupBackgroundColour
  }
}
