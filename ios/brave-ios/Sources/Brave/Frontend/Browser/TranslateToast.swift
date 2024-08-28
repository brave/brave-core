// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import SnapKit
import UIKit

class TranslateToast: Toast {
  private struct DesignUX {
    static let maxToastWidth: CGFloat = BraveUX.baseDimensionValue
  }

  private let toastShadowView = ToastShadowView()
  private var panState: CGPoint = .zero

  init(
    completion: ((_ buttonPressed: Bool) -> Void)?
  ) {
    super.init(frame: .zero)

    self.completionHandler = completion
    clipsToBounds = false

    addSubview(createView())

    toastView.backgroundColor = UIColor.braveBlurpleTint
    toastView.layer.cornerRadius = 8.0
    toastView.layer.cornerCurve = .continuous
    toastView.layer.masksToBounds = true

    toastView.snp.makeConstraints {
      $0.leading.trailing.equalTo(self).inset(ButtonToastUX.toastPadding * 2.0)
      $0.height.equalTo(self)
      self.animationConstraint = $0.top.equalTo(self).offset(ButtonToastUX.toastHeight).constraint
    }

    self.snp.makeConstraints {
      $0.height.equalTo(ButtonToastUX.toastHeight)
    }

    let panGesture = UIPanGestureRecognizer(target: self, action: #selector(onSwipeToDismiss(_:)))
    toastView.addGestureRecognizer(panGesture)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func createView() -> UIView {
    let label = UILabel()
    label.textAlignment = .center
    label.textColor = .white
    label.font = ButtonToastUX.toastLabelFont
    label.text = "Tap Here To Translate"
    label.lineBreakMode = .byWordWrapping
    label.numberOfLines = 0

    let horizontalStackView = UIStackView().then {
      $0.alignment = .center
      $0.spacing = ButtonToastUX.toastPadding
    }

    horizontalStackView.addArrangedSubview(label)
    //    toastView.addSubview(toastShadowView)
    toastView.addSubview(horizontalStackView)

    //    toastShadowView.snp.makeConstraints {
    //      $0.edges.equalToSuperview()
    //    }

    label.snp.makeConstraints { make in
      make.centerX.equalTo(toastView)
    }

    horizontalStackView.snp.makeConstraints { make in
      make.centerX.equalTo(toastView)
      make.centerY.equalTo(toastView)
      make.width.equalTo(toastView.snp.width).offset(-2 * ButtonToastUX.toastPadding)
    }

    return toastView
  }

  @objc override func handleTap(_ gestureRecognizer: UIGestureRecognizer) {
    dismiss(false)
  }

  override func showToast(
    viewController: UIViewController? = nil,
    delay: DispatchTimeInterval,
    duration: DispatchTimeInterval?,
    makeConstraints: @escaping (ConstraintMaker) -> Void,
    completion: (() -> Void)? = nil
  ) {
    super.showToast(
      viewController: viewController,
      delay: delay,
      duration: duration,
      makeConstraints: makeConstraints
    )
  }

  func dismiss(_ buttonPressed: Bool) {
    self.dismiss(buttonPressed, animated: true)
  }

  @objc
  private func onSwipeToDismiss(_ recognizer: UIPanGestureRecognizer) {
    // Distance travelled after decelerating to zero velocity at a constant rate
    func project(initialVelocity: CGFloat, decelerationRate: CGFloat) -> CGFloat {
      return (initialVelocity / 1000.0) * decelerationRate / (1.0 - decelerationRate)
    }

    if recognizer.state == .began {
      panState = toastView.center
    } else if recognizer.state == .ended {
      let velocity = recognizer.velocity(in: toastView)
      if abs(velocity.y) > abs(velocity.x) {
        let y = min(panState.y, panState.y + recognizer.translation(in: toastView).y)
        let projected = project(
          initialVelocity: velocity.y,
          decelerationRate: UIScrollView.DecelerationRate.normal.rawValue
        )
        if y + projected > toastView.frame.maxY {
          dismiss(false)
        }
      }
    }
  }
}
