// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import UIKit
import Web

class TabGridItemCell: UICollectionViewCell {
  let model: TabGridItemViewModel = .init()
  var actionHandler: ((TabGridItemView.Action) -> Void)?
  var isPrivateBrowsing: Bool = false {
    didSet {
      model.isPrivateBrowsing = isPrivateBrowsing
    }
  }

  override init(frame: CGRect) {
    super.init(frame: .zero)

    contentConfiguration = UIHostingConfiguration {
      TabGridItemView(
        viewModel: model,
        actionHandler: { [unowned self] in
          handleAction($0)
        }
      )
    }
    .margins(.all, 0)

    let panGesture = UIPanGestureRecognizer(target: self, action: #selector(handlePan))
    panGesture.delegate = self
    addGestureRecognizer(panGesture)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    model.reset()
  }

  func configure(with tab: some TabState) {
    model.configure(with: tab)
  }

  private func handleAction(_ action: TabGridItemView.Action) {
    actionHandler?(action)
  }
}

extension TabGridItemCell: UIGestureRecognizerDelegate {
  private func computedOffsetBasedOnRubberBandingResistance(
    distance x: CGFloat,
    constant c: CGFloat = 0.55,
    dimension d: CGFloat
  ) -> CGFloat {
    // f(x, d, c) = (x * d * c) / (d + c * x)
    //
    // where,
    // x – distance from the edge
    // c – constant (UIScrollView uses 0.55)
    // d – dimension, either width or height
    return (x * d * c) / (d + c * x)
  }

  // Distance travelled after decelerating to zero velocity at a constant rate
  private func project(
    initialVelocity: CGFloat,
    decelerationRate: CGFloat = UIScrollView.DecelerationRate.normal.rawValue
  ) -> CGFloat {
    (initialVelocity / 1000.0) * decelerationRate / (1.0 - decelerationRate)
  }

  @objc private func handlePan(_ pan: UIPanGestureRecognizer) {
    switch pan.state {
    case .began:
      layer.zPosition = 999
      model.swipeToDeleteGestureState = .init(initialCenter: center)
      let scaleAnimator = UIViewPropertyAnimator(duration: 0.5, dampingRatio: 1)
      scaleAnimator.addAnimations { [self] in
        transform = .init(scaleX: 1.1, y: 1.1)
      }
      scaleAnimator.startAnimation()
    case .changed:
      guard let state = model.swipeToDeleteGestureState else { return }
      let isRTL = traitCollection.layoutDirection == .rightToLeft
      var translation = pan.translation(in: self).x
      if isRTL {
        translation *= -1
      }
      if translation > 0 {
        // If we're attempting to drag towards the trailing edge of the screen, apply a rubber
        // banded translation
        translation = computedOffsetBasedOnRubberBandingResistance(
          distance: translation,
          dimension: bounds.width
        )
      }
      center.x = state.initialCenter.x + translation
    case .ended, .cancelled:
      guard let state = model.swipeToDeleteGestureState else { return }
      let isRTL = traitCollection.layoutDirection == .rightToLeft
      let translation = pan.translation(in: self).x
      let projectedEndTranslation = translation + project(initialVelocity: pan.velocity(in: self).x)
      let willCloseTab =
        isRTL ? projectedEndTranslation > bounds.width : projectedEndTranslation < -bounds.width
      let finalCenter =
        willCloseTab ? -(superview!.bounds.width - bounds.width) / 2 : state.initialCenter.x
      let transformDelta = finalCenter - center.x
      let initialVelocity = CGVector(
        dx: transformDelta.isZero ? 0 : pan.velocity(in: self).x / transformDelta,
        dy: 0
      )
      let centerAnimator = UIViewPropertyAnimator(
        duration: 0.3,
        timingParameters: UISpringTimingParameters(
          dampingRatio: 1,
          initialVelocity: initialVelocity
        )
      )
      // Ensure that the animation can't be removed when the collection view applies a snaphsot
      centerAnimator.isInterruptible = false
      centerAnimator.addAnimations { [self] in
        center.x = finalCenter
      }
      centerAnimator.addCompletion { [self] _ in
        layer.zPosition = 0
      }
      centerAnimator.startAnimation()
      if !willCloseTab {
        // Use a separate animator for the scale transform since we don't want velocity affecting it
        let scaleAnimator = UIViewPropertyAnimator(duration: 0.3, dampingRatio: 1)
        scaleAnimator.addAnimations { [self] in
          transform = .identity
        }
        scaleAnimator.startAnimation()
      }

      if !willCloseTab {
        model.swipeToDeleteGestureState = nil
      }
      if willCloseTab {
        // Thread-hopping helps with animation hitching
        DispatchQueue.main.async {
          self.handleAction(.closedTab)
        }
      }
    default:
      break
    }
  }

  override func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    guard let pan = gestureRecognizer as? UIPanGestureRecognizer else { return false }
    let translation = pan.translation(in: pan.view)
    return abs(translation.x) > abs(translation.y)
  }
}
