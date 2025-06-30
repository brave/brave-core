// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SnapKit
import UIKit

/// A transparent overlay that captures taps outside the toast view and triggers a callback.
/// Used specifically to handle taps in the URL toolbar area.
class ToastTapInterceptingOverlay: UIView {
  var didTapOutside: (() -> Void)?

  override init(frame: CGRect) {
    super.init(frame: frame)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    didTapOutside?()
    return nil
  }
}

class ToastShadowView: UIView {
  private var shadowLayer: CAShapeLayer?

  override func layoutSubviews() {
    super.layoutSubviews()

    let path = UIBezierPath(
      roundedRect: bounds,
      cornerRadius: ButtonToastUX.toastButtonBorderRadius
    ).cgPath
    if let shadowLayer = shadowLayer {
      shadowLayer.path = path
      shadowLayer.shadowPath = path
    } else {
      shadowLayer = CAShapeLayer().then {
        $0.path = path
        $0.fillColor = UIColor.clear.cgColor
        $0.shadowColor = UIColor.black.cgColor
        $0.shadowPath = path
        $0.shadowOffset = .zero
        $0.shadowOpacity = 0.5
        $0.shadowRadius = ButtonToastUX.toastButtonBorderRadius
      }

      shadowLayer?.do {
        layer.insertSublayer($0, at: 0)
      }
    }
  }
}

class Toast: UIView {
  var completionHandler: ((Bool) -> Void)?

  weak var viewController: UIViewController?

  var displayState = State.dismissed

  var tapDismissalMode: TapDismissalMode = .dismissOnOutsideTap

  private var animationConstraint: Constraint?

  private lazy var tapInterceptingOverlay = ToastTapInterceptingOverlay()

  lazy var gestureRecognizer: UITapGestureRecognizer = {
    let gestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(handleTap))
    gestureRecognizer.cancelsTouchesInView = false
    return gestureRecognizer
  }()

  /// TODO(https://github.com/brave/brave-browser/issues/46198): Make private to
  /// for looser coupling with subclasses.
  lazy var toastView: UIView = {
    let toastView = UIView()
    toastView.backgroundColor = SimpleToastUX.toastDefaultColor
    return toastView
  }()

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    superview?.addGestureRecognizer(gestureRecognizer)

    if let superview = superview {
      superview.insertSubview(tapInterceptingOverlay, belowSubview: self)
      tapInterceptingOverlay.frame = superview.bounds
    } else {
      tapInterceptingOverlay.removeFromSuperview()
    }
  }

  func showToast(
    viewController: UIViewController? = nil,
    delay: DispatchTimeInterval,
    duration: DispatchTimeInterval?,
    makeConstraints: @escaping (ConstraintMaker) -> Void,
    completion: (() -> Void)? = nil
  ) {
    self.viewController = viewController

    self.displayState = .pendingShow

    DispatchQueue.main.asyncAfter(deadline: .now() + delay) {
      self.tapInterceptingOverlay.didTapOutside = { [weak self] in
        if self?.tapDismissalMode == .noDismissal {
          return
        }
        self?.dismiss(false, completion: completion)
      }

      viewController?.view.addSubview(self)

      self.layer.removeAllAnimations()
      self.snp.makeConstraints(makeConstraints)
      self.layoutIfNeeded()

      UIView.animate(
        withDuration: SimpleToastUX.toastAnimationDuration,
        animations: {
          self.animationConstraint?.update(offset: -self.bounds.height)
          self.layoutIfNeeded()
        },
        completion: { finished in
          self.displayState = .showing

          self.anchorToastToBottom()

          if let duration = duration {
            DispatchQueue.main.asyncAfter(deadline: .now() + duration) {
              self.dismiss(false, completion: completion)
            }
          }
        }
      )
    }
  }

  func dismiss(_ buttonPressed: Bool, animated: Bool = true, completion: (() -> Void)? = nil) {
    if displayState == .pendingDismiss || displayState == .dismissed {
      return
    }

    displayState = .pendingDismiss
    superview?.removeGestureRecognizer(gestureRecognizer)
    layer.removeAllAnimations()

    let duration = animated ? SimpleToastUX.toastAnimationDuration : 0.1

    setupDismissalToastConstraints()

    UIView.animate(
      withDuration: duration,
      animations: {
        self.animationConstraint?.update(offset: 0)
        self.layoutIfNeeded()
      },
      completion: { finished in
        self.displayState = .dismissed
        self.tapInterceptingOverlay.removeFromSuperview()
        self.removeFromSuperview()
        if !buttonPressed {
          self.completionHandler?(false)
        }

        completion?()
      }
    )
  }

  @objc func handleTap(_ gestureRecognizer: UIGestureRecognizer) {
    if tapDismissalMode == .noDismissal {
      return
    }

    if tapDismissalMode == .dismissOnOutsideTap {
      let location = gestureRecognizer.location(in: self)
      // Check if the tap was inside the toast view
      if self.point(inside: location, with: nil) {
        return
      }
    }

    dismiss(false)
  }

  /// TODO(https://github.com/brave/brave-browser/issues/46198): Make private to
  /// for looser coupling with subclasses.
  func setupInitialToastConstraints() {
    toastView.snp.makeConstraints { make in
      make.leading.trailing.height.equalTo(self)
      self.animationConstraint = make.top.equalTo(self.snp.bottom).constraint
    }
  }

  private func setupDismissalToastConstraints() {
    toastView.snp.remakeConstraints { make in
      make.leading.trailing.height.equalTo(self)
      self.animationConstraint =
        make.top.equalTo(self.snp.bottom).offset(-self.bounds.height).constraint
    }
  }

  private func anchorToastToBottom() {
    toastView.snp.remakeConstraints { make in
      make.leading.trailing.height.equalTo(self)
      make.bottom.equalTo(self.snp.bottom)
    }
  }

  enum State {
    case showing
    case pendingShow
    case pendingDismiss
    case dismissed
  }

  enum TapDismissalMode {
    case dismissOnOutsideTap
    case dismissOnAnyTap
    case noDismissal
  }
}
