// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveCore
import BraveShared
import Shared

private class BraveNotificationGesture: UIGestureRecognizer {
  var onBegan: () -> Void
  var onEnded: () -> Void
  
  init(
    onBegan: @escaping () -> Void,
    onEnded: @escaping () -> Void
  ) {
    self.onBegan = onBegan
    self.onEnded = onEnded
    super.init(target: nil, action: nil)
    cancelsTouchesInView = false
    delaysTouchesBegan = false
    delaysTouchesEnded = false
  }
  
  override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent) {
    super.touchesBegan(touches, with: event)
    onBegan()
  }
  
  override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent) {
    super.touchesEnded(touches, with: event)
    onEnded()
  }
  
  override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent) {
    super.touchesCancelled(touches, with: event)
    onEnded()
  }
}

class BraveNotificationsPresenter: UIViewController {
  private var notificationsQueue: [BraveNotification] = []
  private var widthAnchor: NSLayoutConstraint?
  private var visibleNotification: BraveNotification?
  private weak var currentPresentingVC: UIViewController?
  
  override func loadView() {
    let view = PresenterView(frame: UIScreen.main.bounds)
    view.viewController = self
    self.view = view
  }
  
  func display(notification: BraveNotification, from presentingController: UIViewController) {
    // check the priority of the notification
    if let visibleNotification = visibleNotification {
      if notification.priority <= visibleNotification.priority {
        // won't display if the incoming notification has the same or lower priority
        // put it in queue
        enqueueNotification(notification)
        return
      } else {
        if let timer = dismissTimers[visibleNotification.id] {
          timer.invalidate()
        }
        // will hide the current visible notification and display the incoming notification
        // if the notification has higher priority
        self.hide(visibleNotification)
      }
    }
    
    guard let window = presentingController.view.window else { return }
    
    currentPresentingVC = presentingController
    window.addSubview(view)
    view.snp.makeConstraints {
      $0.edges.equalTo(window.safeAreaLayoutGuide.snp.edges)
    }
    
    let notificationView = notification.view
    view.subviews.forEach { $0.removeFromSuperview() }
    view.addSubview(notificationView)
    notificationView.snp.makeConstraints {
      $0.leading.greaterThanOrEqualTo(view).inset(8)
      $0.trailing.lessThanOrEqualTo(view).inset(8)
      $0.centerX.equalTo(view)
      $0.top.equalTo(view.safeAreaLayoutGuide.snp.top)
      $0.top.greaterThanOrEqualTo(view).offset(4) // Makes sure in landscape its at least 4px from the top
      
      if UIDevice.current.userInterfaceIdiom != .pad {
        $0.width.equalTo(view).priority(.high)
      }
    }
    
    if UIDevice.current.userInterfaceIdiom == .pad {
      widthAnchor = notificationView.widthAnchor.constraint(equalToConstant: 0.0)
      widthAnchor?.priority = .defaultHigh
      widthAnchor?.isActive = true
    }
    
    view.layoutIfNeeded()
    
    notification.dismissAction = { [weak self] in
      guard let self = self else { return }
      self.hide(notification)
    }
    animateIn(adView: notificationView)
    if case .automatic(let interval) = notification.dismissPolicy {
      setupTimeoutTimer(for: notification, interval: interval)
    }
    visibleNotification = notification
    
    // Add common swip gesture (swip-up to dismiss)
    let dismissPanGesture = UIPanGestureRecognizer(target: self, action: #selector(dismissPannedAdView(_:)))
    dismissPanGesture.delegate = self
    notificationView.addGestureRecognizer(dismissPanGesture)
    
    let customGesture = BraveNotificationGesture(
      onBegan: { [weak self] in
        if let visibleNotification = self?.visibleNotification {
          self?.dismissTimers[visibleNotification.id]?.invalidate()
        }
      },
      onEnded: { [weak self] in
        if let visibleNotification = self?.visibleNotification,
           case .automatic(let interval) = visibleNotification.dismissPolicy {
          self?.setupTimeoutTimer(for: visibleNotification, interval: interval)
        }
      }
    )
    customGesture.delegate = self
    notificationView.addGestureRecognizer(customGesture)
  }
  
  override func viewWillLayoutSubviews() {
    super.viewWillLayoutSubviews()
    
    if UIDevice.current.userInterfaceIdiom == .pad {
      let constant = max(view.bounds.width, view.bounds.height) * 0.40
      widthAnchor?.constant = ceil(constant * UIScreen.main.scale) / UIScreen.main.scale
    }
  }
  
  func removeNotification(with id: String) {
    if let index = notificationsQueue.firstIndex(where: { $0.id == id }) {
      notificationsQueue.remove(at: index)
    }
    if let visibleNotification = visibleNotification, visibleNotification.id == id {
      hide(visibleNotification)
    }
  }
  
  func hide(_ notification: BraveNotification) {
    hide(notificationView: notification.view, velocity: nil)
  }
  
  private func hide(notificationView: UIView, velocity: CGFloat?) {
    visibleNotification = nil
    animateOut(adView: notificationView, velocity: velocity) { [weak self] in
      guard let self = self else { return }
      notificationView.removeFromSuperview()
      if self.visibleNotification == nil {
        if self.notificationsQueue.isEmpty {
          self.willMove(toParent: nil)
          self.view.removeFromSuperview()
          self.removeFromParent()
        } else {
          guard let presentingController = self.currentPresentingVC else { return }
          self.display(notification: self.notificationsQueue.popLast()!, from: presentingController)
        }
      }
    }
  }
  
  private func enqueueNotification(_ notification: BraveNotification) {
    // We will skip duplication checking for notifications that have empty id. These notifications are usually custom ads
    if !notification.id.isEmpty,
       notificationsQueue.contains(where: { $0.id == notification.id }) || visibleNotification?.id == notification.id {
      return
    }
    
    if let index = notificationsQueue.firstIndex(where: { $0.priority < notification.priority }) {
      notificationsQueue.insert(notification, at: index + 1)
    } else {
      notificationsQueue.insert(notification, at: 0)
    }
  }
  
  deinit {
    dismissTimers.forEach({ $0.value.invalidate() })
  }
  
  // MARK: - Actions
  
  private var dismissTimers: [String: Timer] = [:]
  
  private func setupTimeoutTimer(for notification: BraveNotification, interval: TimeInterval) {
    if let timer = dismissTimers[notification.id] {
      // Invalidate and reschedule
      timer.invalidate()
    }
    dismissTimers[notification.id] = Timer.scheduledTimer(withTimeInterval: interval, repeats: false, block: { [weak self] _ in
      guard let self = self, case .automatic(_) = notification.dismissPolicy else { return }
      notification.willDismiss(timedOut: true)
      self.hide(notification)
    })
  }
  
  // Distance travelled after decelerating to zero velocity at a constant rate
  func project(initialVelocity: CGFloat, decelerationRate: CGFloat) -> CGFloat {
    return (initialVelocity / 1000.0) * decelerationRate / (1.0 - decelerationRate)
  }
  
  private var panState: CGPoint = .zero
  @objc private func dismissPannedAdView(_ pan: UIPanGestureRecognizer) {
    guard let notificationView = pan.view else { return }
    
    switch pan.state {
    case .began:
      panState = notificationView.center
      // must be the current visible notification
      guard let notification = visibleNotification, notification.view == notificationView else {
        return
      }
      // Make sure to stop the dismiss timer
      dismissTimers[notification.id]?.invalidate()
    case .changed:
      notificationView.transform.ty = min(0, pan.translation(in: notificationView).y)
    case .ended:
      let velocity = pan.velocity(in: notificationView).y
      let y = min(panState.y, panState.y + pan.translation(in: notificationView).y)
      let projected = project(initialVelocity: velocity, decelerationRate: UIScrollView.DecelerationRate.normal.rawValue)
      if y + projected < 0 {
        // must be the current visible notification
        guard let notification = visibleNotification, notification.view == notificationView else {
          return
        }
        hide(notificationView: notificationView, velocity: velocity)
        notification.willDismiss(timedOut: false)
        break
      }
      fallthrough
    case .cancelled:
      // must be the current visible notification
      guard let notification = visibleNotification, notification.view == notificationView else {
        return
      }
      // Re-setup timeout timer
      if case .automatic(let interval) = notification.dismissPolicy {
        setupTimeoutTimer(for: notification, interval: interval)
      }
      UIViewPropertyAnimator(duration: 0.3, dampingRatio: 0.9) {
        notificationView.transform = CGAffineTransform(translationX: 0, y: 0)
      }
      .startAnimation()
    default:
      break
    }
  }
  
  // MARK: - Animations
  
  private func animateIn(adView: UIView) {
    adView.layoutIfNeeded()
    adView.layer.transform = CATransform3DMakeTranslation(0, -adView.bounds.size.height, 0)
    
    UIViewPropertyAnimator(duration: 0.3, dampingRatio: 0.9) {
      adView.transform = CGAffineTransform(translationX: 0, y: 0)
    }
    .startAnimation()
  }
  
  private func animateOut(adView: UIView, velocity: CGFloat? = nil, completion: @escaping () -> Void) {
    adView.layoutIfNeeded()
    let springTiming = UISpringTimingParameters(dampingRatio: 0.9, initialVelocity: velocity.map { CGVector(dx: 0, dy: $0) } ?? .zero)
    let animator = UIViewPropertyAnimator(duration: 0.3, timingParameters: springTiming)
    animator.addAnimations { [self] in
      adView.transform = CGAffineTransform(translationX: 0, y: -(view.frame.origin.y + adView.bounds.size.height))
    }
    animator.addCompletion { _ in
      completion()
    }
    animator.startAnimation()
  }
}

extension BraveNotificationsPresenter {
  class PresenterView: UIView {
    weak var viewController: BraveNotificationsPresenter?
    
    override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
      guard let view = super.hitTest(point, with: event) else { return nil }
      // Only allow tapping the nitification part of this VC
      if let notificationView = viewController?.visibleNotification?.view, view.isDescendant(of: notificationView) {
        return view
      }
      return nil
    }
  }
}

extension BraveNotificationsPresenter: UIGestureRecognizerDelegate {
  
  func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    if let pan = gestureRecognizer as? UIPanGestureRecognizer {
      let velocity = pan.velocity(in: pan.view)
      return abs(velocity.y) > abs(velocity.x)
    }
    return false
  }
}
