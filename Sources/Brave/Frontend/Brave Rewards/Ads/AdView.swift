// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import Shared

public class AdView: UIView {
  let adContentButton = AdContentButton()
  let openSwipeButton = AdSwipeButton(contentType: .text(Strings.Ads.open, textColor: .white)).then {
    $0.backgroundColor = .braveBlurpleTint
  }
  let dislikeSwipeButton = AdSwipeButton(contentType: .image(UIImage(named: "dislike-ad-icon", in: .module, compatibleWith: nil)!)).then {
    $0.backgroundColor = .braveErrorLabel
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
      let springTiming = UISpringTimingParameters(dampingRatio: 0.9)
      let animator = UIViewPropertyAnimator(duration: 0.3, timingParameters: springTiming)
      animator.addAnimations { [self] in
        adContentButton.transform.tx = tx
        setNeedsLayout()
        layoutIfNeeded()
      }
      animator.addCompletion { _ in
        completionBlock?()
      }
      animator.startAnimation()
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
