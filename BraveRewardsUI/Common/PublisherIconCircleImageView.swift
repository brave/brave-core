// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

class PublisherIconCircleImageView: UIView {
  
  private var imageView = UIImageView()
  
  init(size: CGFloat, inset: CGFloat = 7.0) {
    super.init(frame: .zero)
    
    addSubview(imageView)
    
    snp.makeConstraints {
      $0.size.equalTo(size)
    }
    
    imageView.snp.makeConstraints {
      $0.edges.equalTo(self).inset(inset)
    }
    
    setContentHuggingPriority(.required, for: .horizontal)
    
    contentMode = .scaleAspectFit
    clipsToBounds = true
    image = UIImage(frameworkResourceNamed: "defaultFavicon")
    
    layer.do {
      $0.cornerRadius = size / 2.0
      $0.borderColor = Colors.neutral800.cgColor
      $0.borderWidth = 1.0 / UIScreen.main.scale
    }
  }
  
  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) { fatalError() }
  
  var image: UIImage? {
    get { return imageView.image }
    set { imageView.image = newValue }
  }
}
