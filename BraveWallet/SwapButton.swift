/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import BraveUI

class SwapButton: SpringButton {
  private let gradientView = BraveGradientView.alternateGradient02.then {
    $0.isUserInteractionEnabled = false
    $0.clipsToBounds = true
  }
  private let imageView = UIImageView(image: UIImage(imageLiteralResourceName: "swap")).then {
    $0.isUserInteractionEnabled = false
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    imageView.contentMode = .scaleAspectFit
    
    addSubview(gradientView)
    addSubview(imageView)
    
    gradientView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    imageView.snp.makeConstraints {
      $0.center.equalToSuperview()
    }
    snp.makeConstraints {
      $0.width.equalTo(snp.height)
    }
    
    layer.shadowColor = UIColor.black.cgColor
    layer.shadowOffset = .init(width: 0, height: 1)
    layer.shadowRadius = 1
    layer.shadowOpacity = 0.3
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    gradientView.layer.cornerRadius = bounds.height / 2.0
    layer.shadowPath = UIBezierPath(ovalIn: bounds).cgPath
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override var intrinsicContentSize: CGSize {
    .init(width: 44, height: 44)
  }
}
