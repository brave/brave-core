// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Preferences
import Shared

class RewardsNotification: NSObject, BraveNotification {
  enum Action {
    /// The user opened the ad by either clicking on it directly or by swiping and clicking the "view" button
    case opened
    /// The user swiped the ad away
    case dismissed
    /// The user ignored the ad for a given amount of time for it to automatically dismiss
    case timedOut
    /// The user clicked the thumbs down button by swiping on the ad
    case disliked
  }
  
  var view: UIView
  var dismissAction: (() -> Void)?
  var id: String { ad.placementID }
  let ad: NotificationAd
  var dismissPolicy: DismissPolicy {
    guard let adView = view as? AdView else { return .automatic() }
    
    var dismissTimeInterval: TimeInterval = 30
    if !AppConstants.buildChannel.isPublic, let override = Preferences.Rewards.adsDurationOverride.value, override > 0 {
      dismissTimeInterval = TimeInterval(override)
    }
    return adView.swipeTranslation != 0 ? .explicit : .automatic(after: dismissTimeInterval)
  }
  
  private let handler: (Action) -> Void
  
  func willDismiss(timedOut: Bool) {
    guard let adView = view as? AdView else { return }
    adView.setSwipeTranslation(0, animated: true)
    handler(timedOut ? .timedOut : .dismissed)
  }
  
  init(
    ad: NotificationAd,
    handler: @escaping (Action) -> Void
  ) {
    self.ad = ad
    self.view = AdView()
    self.handler = handler
    super.init()
    self.setup()
  }
  
  private func setup() {
    guard let adView = view as? AdView else { return }
    
    adView.adContentButton.titleLabel.text = ad.title
    adView.adContentButton.bodyLabel.text = ad.body
    
    adView.adContentButton.addTarget(self, action: #selector(tappedAdView(_:)), for: .touchUpInside)
    adView.openSwipeButton.addTarget(self, action: #selector(tappedOpen(_:)), for: .touchUpInside)
    adView.dislikeSwipeButton.addTarget(self, action: #selector(tappedDisliked(_:)), for: .touchUpInside)
    
    let swipePanGesture = UIPanGestureRecognizer(target: self, action: #selector(swipePannedAdView(_:)))
    swipePanGesture.delegate = self
    adView.addGestureRecognizer(swipePanGesture)
  }
  
  @objc private func tappedAdView(_ sender: AdContentButton) {
    guard let adView = sender.superview as? AdView else { return }
    if sender.transform.tx != 0 {
      adView.setSwipeTranslation(0, animated: true)
      return
    }
    dismissAction?()
    handler(.opened)
  }
  
  @objc private func tappedOpen(_ sender: AdSwipeButton) {
    dismissAction?()
    handler(.opened)
  }
  
  @objc private func tappedDisliked(_ sender: AdSwipeButton) {
    dismissAction?()
    handler(.disliked)
  }
  
  // Distance travelled after decelerating to zero velocity at a constant rate
  private func project(initialVelocity: CGFloat, decelerationRate: CGFloat) -> CGFloat {
    return (initialVelocity / 1000.0) * decelerationRate / (1.0 - decelerationRate)
  }
  
  private let actionTriggerThreshold: CGFloat = 180.0
  private let actionRestThreshold: CGFloat = 90.0
  
  private var swipeState: CGFloat = 0
  @objc private func swipePannedAdView(_ pan: UIPanGestureRecognizer) {
    guard let adView = pan.view as? AdView else { return }
    switch pan.state {
    case .began:
      swipeState = adView.adContentButton.transform.tx
    case .changed:
      let tx = swipeState + pan.translation(in: adView).x
      if tx < -actionTriggerThreshold && !adView.dislikeSwipeButton.isHighlighted {
        UIImpactFeedbackGenerator(style: .medium).bzzt()
      }
      adView.dislikeSwipeButton.isHighlighted = tx < -actionTriggerThreshold
      adView.adContentButton.transform.tx = min(0, tx)
      adView.setNeedsLayout()
    case .ended:
      let velocity = pan.velocity(in: adView).x
      let tx = swipeState + pan.translation(in: adView).x
      let projected = project(initialVelocity: velocity, decelerationRate: UIScrollView.DecelerationRate.normal.rawValue)
      if /*tx > actionTriggerThreshold ||*/ tx < -actionTriggerThreshold {
        adView.setSwipeTranslation(0, animated: true, panVelocity: velocity)
        dismissAction?()
        handler(tx > 0 ? .opened : .disliked)
        break
      } else if /*tx + projected > actionRestThreshold ||*/ tx + projected < -actionRestThreshold {
        adView.setSwipeTranslation((tx + projected) > 0 ? actionRestThreshold : -actionRestThreshold, animated: true, panVelocity: velocity)
        break
      }
      fallthrough
    case .cancelled:
      adView.setSwipeTranslation(0, animated: true)
    default:
      break
    }
  }
}

extension RewardsNotification: UIGestureRecognizerDelegate {
  
  func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    if let pan = gestureRecognizer as? UIPanGestureRecognizer {
      let velocity = pan.velocity(in: pan.view)
      // Horizontal only
      return abs(velocity.x) > abs(velocity.y)
    }
    return false
  }
}

extension RewardsNotification {
  /// Display a "My First Ad" on a presenting controller and be notified if they tap it
  static func displayFirstAd(
    with presenter: BraveNotificationsPresenter,
    on presentingController: UIViewController,
    completion: @escaping (RewardsNotification.Action, URL) -> Void
  ) {
    let notification = NotificationAd.customAd(
      title: Strings.Ads.myFirstAdTitle,
      body: Strings.Ads.myFirstAdBody,
      url: "https://brave.com/my-first-ad"
    )
    
    guard let targetURL = URL(string: notification.targetURL) else {
      assertionFailure("My First Ad URL is not valid: \(notification.targetURL)")
      return
    }
    
    let rewardsNotification = RewardsNotification(ad: notification) { action in
      completion(action, targetURL)
    }
    presenter.display(notification: rewardsNotification, from: presentingController)
  }
}
