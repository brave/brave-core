// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Data
import DesignSystem
import Foundation
import Shared
import SnapKit
import UIKit

private class ToastShadowView: UIView {
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

private class HighlightableButton: UIButton {
  private var shadowLayer: CAShapeLayer?

  var shadowLayerZOrder: Int {
    guard let shadowLayer = shadowLayer else { return -1 }
    return self.layer.sublayers?.firstIndex(of: shadowLayer) ?? -1
  }

  var isShadowHidden: Bool = false {
    didSet {
      shadowLayer?.isHidden = isShadowHidden
    }
  }

  override var isHighlighted: Bool {
    didSet {
      backgroundColor = isHighlighted ? UIColor.white.withAlphaComponent(0.2) : .clear
    }
  }

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

class PlaylistToast: Toast {
  private struct DesignUX {
    static let maxToastWidth: CGFloat = BraveUX.baseDimensionValue
  }

  private let toastShadowView = ToastShadowView()
  private lazy var gradientView = BraveGradientView.gradient02

  private let button = HighlightableButton()
  private var panState: CGPoint = .zero

  private let state: PlaylistItemAddedState
  var item: PlaylistInfo?

  init(
    item: PlaylistInfo?,
    state: PlaylistItemAddedState,
    completion: ((_ buttonPressed: Bool) -> Void)?
  ) {
    self.item = item
    self.state = state
    super.init(frame: .zero)

    self.completionHandler = completion
    toastView.backgroundColor = .clear
    clipsToBounds = false

    addSubview(createView(item, state))

    toastView.snp.makeConstraints {
      $0.leading.trailing.height.equalTo(self)
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

  func createView(_ item: PlaylistInfo?, _ state: PlaylistItemAddedState) -> UIView {
    if state == .newItem || state == .existingItem {
      let horizontalStackView = UIStackView().then {
        $0.alignment = .center
        $0.spacing = ButtonToastUX.toastPadding
      }

      let labelStackView = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .leading
      }

      let label = UILabel().then {
        $0.textAlignment = .left
        $0.textColor = .white
        $0.font = ButtonToastUX.toastLabelFont
        $0.lineBreakMode = .byWordWrapping
        $0.numberOfLines = 0

        if state == .newItem {
          $0.text = Strings.PlayList.toastAddedToPlaylistTitle
        } else {
          $0.text = Strings.PlayList.toastExitingItemPlaylistTitle
        }
      }

      self.button.do {
        $0.layer.cornerRadius = ButtonToastUX.toastButtonBorderRadius
        $0.layer.borderWidth = ButtonToastUX.toastButtonBorderWidth
        $0.layer.borderColor = UIColor.white.cgColor
        $0.imageView?.tintColor = .white
        $0.setTitle(Strings.PlayList.toastAddToPlaylistOpenButton, for: [])
        $0.setTitleColor(.white, for: .highlighted)
        $0.titleLabel?.font = SimpleToastUX.toastFont
        $0.titleLabel?.numberOfLines = 1
        $0.titleLabel?.lineBreakMode = .byClipping
        $0.titleLabel?.adjustsFontSizeToFitWidth = true
        $0.titleLabel?.minimumScaleFactor = 0.1
        $0.isShadowHidden = true
        $0.addGestureRecognizer(
          UITapGestureRecognizer(target: self, action: #selector(buttonPressed))
        )
      }

      self.button.snp.makeConstraints {
        if let titleLabel = self.button.titleLabel {
          $0.width.equalTo(
            titleLabel.intrinsicContentSize.width + 2 * ButtonToastUX.toastButtonPadding
          )
        }
      }

      labelStackView.addArrangedSubview(label)
      horizontalStackView.addArrangedSubview(labelStackView)
      horizontalStackView.addArrangedSubview(button)

      toastView.addSubview(toastShadowView)
      toastView.addSubview(horizontalStackView)

      toastShadowView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }

      horizontalStackView.snp.makeConstraints {
        $0.centerX.equalTo(toastView)
        $0.centerY.equalTo(toastView)
        $0.width.equalTo(toastView.snp.width).offset(-2 * ButtonToastUX.toastPadding)
      }

      updateGradientView()
      return toastView
    }

    let horizontalStackView = UIStackView().then {
      $0.alignment = .center
      $0.spacing = ButtonToastUX.toastPadding
    }

    self.button.do {
      $0.layer.cornerRadius = ButtonToastUX.toastButtonBorderRadius
      $0.backgroundColor = .clear
      $0.setTitleColor(.white, for: .highlighted)
      $0.imageView?.tintColor = .white
      $0.tintColor = .white
      $0.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: .medium)
      $0.titleLabel?.numberOfLines = 1
      $0.titleLabel?.lineBreakMode = .byClipping
      $0.titleLabel?.adjustsFontSizeToFitWidth = true
      $0.titleLabel?.minimumScaleFactor = 0.1
      $0.contentHorizontalAlignment = .left
      $0.contentEdgeInsets = UIEdgeInsets(top: 10.0, left: 10.0, bottom: 10.0, right: 20.0)
      $0.titleEdgeInsets = UIEdgeInsets(top: 0.0, left: 10.0, bottom: 0.0, right: -10.0)
      $0.isShadowHidden = false
      $0.addGestureRecognizer(
        UITapGestureRecognizer(target: self, action: #selector(buttonPressed))
      )
    }

    horizontalStackView.addArrangedSubview(button)
    toastView.addSubview(horizontalStackView)

    horizontalStackView.snp.makeConstraints {
      $0.centerX.equalTo(toastView)
      $0.centerY.equalTo(toastView)
      $0.width.equalTo(toastView.snp.width).offset(-2 * ButtonToastUX.toastPadding)
    }

    if state == .none {
      button.setImage(
        UIImage(named: "quick_action_new_tab", in: .module, compatibleWith: nil)!.template,
        for: []
      )
      button.setTitle(Strings.PlayList.toastAddToPlaylistTitle, for: [])
    } else {
      assertionFailure(
        "Should Never get here. Others case are handled at the start of this function."
      )
    }

    updateGradientView()
    return toastView
  }

  @objc func buttonPressed(_ gestureRecognizer: UIGestureRecognizer) {
    completionHandler?(true)
    dismiss(true)
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
    super.showToast(viewController: viewController, delay: delay, duration: duration) {
      guard let viewController = viewController as? BrowserViewController else {
        assertionFailure("Playlist Toast should only be presented on BrowserViewController")
        return
      }

      $0.centerX.equalTo(viewController.view.snp.centerX)
      $0.bottom.equalTo(viewController.webViewContainer.safeArea.bottom)
      $0.leading.equalTo(viewController.view.safeArea.leading).priority(.high)
      $0.trailing.equalTo(viewController.view.safeArea.trailing).priority(.high)
      $0.width.lessThanOrEqualTo(DesignUX.maxToastWidth)
    }
  }

  func dismiss(_ buttonPressed: Bool) {
    self.dismiss(buttonPressed, animated: true)
  }

  private var shadowLayerZOrder: Int {
    if state == .newItem || state == .existingItem {
      // In this state, the shadow is on the toastView
      let index = toastView.subviews.firstIndex(of: toastShadowView) ?? -1
      return index + 1
    }
    // In this state, the shadow is on the button itself
    return button.shadowLayerZOrder + 1
  }

  private func updateGradientView() {
    gradientView.removeFromSuperview()

    if state == .newItem || state == .existingItem {
      toastView.insertSubview(gradientView, at: shadowLayerZOrder)
    } else {
      button.insertSubview(gradientView, at: shadowLayerZOrder)
    }

    gradientView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
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
