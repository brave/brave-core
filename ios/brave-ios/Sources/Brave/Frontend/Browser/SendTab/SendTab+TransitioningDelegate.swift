/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import BraveUI

// MARK: - BasicAnimationControllerDelegate

extension SendTabTransitioningController: BasicAnimationControllerDelegate {
  public func animatePresentation(context: UIViewControllerContextTransitioning) {
    context.containerView.addSubview(view)
    
    backgroundView.alpha = 0.0
    contentView.transform = CGAffineTransform(translationX: 0, y: context.containerView.bounds.height)
    
    UIViewPropertyAnimator(duration: 0.35, dampingRatio: 1.0) { [self] in
      backgroundView.alpha = 1.0
      contentView.transform = .identity
    }.startAnimation()

    context.completeTransition(true)
  }

  public func animateDismissal(context: UIViewControllerContextTransitioning) {
    let animator = UIViewPropertyAnimator(duration: 0.25, dampingRatio: 1.0) { [self] in
      backgroundView.alpha = 0.0
      contentView.transform =
        CGAffineTransform(translationX: 0, y: context.containerView.bounds.height)
    }
    animator.addCompletion { _ in
      self.view.removeFromSuperview()
      context.completeTransition(true)
    }
    animator.startAnimation()
  }
}

// MARK: - UIViewControllerTransitioningDelegate

extension SendTabTransitioningController: UIViewControllerTransitioningDelegate {
  public func animationController(
    forPresented presented: UIViewController,
    presenting: UIViewController,
    source: UIViewController
  ) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: self, direction: .presenting)
  }

  public func animationController(
    forDismissed dismissed: UIViewController
  ) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: self, direction: .dismissing)
  }
}
