// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveCore
import SnapKit
import BraveShared

class AdsNotificationHandler: BraveAdsNotificationHandler {
  /// An ad was tapped and a URL should be opened
  var actionOccured: ((NotificationAd?, RewardsNotification.Action) -> Void)?
  /// The ads object
  let ads: BraveAds
  /// Whether or not we should currently show ads currently based on exteranl
  /// factors such as private mode
  var canShowNotifications: (() -> Bool)?
  /// The controller which we will show notifications on top of
  private(set) weak var presentingController: UIViewController?
  /// The controller that display, hide and manage notifications
  private weak var notificationsPresenter: BraveNotificationsPresenter?

  /// Create a handler instance with the given ads instance.
  ///
  /// - note: This method automatically sets `notificationsHandler` on BATBraveAds
  /// to itself
  init(
    ads: BraveAds,
    presentingController: UIViewController,
    notificationsPresenter: BraveNotificationsPresenter
  ) {
    self.ads = ads
    self.notificationsPresenter = notificationsPresenter
    self.presentingController = presentingController
    self.ads.notificationsHandler = self
  }
    
  func showNotificationAd(_ notification: NotificationAd) {
    guard let presentingController = presentingController else { return }
  
    let rewardsNotification = RewardsNotification(ad: notification) { [weak self] action in
      guard let self = self else { return }
      switch action {
      case .opened:
        self.ads.triggerNotificationAdEvent(notification.placementID, eventType: .clicked, completion: { _ in })
      case .dismissed:
        self.ads.triggerNotificationAdEvent(notification.placementID, eventType: .dismissed, completion: { _ in })
      case .timedOut:
        self.ads.triggerNotificationAdEvent(notification.placementID, eventType: .timedOut, completion: { _ in })
      }
      self.actionOccured?(notification, action)
    }
    
    ads.triggerNotificationAdEvent(notification.placementID, eventType: .viewed, completion: { _ in })
    notificationsPresenter?.display(notification: rewardsNotification, from: presentingController)
  }

  func closeNotificationAd(_ identifier: String) {
    notificationsPresenter?.removeNotification(with: identifier)
  }

  func canShowNotificationAds() -> Bool {
    guard let presentingController = presentingController,
      let rootVC = presentingController.currentScene?.browserViewController
    else { return false }
    func topViewController(startingFrom viewController: UIViewController) -> UIViewController {
      var top = viewController
      if let navigationController = top as? UINavigationController,
        let vc = navigationController.visibleViewController {
        return topViewController(startingFrom: vc)
      }
      if let tabController = top as? UITabBarController,
        let vc = tabController.selectedViewController {
        return topViewController(startingFrom: vc)
      }
      while let next = top.presentedViewController {
        top = next
      }
      return top
    }
    let isTopController = presentingController == topViewController(startingFrom: rootVC)
    let isTopWindow = presentingController.view.window?.isKeyWindow == true
    let canShowAds = canShowNotifications?() ?? true
    return isTopController && isTopWindow && canShowAds
  }
}
