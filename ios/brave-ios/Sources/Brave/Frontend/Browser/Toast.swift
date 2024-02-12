/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SnapKit
import UIKit

class Toast: UIView {
  var animationConstraint: Constraint?
  var completionHandler: ((Bool) -> Void)?

  weak var viewController: UIViewController?

  var displayState = State.dismissed

  lazy var gestureRecognizer: UITapGestureRecognizer = {
    let gestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(handleTap))
    gestureRecognizer.cancelsTouchesInView = false
    return gestureRecognizer
  }()

  lazy var toastView: UIView = {
    let toastView = UIView()
    toastView.backgroundColor = SimpleToastUX.toastDefaultColor
    return toastView
  }()

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    superview?.addGestureRecognizer(gestureRecognizer)
  }

  func showToast(viewController: UIViewController? = nil, delay: DispatchTimeInterval, duration: DispatchTimeInterval?, makeConstraints: @escaping (SnapKit.ConstraintMaker) -> Swift.Void) {
    self.viewController = viewController

    self.displayState = .pendingShow

    DispatchQueue.main.asyncAfter(deadline: .now() + delay) {
      viewController?.view.addSubview(self)

      self.layer.removeAllAnimations()
      self.snp.makeConstraints(makeConstraints)
      self.layoutIfNeeded()

      UIView.animate(
        withDuration: SimpleToastUX.toastAnimationDuration,
        animations: {
          self.animationConstraint?.update(offset: 0)
          self.layoutIfNeeded()
        }
      ) { finished in
        self.displayState = .showing
        if let duration = duration {
          DispatchQueue.main.asyncAfter(deadline: .now() + duration) {
            self.dismiss(false)
          }
        }
      }
    }
  }

  func dismiss(_ buttonPressed: Bool) {
    if displayState == .pendingDismiss || displayState == .dismissed {
      return
    }

    displayState = .pendingDismiss
    superview?.removeGestureRecognizer(gestureRecognizer)
    layer.removeAllAnimations()

    UIView.animate(
      withDuration: SimpleToastUX.toastAnimationDuration,
      animations: {
        self.animationConstraint?.update(offset: SimpleToastUX.toastHeight)
        self.layoutIfNeeded()
      }
    ) { finished in
      self.displayState = .dismissed
      self.removeFromSuperview()
      if !buttonPressed {
        self.completionHandler?(false)
      }
    }
  }

  @objc func handleTap(_ gestureRecognizer: UIGestureRecognizer) {
    dismiss(false)
  }

  enum State {
    case showing
    case pendingShow
    case pendingDismiss
    case dismissed
  }
}
