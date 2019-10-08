/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

public let PopoverArrowHeight: CGFloat = 8.0

public class PopoverNavigationController: UINavigationController {
  
  private class NavigationBar: UINavigationBar {
    override var frame: CGRect {
      get { return super.frame.with { $0.origin.y = PopoverArrowHeight } }
      set { super.frame = newValue }
    }
    override var barPosition: UIBarPosition {
      return .topAttached
    }
  }
  
  init() {
    super.init(navigationBarClass: NavigationBar.self, toolbarClass: nil)
    modalPresentationStyle = .currentContext
  }
  
  public override init(rootViewController: UIViewController) {
    super.init(navigationBarClass: NavigationBar.self, toolbarClass: nil)
    modalPresentationStyle = .currentContext
    viewControllers = [rootViewController]
  }
  
  @available(*, unavailable)
  public override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nil, bundle: nil)
    modalPresentationStyle = .currentContext
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  public override var additionalSafeAreaInsets: UIEdgeInsets {
    get { return UIEdgeInsets(top: PopoverArrowHeight, left: 0, bottom: 0, right: 0) }
    set { } // swiftlint:disable:this unused_setter_value
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    activateFullscreenDismiss()
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
