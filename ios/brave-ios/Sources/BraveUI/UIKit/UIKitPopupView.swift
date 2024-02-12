/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

public enum PopupViewDismissType: Int {
  case dont
  case deny
  case noAnimation
  case normal
  case flyUp
  case flyDown
  case scaleDown
}

public enum PopupViewShowType: Int {
  case normal
  case flyUp
  case flyDown
}

public enum PopupViewAlignment: Int {
  case top
  case middle
  case bottom
}

public enum PopupViewStyle: Int {
  case dialog
  case sheet
}

public protocol PopupViewDelegate {
  func popupViewDidShow(_ popupView: UIKitPopupView)
  func popupViewShouldDismiss(_ popupView: UIKitPopupView) -> Bool
  func popupViewDidDismiss(_ popupView: UIKitPopupView)
}

open class UIKitPopupView: UIView, UIGestureRecognizerDelegate {
  class ButtonData: NSObject {
    var title: String = ""
    var button: UIButton?
    var handler: (() -> PopupViewDismissType)?
    var type: ButtonType = .secondary
    var font: UIFont = UIFont.systemFont(ofSize: 17, weight: UIFont.Weight.semibold)
  }

  public enum ButtonType {
    case primary
    case secondary
    case destructive
    case link

    var backgroundColor: UIColor {
      switch self {
      case .primary:
        return .braveBlurpleTint
      case .secondary:
        return .secondaryButtonTint
      case .destructive:
        return .braveErrorLabel
      case .link:
        return .clear
      }
    }

    var titleColor: UIColor {
      if self == .link {
        return .braveBlurpleTint
      }
      return .white
    }
  }

  static let popupView = UIKitPopupView()

  fileprivate var keyboardState: KeyboardState?

  let kPopupDialogPadding: CGFloat = 20.0
  let kPopupDialogButtonHeight: CGFloat = 50.0
  let kPopupDialogMaxWidth: CGFloat = 390.0

  fileprivate let kPopupBackgroundAlpha: CGFloat = 0.4
  fileprivate let kPopupBackgroundDismissTouchDuration: Double = 0.005
  fileprivate let kPopupDialogShakeAngle: CGFloat = 0.2
  fileprivate let kPopupDialogCornerRadius: CGFloat = 12.0
  fileprivate let kPopupDialogButtonRadius: CGFloat = 0.0
  fileprivate let kPopupDialogButtonPadding: CGFloat = 16.0
  fileprivate let kPopupDialogButtonSpacing: CGFloat = 16.0
  fileprivate let kPopupDialogButtonTextSize: CGFloat = 17.0

  var showHandler: (() -> Void)?
  var dismissHandler: (() -> Void)?

  var dialogWidth: CGFloat {
    get {
      if let superview = self.superview {
        return min(superview.bounds.width - padding * 2.0, kPopupDialogMaxWidth)
      }

      return 0.0
    }
  }

  var dialogView: UIView!
  var overlayView: UIView!
  var contentView: UIView!

  var style: PopupViewStyle!
  var verticalAlignment: PopupViewAlignment = .middle
  var defaultShowType: PopupViewShowType = .normal
  var defaultDismissType: PopupViewDismissType = .normal
  var overlayDismisses: Bool = true
  var overlayDismissHandler: (() -> Bool)?
  var presentsOverWindow: Bool = true
  var automaticallyMovesWithKeyboard: Bool = true
  var padding: CGFloat = 20.0 {
    didSet {
      setNeedsLayout()
    }
  }

  var dialogButtons: Array<ButtonData> = []
  var dialogButtonsContainer: UIView!

  var delegate: PopupViewDelegate?

  public override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .clear
    autoresizingMask = [.flexibleWidth, .flexibleHeight]

    let touchRecognizer: UILongPressGestureRecognizer = UILongPressGestureRecognizer(target: self, action: #selector(backgroundTapped(recognizer:)))
    touchRecognizer.minimumPressDuration = kPopupBackgroundDismissTouchDuration
    touchRecognizer.delegate = self

    overlayView = UIView(frame: bounds)
    overlayView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    overlayView.backgroundColor = .black
    overlayView.alpha = kPopupBackgroundAlpha
    overlayView.addGestureRecognizer(touchRecognizer)
    addSubview(overlayView)

    dialogButtonsContainer = UIView(frame: CGRect.zero)
    dialogButtonsContainer.autoresizingMask = [.flexibleWidth, .flexibleHeight]

    dialogView = UIView(frame: CGRect.zero)
    dialogView.clipsToBounds = true
    dialogView.autoresizingMask = [.flexibleWidth, .flexibleTopMargin, .flexibleBottomMargin]
    dialogView.backgroundColor = .white

    setStyle(popupStyle: .dialog)

    KeyboardHelper.defaultHelper.addDelegate(self)
  }

  public required init(coder: NSCoder) {
    super.init(coder: coder)!
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  // MARK: Layout

  private var applicationWindow: UIWindow? {
    return UIApplication.shared.delegate?.window as? UIWindow
  }

  public override func layoutSubviews() {
    let contentSize: CGSize = contentView.frame.size
    let keyboardHeight = keyboardState?.intersectionHeightForView(applicationWindow ?? self) ?? 0

    dialogView.frame = _dialogFrameWithKeyboardHeight(height: keyboardHeight)
    contentView.frame = CGRect(x: 0.0, y: 0.0, width: dialogView.bounds.size.width, height: contentSize.height)

    // Add and create buttons as necessary.
    if !dialogButtons.isEmpty {
      var buttonWidth: CGFloat = 0.0

      dialogButtonsContainer.frame = CGRect(x: 0.0, y: contentSize.height, width: dialogView.frame.size.width, height: kPopupDialogButtonHeight)

      buttonWidth = dialogView.frame.width - (kPopupDialogButtonPadding * 2.0)
      buttonWidth -= CGFloat((dialogButtons.count - 1)) * kPopupDialogButtonSpacing
      buttonWidth = buttonWidth / CGFloat(dialogButtons.count)
      buttonWidth = rint(buttonWidth)

      var buttonFrame: CGRect = CGRect(x: kPopupDialogButtonPadding, y: 0, width: buttonWidth, height: kPopupDialogButtonHeight)

      for buttonData in dialogButtons {
        var button = buttonData.button
        if button == nil {
          button = UIButton(type: .system)
          button!.titleLabel!.font = buttonData.font
          button!.titleLabel!.adjustsFontSizeToFitWidth = true
          button!.layer.cornerRadius = buttonFrame.height / 2.0  // kPopupDialogButtonRadius
          button!.layer.cornerCurve = .continuous
          button!.backgroundColor = buttonData.type.backgroundColor
          button!.setTitle(buttonData.title, for: .normal)
          button!.setTitleColor(buttonData.type.titleColor, for: .normal)
          button!.addTarget(self, action: #selector(dialogButtonTapped(button:)), for: .touchUpInside)
          buttonData.button = button
          dialogButtonsContainer.addSubview(button!)
        }

        button?.frame = buttonFrame
        buttonFrame.origin.x += buttonFrame.size.width + kPopupDialogButtonSpacing
      }

      dialogView.addSubview(dialogButtonsContainer)
    }
  }

  @discardableResult fileprivate func _dialogFrameWithKeyboardHeight(height: CGFloat) -> CGRect {
    var visibleFrame: CGRect = bounds
    var dialogFrame: CGRect = CGRect.zero
    let contentSize: CGSize = contentView.frame.size
    var dialogSize: CGSize = CGSize(width: dialogWidth, height: contentSize.height)

    if !dialogButtons.isEmpty {
      dialogSize.height += kPopupDialogButtonHeight
      dialogSize.height += kPopupDialogButtonPadding
    }

    if automaticallyMovesWithKeyboard && height > 0 {
      visibleFrame = CGRect(x: 0.0, y: 0.0, width: bounds.size.width, height: UIScreen.main.bounds.height - height)
    }

    if !dialogView.transform.isIdentity {
      dialogSize = dialogSize.applying(dialogView.transform)
    }

    dialogFrame = CGRect(x: rint(visibleFrame.midX - (dialogSize.width / 2.0)), y: rint(visibleFrame.midY - (dialogSize.height / 2.0)), width: dialogSize.width, height: dialogSize.height)
    dialogFrame.origin.y = max(dialogFrame.origin.y, padding)

    if verticalAlignment == .top {
      dialogFrame.origin.y = visibleFrame.origin.y
    } else if verticalAlignment == .bottom {
      dialogFrame.origin.y = visibleFrame.size.height - dialogFrame.size.height
    }

    return dialogFrame
  }

  // MARK: Presentation

  private var autoDismissTimer: Timer?

  public func show() {
    showWithType(showType: defaultShowType)
  }

  public func showWithType(showType: PopupViewShowType, autoDismissTime: TimeInterval? = nil) {
    if superview != nil { return }

    if let autoDismissTime = autoDismissTime {
      autoDismissTimer?.invalidate()
      autoDismissTimer = nil
      autoDismissTimer =
        Timer.scheduledTimer(
          withTimeInterval: autoDismissTime, repeats: false,
          block: { [weak self] _ in
            self?.dismiss()
          })
    }

    dialogView.removeFromSuperview()

    if presentsOverWindow {
      UIApplication.shared.keyWindow?.addSubview(self)
    } else {
      let currentViewController: AnyObject = (applicationWindow?.rootViewController)!
      if currentViewController is UINavigationController {
        let navigationController: UINavigationController? = currentViewController as? UINavigationController
        navigationController?.visibleViewController?.view.addSubview(self)
      } else if currentViewController is UIViewController {
        let viewController: UIViewController? = currentViewController as? UIViewController
        viewController?.view.addSubview(self)
      }
    }

    frame = superview?.bounds ?? UIScreen.main.bounds

    addSubview(dialogView)
    setNeedsLayout()
    layoutIfNeeded()

    if showType == .flyUp {
      let finalFrame: CGRect = dialogView.frame
      var startFrame: CGRect = dialogView.frame
      startFrame.origin.y = frame.maxY
      dialogView.frame = startFrame

      // For subclasses.
      willShowWithType(showType: showType)

      UIView.animate(
        withDuration: 0.4, delay: 0, usingSpringWithDamping: 0.85, initialSpringVelocity: 0, options: [],
        animations: {
          self.dialogView.frame = finalFrame
        }, completion: nil)
    } else if showType == .flyDown {
      let finalFrame: CGRect = dialogView.frame
      var startFrame: CGRect = dialogView.frame
      startFrame.origin.y = -dialogView.frame.height
      dialogView.frame = startFrame

      willShowWithType(showType: showType)

      UIView.animate(
        withDuration: 0.4, delay: 0, usingSpringWithDamping: 0.85, initialSpringVelocity: 0, options: [],
        animations: {
          self.dialogView.frame = finalFrame
        }, completion: nil)
    }

    overlayView.alpha = 0.0

    UIView.animate(
      withDuration: 0.2, delay: 0.0, options: [.curveEaseOut],
      animations: {
        self.overlayView.alpha = self.kPopupBackgroundAlpha
      }, completion: nil)

    didShowWithType(showType: showType)

    showHandler?()

    delegate?.popupViewDidShow(self)
    setNeedsLayout()
  }

  public func dismiss() {
    dismissWithType(dismissType: defaultDismissType)
  }

  public func dismissWithType(dismissType: PopupViewDismissType) {
    if dismissType == .dont {
      return
    }

    if delegate?.popupViewShouldDismiss(self) == false {
      return
    }

    if dismissType != .deny {
      self.endEditing(true)
    }

    switch dismissType {
    case .dont:
      break
    case .deny:
      let animation: CAKeyframeAnimation = CAKeyframeAnimation(keyPath: "transform")
      animation.duration = 0.06
      animation.repeatCount = 2
      animation.autoreverses = true
      animation.isRemovedOnCompletion = true
      animation.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
      animation.values = [NSValue(caTransform3D: CATransform3DMakeRotation(kPopupDialogShakeAngle, 0.0, 0.0, 1.0)), NSValue(caTransform3D: CATransform3DMakeRotation(-kPopupDialogShakeAngle, 0.0, 0.0, 1.0))]
      dialogView.layer.add(animation, forKey: "dialog.shake")
    case .noAnimation:
      willDismissWithType(dismissType: dismissType)
      removeFromSuperview()
      didDismissWithType(dismissType: dismissType)
      delegate?.popupViewDidDismiss(self)
      dismissHandler?()
    case .normal:
      willDismissWithType(dismissType: dismissType)

      UIView.animate(
        withDuration: 0.2, delay: 0.0, options: [.beginFromCurrentState, .curveEaseIn],
        animations: {
          self.alpha = 0.0
        },
        completion: { (finished) in
          self.removeFromSuperview()
          self.alpha = 1.0
          self.didDismissWithType(dismissType: dismissType)
          self.delegate?.popupViewDidDismiss(self)
          self.dismissHandler?()
        })
    case .scaleDown:
      willDismissWithType(dismissType: dismissType)

      UIView.animate(
        withDuration: 0.2, delay: 0.0, options: [.beginFromCurrentState, .curveEaseIn],
        animations: {
          self.alpha = 0.0
          self.dialogView.transform = self.dialogView.transform.scaledBy(x: 0.9, y: 0.9)
        },
        completion: { (finished) in
          self.removeFromSuperview()
          self.alpha = 1.0
          self.dialogView.transform = CGAffineTransform.identity
          self.didDismissWithType(dismissType: dismissType)
          self.delegate?.popupViewDidDismiss(self)
          self.dismissHandler?()
        })
    case .flyUp:
      willDismissWithType(dismissType: dismissType)

      overlayView.alpha = kPopupBackgroundAlpha
      UIView.animate(
        withDuration: 0.2, delay: 0.0, options: [.beginFromCurrentState, .curveEaseIn],
        animations: {
          var flyawayFrame: CGRect = self.dialogView.frame
          flyawayFrame.origin.y = -flyawayFrame.height
          flyawayFrame.origin.y -= 20.0
          self.dialogView.frame = flyawayFrame
          self.overlayView.alpha = 0.0
        },
        completion: { (finished) in
          self.removeFromSuperview()
          self.overlayView.alpha = self.kPopupBackgroundAlpha
          self.didDismissWithType(dismissType: dismissType)
          self.delegate?.popupViewDidDismiss(self)
          self.dismissHandler?()
        })
    case .flyDown:
      willDismissWithType(dismissType: dismissType)

      overlayView.alpha = kPopupBackgroundAlpha
      UIView.animate(
        withDuration: 0.2, delay: 0.0, options: [.beginFromCurrentState, .curveEaseIn],
        animations: {
          var flyawayFrame: CGRect = self.dialogView.frame
          flyawayFrame.origin.y = self.overlayView.bounds.height
          flyawayFrame.origin.y += 20.0
          self.dialogView.frame = flyawayFrame
          self.overlayView.alpha = 0.0
        },
        completion: { (finished) in
          self.removeFromSuperview()
          self.overlayView.alpha = self.kPopupBackgroundAlpha
          self.didDismissWithType(dismissType: dismissType)
          self.delegate?.popupViewDidDismiss(self)
          self.dismissHandler?()
        })
    }
  }

  // MARK: Options

  func setStyle(popupStyle: PopupViewStyle) {
    style = popupStyle

    switch popupStyle {
    case .dialog:
      dialogView.layer.cornerRadius = kPopupDialogCornerRadius
      dialogView.layer.cornerCurve = .continuous
      defaultShowType = .flyUp
      defaultDismissType = .flyDown
      verticalAlignment = .middle
      padding = kPopupDialogPadding
    case .sheet:
      dialogView.layer.cornerRadius = 0.0
      defaultShowType = .flyUp
      defaultDismissType = .flyDown
      verticalAlignment = .bottom
      padding = 0.0
    }

    setNeedsLayout()
  }

  func setVerticalAlignment(alignment: PopupViewAlignment) {
    verticalAlignment = alignment
    setNeedsLayout()
  }

  func setDialogColor(color: UIColor) {
    dialogView.backgroundColor = color
  }

  func setOverlayColor(color: UIColor, animate: Bool) {
    if !animate {
      overlayView.backgroundColor = color
    } else {
      UIView.animate(
        withDuration: 0.35, delay: 0.0, options: [.beginFromCurrentState],
        animations: {
          self.overlayView.backgroundColor = color
        }, completion: nil)
    }
  }

  public func setPopupContentView(view: UIView) {
    contentView?.removeFromSuperview()
    contentView = view
    contentView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    dialogView.addSubview(contentView)
    setNeedsLayout()
  }

  public func addButton(title: String, type: ButtonType = .secondary, fontSize: CGFloat? = 15, tapped: (() -> PopupViewDismissType)?) {
    let buttonData: ButtonData = ButtonData()
    buttonData.title = title
    buttonData.handler = tapped
    buttonData.type = type
    if let fontSize = fontSize {
      buttonData.font = UIFont.systemFont(ofSize: fontSize, weight: UIFont.Weight.semibold)
    }

    dialogButtons.append(buttonData)
    setNeedsLayout()
  }

  public func setTitle(title: String, buttonIndex: Int) {
    if buttonIndex >= dialogButtons.count {
      return
    }

    let buttonData: ButtonData = dialogButtons[buttonIndex]
    buttonData.title = title

    if let button = buttonData.button {
      button.setTitle(title, for: .normal)
    }
  }

  public func removeAllButtons() {
    dialogButtons.forEach { $0.button?.removeFromSuperview() }
    dialogButtons.removeAll()
    setNeedsLayout()
  }

  public func removeButtonAtIndex(buttonIndex: Int) {
    if buttonIndex >= dialogButtons.count {
      return
    }

    let buttonData: ButtonData = dialogButtons[buttonIndex]
    if let button = buttonData.button {
      button.removeFromSuperview()
    }

    dialogButtons.remove(at: buttonIndex)
    setNeedsLayout()
  }

  public func numberOfButtons() -> Int {
    return dialogButtons.count
  }

  // MARK: Actions

  @objc func dialogButtonTapped(button: UIButton) {
    var dismissType = defaultDismissType

    for buttonData in dialogButtons {
      if let target = buttonData.button {
        if target == button {
          if let handler = buttonData.handler {
            dismissType = handler()
            break
          }
        }
      }
    }

    dismissWithType(dismissType: dismissType)
  }

  // MARK: Background

  public func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
    let point: CGPoint = touch.location(in: dialogView)
    return dialogView.point(inside: point, with: nil) == false
  }

  @objc func backgroundTapped(recognizer: UIGestureRecognizer?) {
    if overlayDismisses == false {
      return
    }

    guard let recognizer = recognizer else { return }

    if recognizer.state == .began {
      if let overlayDismissHandler = overlayDismissHandler, overlayDismissHandler() {
        dismiss()
      }
    }
  }

  // MARK: Subclass Hooks

  func willShowWithType(showType: PopupViewShowType) {}
  func didShowWithType(showType: PopupViewShowType) {}
  func willDismissWithType(dismissType: PopupViewDismissType) {}
  func didDismissWithType(dismissType: PopupViewDismissType) {}
}

extension UIKitPopupView: KeyboardHelperDelegate {
  public func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillShowWithState state: KeyboardState) {
    keyboardState = state
    if !automaticallyMovesWithKeyboard {
      return
    }
    let keyboardHeight = keyboardState?.intersectionHeightForView(applicationWindow ?? self) ?? 0
    _dialogFrameWithKeyboardHeight(height: keyboardHeight)
  }

  public func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillHideWithState state: KeyboardState) {
    keyboardState = nil
    if !automaticallyMovesWithKeyboard {
      return
    }
    let keyboardHeight = keyboardState?.intersectionHeightForView(applicationWindow ?? self) ?? 0
    _dialogFrameWithKeyboardHeight(height: keyboardHeight)
  }
}
