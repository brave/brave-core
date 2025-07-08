// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import Shared
import UIKit
import os.log

extension TabGridHostingController: UIViewControllerTransitioningDelegate {
  func animationController(
    forPresented presented: UIViewController,
    presenting: UIViewController,
    source: UIViewController
  ) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: self, direction: .presenting)
  }

  func animationController(
    forDismissed dismissed: UIViewController
  ) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: self, direction: .dismissing)
  }
}

extension TabGridHostingController: BasicAnimationControllerDelegate {
  func animatePresentation(context: any UIViewControllerContextTransitioning) {
    let finalFrame = context.finalFrame(for: self)
    guard let fromView = context.view(forKey: .from),
      let selectedTab = tabManager.selectedTab
    else {
      view.frame = finalFrame
      context.containerView.addSubview(view)
      context.completeTransition(true)
      return
    }

    let dimmingView = UIVisualEffectView(effect: UIBlurEffect(style: .systemUltraThinMaterial))
    dimmingView.backgroundColor = .clear
    dimmingView.frame = finalFrame

    context.containerView.insertSubview(view, belowSubview: fromView)
    context.containerView.insertSubview(dimmingView, belowSubview: fromView)

    view.frame = finalFrame
    view.setNeedsLayout()
    view.layoutIfNeeded()

    guard let destinationIndexPath = containerView.dataSource.indexPath(for: selectedTab.id),
      let destinationCell = containerView.collectionView.cellForItem(at: destinationIndexPath),
      destinationCell.bounds.size != .zero,
      let destinationCellSnapshot = destinationCell.snapshotView(afterScreenUpdates: true)
    else {
      let fallbackAnimation = UIViewPropertyAnimator(duration: 0.3, curve: .linear)
      fallbackAnimation.addAnimations {
        fromView.alpha = 0
      }
      fallbackAnimation.addCompletion { _ in
        context.completeTransition(true)
      }
      fallbackAnimation.startAnimation()
      return
    }

    context.containerView.insertSubview(destinationCellSnapshot, aboveSubview: dimmingView)

    let cellCenterInContainer = context.containerView.convert(
      destinationCell.center,
      from: containerView.collectionView
    )
    let cellFrameInContainer = destinationCell.convert(
      destinationCell.bounds,
      to: context.containerView
    )

    destinationCell.alpha = 0
    destinationCellSnapshot.center = context.containerView.convert(
      fromView.center,
      to: context.containerView
    )
    destinationCellSnapshot.transform = .init(
      scaleX: fromView.bounds.width / destinationCell.bounds.width,
      y: fromView.bounds.height / destinationCell.bounds.height
    )

    fromView.layer.cornerCurve = .continuous
    fromView.layer.cornerRadius = 16 * max(destinationCell.transform.a, destinationCell.transform.d)

    containerView.collectionView.transform = .init(scaleX: 0.9, y: 0.9)

    let animator = UIViewPropertyAnimator(duration: 0.4, dampingRatio: 1)
    animator.addAnimations { [self] in
      containerView.collectionView.transform = .identity
      destinationCellSnapshot.transform = .identity
      destinationCellSnapshot.center = cellCenterInContainer
      fromView.alpha = 0
      fromView.transform = .init(
        scaleX: cellFrameInContainer.width / fromView.frame.width,
        y: cellFrameInContainer.height / fromView.frame.height
      )
      fromView.layer.cornerRadius = 16
      fromView.center = .init(x: cellFrameInContainer.midX, y: cellFrameInContainer.midY)
      containerView.collectionView.transform = .identity
    }
    animator.addAnimations(
      {
        dimmingView.alpha = 0
      },
      delayFactor: 0.1
    )
    animator.addCompletion { _ in
      fromView.transform = .identity
      fromView.alpha = 1
      fromView.layer.cornerRadius = 0
      destinationCell.alpha = 1
      destinationCellSnapshot.removeFromSuperview()
      dimmingView.removeFromSuperview()
      context.completeTransition(true)
    }
    animator.startAnimation()
  }

  func animateDismissal(context: any UIViewControllerContextTransitioning) {
    guard let toView = context.view(forKey: .to),
      let toVC = context.viewController(forKey: .to)
    else {
      context.completeTransition(true)
      return
    }

    let finalFrame = context.finalFrame(for: toVC)
    let dimmingView = UIVisualEffectView(effect: UIBlurEffect(style: .systemUltraThinMaterial))
    dimmingView.backgroundColor = .clear
    dimmingView.frame = finalFrame
    dimmingView.alpha = 0

    toView.frame = finalFrame
    toView.alpha = 0

    context.containerView.addSubview(dimmingView)
    context.containerView.addSubview(toView)

    toView.setNeedsLayout()
    toView.layoutIfNeeded()

    guard let selectedTab = tabManager.selectedTab,
      let sourceIndexPath = containerView.dataSource.indexPath(for: selectedTab.id),
      let sourceAttributes = containerView.collectionView.layoutAttributesForItem(
        at: sourceIndexPath
      )
    else {
      let fallbackAnimation = UIViewPropertyAnimator(duration: 0.3, curve: .linear)
      toView.alpha = 0
      fallbackAnimation.addAnimations {
        toView.alpha = 1
      }
      fallbackAnimation.addCompletion { _ in
        context.completeTransition(true)
      }
      fallbackAnimation.startAnimation()
      return
    }

    // If the source cell is visible on the screen we'll include a snapshot of said cell to
    // animate from
    let sourceCell = containerView.collectionView.cellForItem(at: sourceIndexPath)
    let sourceCellSnapshot = sourceCell?.snapshotView(afterScreenUpdates: true)
    if let sourceCell, let sourceCellSnapshot {
      context.containerView.insertSubview(sourceCellSnapshot, aboveSubview: dimmingView)
      sourceCellSnapshot.center = context.containerView.convert(
        sourceAttributes.center,
        from: containerView.collectionView
      )
      sourceCell.alpha = 0
    }

    toView.layer.cornerCurve = .continuous
    toView.layer.cornerRadius = 16

    toView.center = containerView.collectionView.convert(
      sourceAttributes.center,
      to: context.containerView
    )
    toView.transform = .init(
      scaleX: sourceAttributes.bounds.width / toView.bounds.width,
      y: sourceAttributes.bounds.height / toView.bounds.height
    )

    let animator = UIViewPropertyAnimator(duration: 0.4, dampingRatio: 1)
    animator.addAnimations { [self] in
      dimmingView.alpha = 1
      toView.alpha = 1
      toView.center = finalFrame.center
      containerView.collectionView.transform = .init(scaleX: 0.9, y: 0.9)
      sourceCellSnapshot?.transform = .init(
        scaleX: view.bounds.width / sourceAttributes.bounds.width,
        y: view.bounds.height / sourceAttributes.bounds.height
      )
      sourceCellSnapshot?.center = context.containerView.convert(
        view.center,
        from: view.superview
      )
      toView.layer.cornerRadius = 0
      toView.transform = .identity
    }
    animator.addCompletion { _ in
      toView.layer.cornerRadius = 0
      sourceCellSnapshot?.removeFromSuperview()
      dimmingView.removeFromSuperview()
      context.completeTransition(!context.transitionWasCancelled)
    }
    animator.startAnimation()
  }
}
