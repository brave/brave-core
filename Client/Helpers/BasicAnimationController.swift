/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit

protocol BasicAnimationControllerDelegate: class {
  
  /// Animate the presentation of a controller
  ///
  /// - parameter context: The transitioning context
  /// - parameter finished: Whether or not the animation completed
  func animatePresentation(context: UIViewControllerContextTransitioning)
  
  /// Animate the dismissal of a controller
  ///
  /// - parameter context: The transitioning context
  /// - parameter finished: Whether or not the animation completed
  func animateDismissal(context: UIViewControllerContextTransitioning)
}

/// Defines an animation controller which simply redirects presentation/dismissal animations to its delegate.
///
/// This allows us to create complex animations within the controller which needs to be animated without having to mark
/// some sort of state for whether or not its being dismissed or presented.
///
/// It also allows us to access private variables/properties without having to expose them to the animation controller.
class BasicAnimationController: NSObject {
  
  /// The animation direction
  enum Direction {
    /// The controller is being presented
    case presenting
    /// The controller is being dismissed
    case dismissing
  }
  
  /// Whether or not this animation controller is being used for presentation or dismissal
  let direction: Direction
  
  /// The controller to handle animating
  private(set) weak var delegate: BasicAnimationControllerDelegate?
  
  init(delegate: BasicAnimationControllerDelegate, direction: Direction) {
    self.direction = direction
    self.delegate = delegate
  }
}

extension BasicAnimationController: UIViewControllerAnimatedTransitioning {
  
  func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
    // This value doesn't really matter... We could have it assignable as a property if we need it really...
    return 0.2
  }
  
  func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
    switch direction {
    case .presenting:
      delegate?.animatePresentation(context: transitionContext)
    case .dismissing:
      delegate?.animateDismissal(context: transitionContext)
    }
  }
}

