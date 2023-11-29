// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import PanModal
import Shared
import BraveShared
import BraveUI
import SwiftUI
import Growth

struct MenuItemHeaderView: View {
  @Environment(\.colorScheme) private var colorScheme: ColorScheme
  @ScaledMetric private var iconSize: CGFloat = 32.0
  var icon: Image
  var title: String
  var subtitle: String?

  var body: some View {
    HStack(spacing: 14) {
      icon
        .font(.body)
        .frame(width: iconSize, height: iconSize)
        .foregroundColor(Color(.braveLabel))
        .background(
          RoundedRectangle(cornerRadius: 8, style: .continuous)
            .fill(Color(.secondaryBraveGroupedBackground))
            .shadow(color: Color.black.opacity(0.1), radius: 1, x: 0, y: 1)
        )
        .padding(.vertical, 2)
      VStack(alignment: .leading, spacing: 3) {
        Text(verbatim: title)
        if let subTitle = subtitle {
          Text(subTitle)
            .font(.subheadline)
            .foregroundColor(Color(.secondaryBraveLabel))
        }
      }
      .padding(.vertical, subtitle != nil ? 5 : 0)
    }
    .foregroundColor(Color(.braveLabel))
  }
}

private struct MenuView<Content: View>: View {
  var content: Content
  var body: some View {
    ScrollView(.vertical) {
      content
        .padding(.vertical, 8)
        .frame(maxWidth: .infinity)
        .accentColor(Color(.braveBlurpleTint))
    }
  }
}

struct MenuItemButton: View {
  @Environment(\.colorScheme) var colorScheme: ColorScheme

  var icon: Image
  var title: String
  var subtitle: String?
  var action: () -> Void

  var body: some View {
    Button(action: action) {
      MenuItemHeaderView(icon: icon, title: title, subtitle: subtitle)
        .padding(.horizontal, 14)
        .frame(maxWidth: .infinity, minHeight: 44.0, alignment: .leading)
    }
    .buttonStyle(TableCellButtonStyle())
  }
}

class MenuViewController: UINavigationController, UIPopoverPresentationControllerDelegate {

  private var menuNavigationDelegate: MenuNavigationControllerDelegate?
  private let initialHeight: CGFloat

  init<MenuContent: View>(initialHeight: CGFloat, @ViewBuilder content: (MenuViewController) -> MenuContent) {
    self.initialHeight = initialHeight
    super.init(nibName: nil, bundle: nil)
    viewControllers = [MenuHostingController(content: content(self))]
    menuNavigationDelegate = MenuNavigationControllerDelegate(panModal: self)
    delegate = menuNavigationDelegate
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private var previousPreferredContentSize: CGSize?
  func presentInnerMenu(
    _ viewController: UIViewController,
    expandToLongForm: Bool = true
  ) {
    let container = InnerMenuNavigationController(rootViewController: viewController)
    container.delegate = menuNavigationDelegate
    container.modalPresentationStyle = .overCurrentContext  // over to fix the dismiss animation
    container.innerMenuDismissed = { [weak self] in
      guard let self = self else { return }
      if !self.isDismissing {
        self.panModalSetNeedsLayoutUpdate()
      }
      // Restore original content size
      if let contentSize = self.previousPreferredContentSize {
        self.preferredContentSize = contentSize
      }
    }
    // Save current content size to be restored when inner menu is dismissed
    if preferredContentSize.height < 580 {
      previousPreferredContentSize = preferredContentSize
      preferredContentSize = CGSize(width: 375, height: 580)
    }
    present(container, animated: true) {
      self.panModalSetNeedsLayoutUpdate()
    }
    if expandToLongForm {
      // Delay a fraction of a second to make the animation look more fluid
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.15) {
        self.panModalTransition(to: .longForm)
      }
    }
  }

  func pushInnerMenu(
    _ viewController: UIViewController,
    expandToLongForm: Bool = true
  ) {
    super.pushViewController(viewController, animated: true)
    if expandToLongForm {
      panModalTransition(to: .longForm)
    }
  }

  @available(*, unavailable, message: "Use 'pushInnerMenu(_:expandToLongForm:)' instead")
  override func pushViewController(_ viewController: UIViewController, animated: Bool) {
    super.pushViewController(viewController, animated: animated)
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    navigationBar.isTranslucent = false
    recordMenuOpenedP3A()
  }
  
  private func recordMenuOpenedP3A() {
    var storage = P3ATimedStorage<Int>.menuPresentedStorage
    storage.add(value: 1, to: Date())
    UmaHistogramRecordValueToBucket(
      "Brave.Toolbar.MenuOpens",
      buckets: [
        0,
        .r(1...5),
        .r(6...15),
        .r(16...29),
        .r(30...49),
        .r(50...),
      ],
      value: storage.combinedValue
    )
  }

  override func viewSafeAreaInsetsDidChange() {
    super.viewSafeAreaInsetsDidChange()

    // Bug with pan modal + hidden nav bar causes safe area insets to zero out
    if view.safeAreaInsets == .zero, isPanModalPresented,
      var insets = view.window?.safeAreaInsets {
      // When that happens we re-set them via additionalSafeAreaInsets to the windows safe
      // area insets. Since the pan modal appears over the entire screen we can safely use
      // the windows safe area. Top will stay 0 since we are using non-translucent nav bar
      // and the top never reachs the safe area (handled by pan modal)
      insets.top = 0
      additionalSafeAreaInsets = insets
    }
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    .lightContent
  }

  override var shouldAutorotate: Bool {
    // Due to a bug in PanModal, the presenting controller does not receive safe area updates
    // while a pan modal is presented, therefore for the time being, do not allow rotation
    // while this menu is open.
    //
    // Issue: https://github.com/slackhq/PanModal/issues/139
    false
  }

  private var isDismissing = false

  override func dismiss(animated flag: Bool, completion: (() -> Void)? = nil) {
    if let _ = presentedViewController as? InnerMenuNavigationController,
      presentingViewController?.presentedViewController === self {
      isDismissing = true
      presentingViewController?.dismiss(animated: flag, completion: completion)
    } else {
      super.dismiss(animated: flag, completion: completion)
    }
  }

  private var isPresentingInnerMenu: Bool {
    presentedViewController is InnerMenuNavigationController
  }

  override func accessibilityPerformEscape() -> Bool {
    dismiss(animated: true)
    return true
  }
}

// MARK: PanModalPresentable

extension MenuViewController: PanModalPresentable {
  var panScrollable: UIScrollView? {
    // For SwiftUI:
    //  - in iOS 13, ScrollView will exist within a host view
    //  - in iOS 14, it will be a direct subview
    // For UIKit:
    //  - UITableViewController's view is a UITableView, thus the view itself is a UIScrollView
    //  - For our non-UITVC's, the scroll view is a usually a subview of the main view
    func _scrollViewChild(in parentView: UIView, depth: Int = 0) -> UIScrollView? {
      if depth > 2 { return nil }
      if let scrollView = parentView as? UIScrollView {
        return scrollView
      }
      for view in parentView.subviews {
        if let scrollView = view as? UIScrollView {
          return scrollView
        }
        if !view.subviews.isEmpty, let childScrollView = _scrollViewChild(in: view, depth: depth + 1) {
          return childScrollView
        }
      }
      return nil
    }
    if let vc = presentedViewController, !vc.isBeingPresented {
      if let nc = vc as? UINavigationController, let vc = nc.topViewController {
        let scrollView = _scrollViewChild(in: vc.view)
        return scrollView
      }
      let scrollView = _scrollViewChild(in: vc.view)
      return scrollView
    }
    guard let topVC = topViewController else { return nil }
    topVC.view.layoutIfNeeded()
    return _scrollViewChild(in: topVC.view)
  }
  var topOffset: CGFloat {
    let topInset = view.window?.safeAreaInsets.top ?? 0
    return topInset + 32
  }
  var longFormHeight: PanModalHeight {
    .maxHeight
  }
  var shortFormHeight: PanModalHeight {
    isPresentingInnerMenu ? .maxHeight : .contentHeight(initialHeight)
  }

  func shouldRespond(to panModalGestureRecognizer: UIPanGestureRecognizer) -> Bool {
    // This enables reordering elements without colliding with PanModal gestures, see bug #3787.
    if let tableView = panScrollable as? UITableView, tableView.isEditing {
      return false
    }
    return true
  }

  var allowsExtendedPanScrolling: Bool {
    true
  }
  var cornerRadius: CGFloat {
    10.0
  }
  var anchorModalToLongForm: Bool {
    isPresentingInnerMenu
  }
  var panModalBackgroundColor: UIColor {
    UIColor(white: 0.0, alpha: 0.5)
  }

  var dragIndicatorBackgroundColor: UIColor {
    UIColor(white: 0.95, alpha: 1.0)
  }
  var transitionDuration: Double {
    0.35
  }
  var springDamping: CGFloat {
    0.85
  }
}

private class MenuHostingController<MenuContent: View>: UIHostingController<MenuView<MenuContent>> {
  init(content: MenuContent) {
    super.init(rootView: MenuView(content: content))
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    .lightContent
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    let animateNavBar = (navigationController?.isBeingPresented == false ? animated : false)
    navigationController?.setNavigationBarHidden(true, animated: animateNavBar)
    self.navigationController?.preferredContentSize = {
      let controller = UIHostingController(rootView: self.rootView.content)
      let size = controller.view.sizeThatFits(CGSize(width: 375, height: 0))
      let navBarHeight = navigationController?.navigationBar.bounds.height ?? 0
      let preferredPopoverWidth: CGFloat = 375.0
      let minimumPopoverHeight: CGFloat = 240.0
      let maximumPopoverHeight: CGFloat = 580.0
      return CGSize(
        width: preferredPopoverWidth,
        // Have to increase the content size by the hidden nav bar height so that the size
        // doesn't change when the user navigates within the menu where the nav bar is
        // visible.
        height: min(max(size.height + 16, minimumPopoverHeight), maximumPopoverHeight + navBarHeight)
      )
    }()
    view.backgroundColor = .braveGroupedBackground
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    if navigationController?.isBeingDismissed == false {
      navigationController?.setNavigationBarHidden(false, animated: animated)
      navigationController?.preferredContentSize = CGSize(width: 375, height: 580)
    }
  }
}

// MARK: MenuNavigationControllerDelegate

private class MenuNavigationControllerDelegate: NSObject, UINavigationControllerDelegate {
  weak var panModal: (UIViewController & PanModalPresentable)?
  init(panModal: UIViewController & PanModalPresentable) {
    self.panModal = panModal
    super.init()
  }
  func navigationController(
    _ navigationController: UINavigationController,
    didShow viewController: UIViewController,
    animated: Bool
  ) {
    panModal?.panModalSetNeedsLayoutUpdate()
  }
  
  public func navigationControllerSupportedInterfaceOrientations(_ navigationController: UINavigationController) -> UIInterfaceOrientationMask {
    return navigationController.visibleViewController?.supportedInterfaceOrientations ?? navigationController.supportedInterfaceOrientations
  }

  public func navigationControllerPreferredInterfaceOrientationForPresentation(_ navigationController: UINavigationController) -> UIInterfaceOrientation {
    return navigationController.visibleViewController?.preferredInterfaceOrientationForPresentation ?? navigationController.preferredInterfaceOrientationForPresentation
  }
}

private class InnerMenuNavigationController: UINavigationController {
  var innerMenuDismissed: (() -> Void)?

  override func viewDidLoad() {
    super.viewDidLoad()

    // Needed or else pan modal top scroll insets are messed up for some reason
    navigationBar.isTranslucent = false
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    innerMenuDismissed?()
  }
}

class ColorAwareNavigationController: UINavigationController {
  var statusBarStyle: UIStatusBarStyle = .default {
    didSet {
      setNeedsStatusBarAppearanceUpdate()
    }
  }
  override var preferredStatusBarStyle: UIStatusBarStyle {
    return statusBarStyle
  }
}

extension P3ATimedStorage where Value == Int {
  fileprivate static var menuPresentedStorage: Self { .init(name: "menu-presented", lifetimeInDays: 7) }
}
