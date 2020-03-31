// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import pop
import BraveUI

public class AdView: UIView {
  let adContentButton = AdContentButton()
  let openSwipeButton = AdSwipeButton(contentType: .text(Strings.open, textColor: .white)).then {
    $0.backgroundColor = Colors.blurple500
  }
  let dislikeSwipeButton = AdSwipeButton(contentType: .image(UIImage(frameworkResourceNamed: "thumbsdown"))).then {
    $0.backgroundColor = Colors.red600
  }
  
  public override init(frame: CGRect) {
    super.init(frame: frame)
   
    addSubview(adContentButton)
//    addSubview(openSwipeButton)
    addSubview(dislikeSwipeButton)
    
    adContentButton.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }
  
  public override func layoutSubviews() {
    super.layoutSubviews()
    
//    let openWidth = max(0, adContentButton.frame.minX - 8)
//    openSwipeButton.frame = CGRect(
//      x: 0,
//      y: 0,
//      width: openWidth,
//      height: adContentButton.bounds.height
//    )
//    openSwipeButton.alpha = max(0, openWidth - 30) / 20
    
    let dislikeWidth = max(0, bounds.width - adContentButton.frame.maxX - 8)
    dislikeSwipeButton.frame = CGRect(
      x: adContentButton.frame.maxX + 8,
      y: 0,
      width: dislikeWidth,
      height: adContentButton.bounds.height
    )
    dislikeSwipeButton.alpha = max(0, dislikeWidth - 30) / 20
  }
  
  var swipeTranslation: CGFloat {
    return adContentButton.transform.tx
  }
  
  /// Set the horizontal swipe translation
  func setSwipeTranslation(_ tx: CGFloat, animated: Bool = false, panVelocity: CGFloat? = nil, completionBlock: (() -> Void)? = nil) {
    if animated {
      adContentButton.layer.springAnimate(property: kPOPLayerTranslationX, key: "translation.x") { animation, _ in
        animation.toValue = tx
        if let velocity = panVelocity {
          animation.velocity = velocity
        }
        animation.animationDidApplyBlock = { _ in
          self.setNeedsLayout()
        }
        if let completionBlock = completionBlock {
          animation.completionBlock = { _, _ in
            completionBlock()
          }
        }
      }
    } else {
      adContentButton.transform.tx = tx
      setNeedsLayout()
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
