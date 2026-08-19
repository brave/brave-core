// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit
import Web

extension TabDataValues {
  private struct KeyboardGuideTabHelperKey: TabDataKey {
    static var defaultValue: KeyboardLayoutTabHelper?
  }
  var keyboardLayoutHelper: KeyboardLayoutTabHelper? {
    get { self[KeyboardGuideTabHelperKey.self] }
    set { self[KeyboardGuideTabHelperKey.self] = newValue }
  }
}

/// A helper which exposes a special layout guide that can be used to constrain items that overlay
/// the tabs web view to the keyboard when the web view itself is the reason the keyboard is visible
///
/// ## Why not use the keyboard layout guide?
///
/// On iOS 26 and up, `WKWebView` breaks the standard `keyboardLayoutGuide` and reported keyboard
/// heights whenever the user focuses on an input element that brings up either software or hardware
/// keyboard, or uses the `UIFindInteraction`. The result after dismissing the keyboard leaves the
/// keyboard layout guide either with additional height even there is no active keyboard anymore,
/// incorrectly sizes in general or doesn't compute the correct height when switching between
/// software/hardware keyboards. As of iOS 27 beta 4 this is only partially fixed.
///
/// ## The solution
///
/// Avoid the standard keyboard layout guide, create our own layout guide and update its constraints
/// with the keyboard location/size only when the web view is the reason for keyboard appearance.
///
/// There is currently no way to force refresh the keyboard layout guide as its handling is fully
/// controlled by the OS. So instead we'll create our own keyboard layout guide specific to the tab.
///
/// This isn't a full fledged tracking layout guide so it can only be used to place things above the
/// keyboard but thats currently our only usage so its okay.
///
/// ## Caveats
///
/// This layout guide relies on activating the constraints only when the keyboard is actually
/// associated with the web view, which we check using `WebViewProxy.isKeyboardVisible`. This is a
/// Chromium function which does a first responder `inputAccessoryView` null check and iPads do not
/// show an `inputAccessoryView` so this will always be false.
///
/// This should be okay as bottom bar is not currently supported on iPad, however if this
/// requirement changes we may have to find alternative means (or hope that keyboardLayoutGuide is
/// fixed by that point.)
class KeyboardLayoutTabHelper: TabObserver {
  let webViewKeyboardLayoutGuide: UILayoutGuide = .init()

  /// When the keyboard is offscreen, the layout guide is tied to the bottomAnchor of the view's
  /// safeAreaLayoutGuide.
  var usesBottomSafeArea: Bool = true {
    didSet {
      NSLayoutConstraint.deactivate(inactiveKeyboardConstraints)
      inactiveKeyboardConstraints = makeInactiveKeyboardConstraints()
      if !isWebViewKeyboardActive {
        NSLayoutConstraint.activate(inactiveKeyboardConstraints)
      }
    }
  }

  /// A closure executed inside of a `UIViewPropertyAnimator` so you may animate any changes along
  /// side the `webViewKeyboardLayoutGuide` updating.
  var keyboardActiveStateDidChange: ((_ isKeyboardActive: Bool) -> Void)?

  init(tab: some TabState) {
    self.tab = tab
    self.webViewKeyboardLayoutGuide.identifier = "WebViewKeyboardLayoutGuide"

    tab.view.addLayoutGuide(webViewKeyboardLayoutGuide)
    tab.addObserver(self)

    inactiveKeyboardConstraints = makeInactiveKeyboardConstraints()
    NSLayoutConstraint.activate(inactiveKeyboardConstraints)

    activeKeyboardConstraints = .init(
      leading: webViewKeyboardLayoutGuide.leadingAnchor.constraint(
        equalTo: tab.view.leadingAnchor,
        constant: 0
      ),
      top: webViewKeyboardLayoutGuide.topAnchor.constraint(
        equalTo: tab.view.topAnchor,
        constant: 0
      ),
      width: webViewKeyboardLayoutGuide.widthAnchor.constraint(equalToConstant: 0),
      height: webViewKeyboardLayoutGuide.heightAnchor.constraint(equalToConstant: 0)
    )
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
    tab?.removeObserver(self)
  }

  private struct VisibleConstraints {
    var leading: NSLayoutConstraint
    var top: NSLayoutConstraint
    var width: NSLayoutConstraint
    var height: NSLayoutConstraint

    var constraints: [NSLayoutConstraint] {
      [leading, top, width, height]
    }
  }

  private weak var tab: (any TabState)?
  private var isWebViewKeyboardActive: Bool = false {
    didSet {
      if oldValue == isWebViewKeyboardActive { return }
      if isWebViewKeyboardActive {
        NSLayoutConstraint.deactivate(inactiveKeyboardConstraints)
        if let constraints = activeKeyboardConstraints?.constraints {
          NSLayoutConstraint.activate(constraints)
        }
      } else {
        if let constraints = activeKeyboardConstraints?.constraints {
          NSLayoutConstraint.deactivate(constraints)
        }
        NSLayoutConstraint.activate(inactiveKeyboardConstraints)
      }
    }
  }
  private var activeKeyboardConstraints: VisibleConstraints?
  private var inactiveKeyboardConstraints: [NSLayoutConstraint] = []

  private func makeInactiveKeyboardConstraints() -> [NSLayoutConstraint] {
    guard let tab else { return [] }
    return [
      webViewKeyboardLayoutGuide.leadingAnchor.constraint(equalTo: tab.view.leadingAnchor),
      webViewKeyboardLayoutGuide.trailingAnchor.constraint(equalTo: tab.view.trailingAnchor),
      webViewKeyboardLayoutGuide.topAnchor.constraint(
        equalTo: usesBottomSafeArea
          ? tab.view.safeAreaLayoutGuide.bottomAnchor : tab.view.bottomAnchor
      ),
      webViewKeyboardLayoutGuide.bottomAnchor.constraint(
        equalTo: usesBottomSafeArea
          ? tab.view.safeAreaLayoutGuide.bottomAnchor : tab.view.bottomAnchor
      ),
    ]
  }

  private func updateActiveConstraintsForKeyboard(notification: Notification) {
    guard let tab else { return }
    let keyboardFrame =
      (notification.userInfo?[UIResponder.keyboardFrameEndUserInfoKey] as? CGRect) ?? .zero
    let frame = tab.view.convert(keyboardFrame, from: nil)
    activeKeyboardConstraints?.leading.constant = frame.minX
    activeKeyboardConstraints?.top.constant = frame.minY
    activeKeyboardConstraints?.width.constant = frame.width
    activeKeyboardConstraints?.height.constant = frame.height
  }

  private func animateKeyboardChanges(notification: Notification) {
    let animationDuration =
      (notification.userInfo?[UIResponder.keyboardAnimationDurationUserInfoKey] as? Double) ?? 0.2
    var curve = UIView.AnimationCurve.easeIn
    if let curveValue = notification.userInfo?[UIResponder.keyboardAnimationCurveUserInfoKey]
      as? Int
    {
      NSNumber(value: curveValue as Int).getValue(&curve)
    }
    let animator = UIViewPropertyAnimator(duration: animationDuration, curve: curve)
    animator.addAnimations { [self] in
      keyboardActiveStateDidChange?(isWebViewKeyboardActive)
    }
    animator.startAnimation()
  }

  private func isWebViewInputActive(notification: Notification) -> Bool {
    guard let tab else { return false }
    let isLocal = notification.userInfo?[UIResponder.keyboardIsLocalUserInfoKey] as? Bool == true
    return isLocal && (tab.webViewProxy?.isKeyboardVisible == true || tab.isFindNavigatorVisible)
  }

  // MARK: - Keyboard Notifications

  @objc private func keyboardWillShow(_ notification: Notification) {
    updateActiveConstraintsForKeyboard(notification: notification)
    isWebViewKeyboardActive = isWebViewInputActive(notification: notification)
    animateKeyboardChanges(notification: notification)
  }

  @objc private func keyboardWillHide(_ notification: Notification) {
    guard let tab else { return }
    let handleNotification = { [weak self] in
      guard let self else { return }
      updateActiveConstraintsForKeyboard(notification: notification)
      isWebViewKeyboardActive = isWebViewInputActive(notification: notification)
      animateKeyboardChanges(notification: notification)
    }
    if tab.isFindNavigatorVisible == true {
      // Defer one run loop: isFindNavigatorVisible is unreliable synchronously
      guard isWebViewKeyboardActive else { return }
      DispatchQueue.main.async {
        handleNotification()
      }
      return
    }
    handleNotification()
  }

  // MARK: - TabObserver

  func tabWasShown(_ tab: some TabState) {
    NotificationCenter.default.removeObserver(self)
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(keyboardWillShow(_:)),
      name: UIResponder.keyboardWillShowNotification,
      object: nil
    )
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(keyboardWillHide(_:)),
      name: UIResponder.keyboardWillHideNotification,
      object: nil
    )
  }

  func tabWasHidden(_ tab: some TabState) {
    NotificationCenter.default.removeObserver(self)
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    NotificationCenter.default.removeObserver(self)
    tab.removeObserver(self)
  }
}
