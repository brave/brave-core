// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import BraveShared
import Shared
import pop

public class AdsViewController: UIViewController {
  public typealias ActionHandler = (AdsNotification, AdsNotificationHandler.Action) -> Void
  private var widthAnchor: NSLayoutConstraint?
  
  private struct DisplayedAd {
    let ad: AdsNotification
    let handler: ActionHandler
    let animatedOut: () -> Void
  }
  
  /// The number of seconds until the ad is automatically dismissed
  private let automaticDismissalInterval: TimeInterval = 30
  
  private var displayedAds: [AdView: DisplayedAd] = [:]
  private(set) var visibleAdView: AdView?
  
  public override func loadView() {
    view = View(frame: UIScreen.main.bounds)
  }
  
  private let dismissGestureName = "dismiss"
  private let swipeGestureName = "swipe"
  
  public func display(ad: AdsNotification, handler: @escaping ActionHandler, animatedOut: @escaping () -> Void) {
    let adView = AdView()
    adView.adContentButton.titleLabel.text = ad.title
    adView.adContentButton.bodyLabel.text = ad.body
    
    view.addSubview(adView)
    
    adView.snp.makeConstraints {
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
      widthAnchor = adView.widthAnchor.constraint(equalToConstant: 0.0)
      widthAnchor?.priority = .defaultHigh
      widthAnchor?.isActive = true
    }
    
    view.layoutIfNeeded()
    
    animateIn(adView: adView)
    
    visibleAdView = adView
    displayedAds[adView] = DisplayedAd(ad: ad, handler: handler, animatedOut: animatedOut)
    
    setupTimeoutTimer(for: adView)
    
    adView.adContentButton.addTarget(self, action: #selector(touchDownAdView(_:)), for: .touchDown)
    adView.adContentButton.addTarget(self, action: #selector(touchUpOutsideAdView(_:)), for: .touchUpOutside)
    adView.adContentButton.addTarget(self, action: #selector(tappedAdView(_:)), for: .touchUpInside)
    adView.openSwipeButton.addTarget(self, action: #selector(tappedOpen(_:)), for: .touchUpInside)
    adView.dislikeSwipeButton.addTarget(self, action: #selector(tappedDisliked(_:)), for: .touchUpInside)
    let dismissPanGesture = UIPanGestureRecognizer(target: self, action: #selector(dismissPannedAdView(_:)))
    dismissPanGesture.name = dismissGestureName
    dismissPanGesture.delegate = self
    adView.addGestureRecognizer(dismissPanGesture)
    
    let swipePanGesture = UIPanGestureRecognizer(target: self, action: #selector(swipePannedAdView(_:)))
    swipePanGesture.name = swipeGestureName
    swipePanGesture.delegate = self
    adView.addGestureRecognizer(swipePanGesture)
  }
  
  public override func viewWillLayoutSubviews() {
    super.viewWillLayoutSubviews()
    
    if UIDevice.current.userInterfaceIdiom == .pad {
      let constant = max(view.bounds.width, view.bounds.height) * 0.40
      widthAnchor?.constant = ceil(constant * UIScreen.main.scale) / UIScreen.main.scale
    }
  }
  
  public func hide(adView: AdView) {
    hide(adView: adView, velocity: nil)
  }
  
  private func hide(adView: AdView, velocity: CGFloat?) {
    visibleAdView = nil
    let handler = displayedAds[adView]
    displayedAds[adView] = nil
    animateOut(adView: adView, velocity: velocity) {
      adView.removeFromSuperview()
      handler?.animatedOut()
    }
  }
  
  deinit {
    dismissTimers.forEach({ $0.value.invalidate() })
  }
  
  // MARK: - Actions
  
  private var dismissTimers: [AdView: Timer] = [:]
  
  private func setupTimeoutTimer(for adView: AdView) {
    if let timer = dismissTimers[adView] {
      // Invalidate and reschedule
      timer.invalidate()
    }
    var dismissInterval = automaticDismissalInterval
    if !AppConstants.buildChannel.isPublic, let override = Preferences.Rewards.adsDurationOverride.value, override > 0 {
      dismissInterval = TimeInterval(override)
    }
    dismissTimers[adView] = Timer.scheduledTimer(withTimeInterval: dismissInterval, repeats: false, block: { [weak self] _ in
      guard let self = self, let handler = self.displayedAds[adView] else { return }
      self.hide(adView: adView)
      handler.handler(handler.ad, .timedOut)
    })
  }
  
  @objc private func touchDownAdView(_ sender: AdContentButton) {
    guard let adView = sender.superview as? AdView else { return }
    // Make sure the ad doesnt disappear under the users finger
    dismissTimers[adView]?.invalidate()
  }
  
  @objc private func touchUpOutsideAdView(_ sender: AdContentButton) {
    guard let adView = sender.superview as? AdView else { return }
    setupTimeoutTimer(for: adView)
  }
  
  @objc private func tappedAdView(_ sender: AdContentButton) {
    guard let adView = sender.superview as? AdView else { return }
    
    if sender.transform.tx != 0 {
      adView.setSwipeTranslation(0, animated: true) {
        self.setupTimeoutTimer(for: adView)
      }
      return
    }
    
    guard let displayedAd = displayedAds[adView] else { return }
    hide(adView: adView)
    displayedAd.handler(displayedAd.ad, .opened)
  }
  
  @objc private func tappedOpen(_ sender: AdSwipeButton) {
    guard let adView = sender.superview as? AdView else { return }
    guard let displayedAd = displayedAds[adView] else { return }
    hide(adView: adView)
    adView.setSwipeTranslation(0, animated: true)
    displayedAd.handler(displayedAd.ad, .opened)
  }
  
  @objc private func tappedDisliked(_ sender: AdSwipeButton) {
    guard let adView = sender.superview as? AdView else { return }
    guard let displayedAd = displayedAds[adView] else { return }
    hide(adView: adView)
    adView.setSwipeTranslation(0, animated: true)
    displayedAd.handler(displayedAd.ad, .disliked)
  }
  
  // Distance travelled after decelerating to zero velocity at a constant rate
  func project(initialVelocity: CGFloat, decelerationRate: CGFloat) -> CGFloat {
    return (initialVelocity / 1000.0) * decelerationRate / (1.0 - decelerationRate)
  }
  
  private var panState: CGPoint = .zero
  @objc private func dismissPannedAdView(_ pan: UIPanGestureRecognizer) {
    guard let adView = pan.view as? AdView else { return }
    switch pan.state {
    case .began:
      panState = adView.center
      // Make sure to stop the dismiss timer
      dismissTimers[adView]?.invalidate()
    case .changed:
      adView.transform.ty = min(0, pan.translation(in: adView).y)
    case .ended:
      let velocity = pan.velocity(in: adView).y
      let y = min(panState.y, panState.y + pan.translation(in: adView).y)
      let projected = project(initialVelocity: velocity, decelerationRate: UIScrollView.DecelerationRate.normal.rawValue)
      if y + projected < 0 {
        guard let displayedAd = self.displayedAds[adView] else { return }
        hide(adView: adView, velocity: velocity)
        displayedAd.handler(displayedAd.ad, .dismissed)
        break
      }
      fallthrough
    case .cancelled:
      // Re-setup timeout timer
      setupTimeoutTimer(for: adView)
      adView.layer.springAnimate(property: kPOPLayerTranslationY, key: "translation.y") { animation, _ in
        animation.toValue = 0
      }
    default:
      break
    }
  }
  
  private let actionTriggerThreshold: CGFloat = 180.0
  private let actionRestThreshold: CGFloat = 90.0
  
  private var swipeState: CGFloat = 0
  @objc private func swipePannedAdView(_ pan: UIPanGestureRecognizer) {
    guard let adView = pan.view as? AdView else { return }
    switch pan.state {
    case .began:
      swipeState = adView.adContentButton.transform.tx
      // Make sure to stop the dismiss timer
      dismissTimers[adView]?.invalidate()
    case .changed:
      let tx = swipeState + pan.translation(in: adView).x
//      if tx > actionTriggerThreshold && !adView.openSwipeButton.isHighlighted {
//        let impact = UIImpactFeedbackGenerator(style: .medium)
//        impact.prepare()
//        impact.impactOccurred()
//      }
//      adView.openSwipeButton.isHighlighted = tx > actionTriggerThreshold
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
        guard let displayedAd = displayedAds[adView] else { break }
        adView.setSwipeTranslation(0, animated: true, panVelocity: velocity)
        hide(adView: adView)
        displayedAd.handler(displayedAd.ad, tx > 0 ? .opened : .disliked)
        break
      } else if /*tx + projected > actionRestThreshold ||*/ tx + projected < -actionRestThreshold {
        adView.setSwipeTranslation((tx + projected) > 0 ? actionRestThreshold : -actionRestThreshold, animated: true, panVelocity: velocity)
        break
      }
      fallthrough
    case .cancelled:
      // Re-setup timeout timer
      setupTimeoutTimer(for: adView)
      adView.setSwipeTranslation(0, animated: true)
    default:
      break
    }
  }
  
  // MARK: - Animations
  
  private func animateIn(adView: AdView) {
    adView.layoutIfNeeded()
    adView.layer.transform = CATransform3DMakeTranslation(0, -adView.bounds.size.height, 0)
    
    adView.layer.springAnimate(property: kPOPLayerTranslationY, key: "translation.y") { animation, _ in
      animation.toValue = 0
    }
  }
  
  private func animateOut(adView: AdView, velocity: CGFloat? = nil, completion: @escaping () -> Void) {
    adView.layoutIfNeeded()
    let y = adView.frame.minY - view.safeAreaInsets.top - adView.transform.ty
    
    adView.layer.springAnimate(property: kPOPLayerTranslationY, key: "translation.y") { animation, _ in
      animation.toValue = -(view.safeAreaInsets.top + y + adView.bounds.size.height)
      if let velocity = velocity {
        animation.velocity = velocity
      }
      animation.completionBlock = { _, _ in
        completion()
      }
    }
  }
}

extension AdsViewController {
  class View: UIView {
    override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
      // Only allow tapping the ad part of this VC
      if let view = super.hitTest(point, with: event), view.superview is AdView {
        return view
      }
      return nil
    }
  }
}

extension AdsViewController: UIGestureRecognizerDelegate {
  
  public func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    if let pan = gestureRecognizer as? UIPanGestureRecognizer {
      let velocity = pan.velocity(in: pan.view)
      if pan.name == dismissGestureName {
        // Vertical only and only if the user isn't in a swipe transform
        if let adView = pan.view as? AdView, adView.swipeTranslation != 0 {
          return false
        }
        return abs(velocity.y) > abs(velocity.x)
      }
      if pan.name == swipeGestureName {
        // Horizontal only
        return abs(velocity.x) > abs(velocity.y)
      }
    }
    return false
  }
}

extension AdsViewController {
  
  /// Display a "My First Ad" on a presenting controller and be notified if they tap it
  public static func displayFirstAd(on presentingController: UIViewController, completion: @escaping (AdsNotificationHandler.Action, URL) -> Void) {
    let adsViewController = AdsViewController()
    
    guard let window = presentingController.view.window else {
      return
    }
    
    window.addSubview(adsViewController.view)
    adsViewController.view.snp.makeConstraints {
      $0.edges.equalTo(window.safeAreaLayoutGuide.snp.edges)
    }
    
    let notification = AdsNotification.customAd(
      title: Strings.myFirstAdTitle,
      body: Strings.myFirstAdBody,
      url: URL(string: "https://brave.com/my-first-ad")!
    )
    
    adsViewController.display(ad: notification, handler: { (notification, action) in
      completion(action, notification.targetURL)
    }, animatedOut: {
      adsViewController.view.removeFromSuperview()
    })
  }
}

extension AdsViewController {
  public override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    
    if #available(iOS 13.0, *) {
        if UITraitCollection.current.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
          visibleAdView?.adContentButton.applyTheme(for: traitCollection)
        }
    }
  }
}
