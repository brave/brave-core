// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveRewards
import pop
import SnapKit
import BraveShared

public class AdsNotificationHandler: BraveAdsNotificationHandler {
  /// An action type occuring on the ad
  public enum Action {
    /// The user opened the ad by either clicking on it directly or by swiping and clicking the "view" button
    case opened
    /// The user swiped the ad away
    case dismissed
    /// The user ignored the ad for a given amount of time for it to automatically dismiss
    case timedOut
    /// The user clicked the thumbs down button by swiping on the ad
    case disliked
  }
  /// An ad was tapped and a URL should be opened
  public var actionOccured: ((AdsNotification, Action) -> Void)?
  /// The ads object
  public let ads: BraveAds
  /// Whether or not we should currently show ads currently based on exteranl
  /// factors such as private mode
  public var canShowNotifications: (() -> Bool)?
  /// The controller which we will show notifications on top of
  public private(set) weak var presentingController: UIViewController?
  
  /// Create a handler instance with the given ads instance.
  ///
  /// - note: This method automatically sets `notificationsHandler` on BATBraveAds
  /// to itself
  public init(ads: BraveAds, presentingController: UIViewController) {
    self.ads = ads
    self.ads.notificationsHandler = self
    self.presentingController = presentingController
  }
  
  private var adsQueue: [AdsNotification] = []
  
  private lazy var adsViewController = AdsViewController()
  
  private func displayAd(notification: AdsNotification) {
    guard let presentingController = presentingController else { return }
    
    guard let window = presentingController.view.window else {
      return
    }
    
    if adsViewController.parent == nil {
      window.addSubview(adsViewController.view)
      adsViewController.view.snp.makeConstraints {
        $0.edges.equalTo(window.safeAreaLayoutGuide.snp.edges)
      }
    }
    
    self.ads.reportAdNotificationEvent(notification.uuid, eventType: .viewed)
    MonthlyAdsGrantReminder.schedule()
    
    adsViewController.display(ad: notification, handler: { [weak self] (notification, action) in
      guard let self = self else { return }
      switch action {
      case .opened:
        self.ads.reportAdNotificationEvent(notification.uuid, eventType: .clicked)
      case .dismissed:
        self.ads.reportAdNotificationEvent(notification.uuid, eventType: .dismissed)
      case .timedOut:
        self.ads.reportAdNotificationEvent(notification.uuid, eventType: .timedOut)
      case .disliked:
        self.ads.reportAdNotificationEvent(notification.uuid, eventType: .dismissed)
        self.ads.toggleThumbsDown(forAd: notification.uuid, creativeSetID: notification.creativeSetID)
      }
      self.actionOccured?(notification, action)
      
      if let nextAd = self.adsQueue.popLast() {
        self.displayAd(notification: nextAd)
      }
    }, animatedOut: { [weak self] in
      guard let self = self else { return }
      if self.adsViewController.visibleAdView == nil && self.adsQueue.isEmpty {
        self.adsViewController.willMove(toParent: nil)
        self.adsViewController.view.removeFromSuperview()
        self.adsViewController.removeFromParent()
      }
    })
  }
  
  public func show(_ notification: AdsNotification) {
    adsQueue.insert(notification, at: 0)
    if adsViewController.visibleAdView == nil {
      // Nothing currently waiting
      displayAd(notification: adsQueue.popLast()!)
    }
  }
  
  public func clearNotification(withIdentifier identifier: String) {
    adsQueue.removeAll(where: { $0.uuid == identifier })
  }
  
  public func shouldShowNotifications() -> Bool {
    guard let presentingController = presentingController,
      let rootVC = UIApplication.shared.delegate?.window??.rootViewController else { return false }
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
