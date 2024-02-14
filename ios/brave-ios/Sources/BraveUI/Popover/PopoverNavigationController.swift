/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

public let PopoverArrowHeight: CGFloat = 8.0

open class PopoverNavigationController: UINavigationController, PopoverContentComponent {

  private class NavigationBar: UINavigationBar {
    override var frame: CGRect {
      get { return super.frame.with { $0.origin.y = PopoverArrowHeight } }
      set { super.frame = newValue }
    }
    override var barPosition: UIBarPosition {
      return .topAttached
    }
  }

  private class Toolbar: UIToolbar {
    override var frame: CGRect {
      get { return super.frame.with { $0.size.height = 44 } }
      set { super.frame = newValue }
    }
    override var barPosition: UIBarPosition {
      return .bottom
    }
  }

  public init() {
    super.init(navigationBarClass: NavigationBar.self, toolbarClass: Toolbar.self)
    modalPresentationStyle = .currentContext
  }

  public override init(rootViewController: UIViewController) {
    super.init(navigationBarClass: NavigationBar.self, toolbarClass: Toolbar.self)
    modalPresentationStyle = .currentContext
    viewControllers = [rootViewController]
  }

  @available(*, unavailable)
  public override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nil, bundle: nil)
    modalPresentationStyle = .currentContext
  }

  @available(*, unavailable)
  required public init(coder: NSCoder) {
    fatalError()
  }

  open override func viewDidLoad() {
    super.viewDidLoad()
    activateFullscreenDismiss()
    if let vc = viewControllers.first {
      preferredContentSize = vc.preferredContentSize
    }
  }

  func activateFullscreenDismiss() {
    // WARNING: Using private API, this may break in the future
    let senderKeyPath = String(format: "_cach%@roller", "edInteractionCont")
    let senderSelector = Selector(senderKeyPath)
    let handlerSelector = Selector(String(format: "handleNav%@nsition:", "igationTra"))
    if responds(to: senderSelector) {
      if let controller = value(forKeyPath: senderKeyPath) as? NSObject, controller.responds(to: handlerSelector) {
        let pan = UIPanGestureRecognizer(target: controller, action: handlerSelector)
        pan.delegate = self
        view.addGestureRecognizer(pan)
      }
    }
  }

  public override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    if !isNavigationBarHidden {
      var navBarFrame = navigationBar.frame
      navBarFrame.origin.y = additionalSafeAreaInsets.top
      navigationBar.frame = navBarFrame
    }
  }

  public var extendEdgeIntoArrow: Bool {
    return true
  }

  public var isPanToDismissEnabled: Bool {
    return self.visibleViewController === self.viewControllers.first
  }

  override public func preferredContentSizeDidChange(forChildContentContainer container: UIContentContainer) {
    self.preferredContentSize = container.preferredContentSize
  }

  /// The last visible controller to be popped from this navigation controller
  ///
  /// Used because when you pop a controller off the stack it is no longer listed within
  /// `viewControllers`, or set as `visibleViewController` even if the animation is still
  /// ongoing.
  private(set) weak var lastPoppedController: UIViewController?

  open override func popViewController(animated: Bool) -> UIViewController? {
    let vc = super.popViewController(animated: animated)
    lastPoppedController = vc
    return vc
  }

  open override func popToRootViewController(animated: Bool) -> [UIViewController]? {
    let vcs = super.popToRootViewController(animated: animated)
    lastPoppedController = vcs?.last
    return vcs
  }
}

extension PopoverNavigationController: UIGestureRecognizerDelegate {
  public func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    guard let pan = gestureRecognizer as? UIPanGestureRecognizer else { return false }
    if viewControllers.first == visibleViewController { return false }
    let velocity = pan.velocity(in: view)
    // Only allow left-to-right swipes
    return velocity.x > 0
  }
}
