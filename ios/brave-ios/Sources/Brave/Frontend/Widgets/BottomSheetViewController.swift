/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SnapKit
import UIKit

/// Presents a sheet with a child view controller of choice.
/// On iPhones it presents as a bottom drawer style.
/// On iPads it presents as a popup at center of the screen.
class BottomSheetViewController: UIViewController {

  private struct UX {
    static let handleWidth: CGFloat = 35
    static let handleHeight: CGFloat = 5
    static let handleMargin: CGFloat = 20
  }

  /// For landscape orientation the content view isn't full width.
  /// A fixed constant is used instead.
  private let maxHorizontalWidth = 400
  private let animationDuration: TimeInterval = 0.25

  // let childViewController: UIViewController

  // MARK: - Views

  let contentView = UIView().then {
    $0.layer.cornerRadius = 12
    $0.layer.cornerCurve = .continuous
  }

  private let backgroundOverlayView = UIView().then {
    $0.backgroundColor = .black
    $0.alpha = 0
  }
  private let handleView = UIView().then {
    $0.backgroundColor = .black
    $0.alpha = 0.25
    $0.layer.cornerCurve = .continuous
    $0.layer.cornerRadius = UX.handleHeight / 2
  }

  private let closeButton = UIButton().then {
    $0.setImage(UIImage(named: "close_popup", in: .module, compatibleWith: nil)!.template, for: .normal)
    $0.tintColor = .lightGray
  }

  // MARK: - Constraint properties

  /// Controls height of the content view.
  private var yPosition: CGFloat = 0 {
    didSet {
      let maxY = view.frame.maxY

      func update() {
        if maxY <= 0 { return }
        // Update dark blur, the more of content view go away the less dark it is.
        backgroundOverlayView.alpha = (maxY - yPosition) / maxY

        let newFrame = contentView.frame
        contentView.frame = CGRect(x: newFrame.minX, y: yPosition, width: newFrame.width, height: newFrame.height)
        view.layoutIfNeeded()
      }

      if oldValue == yPosition { return }

      // All vertical position manipulation on iPads happens programatically,
      // no need to check for Y position limits.
      if showAsPopup {
        update()
        return
      }

      let initialY = initialDrawerYPosition

      // Only move the view if dragged below initial level.
      if yPosition <= initialY {
        yPosition = initialY
      } else if yPosition > maxY {  // Dragged all way down, remove the view.
        yPosition = maxY
      }

      update()
    }
  }

  private var initialDrawerYPosition: CGFloat {
    let popupY = ceil((view.frame.height / 2) - (contentView.frame.height / 2))

    let regularY = view.frame.maxY - contentView.frame.height

    return showAsPopup ? popupY : regularY
  }

  private var showAsPopup: Bool {
    traitCollection.userInterfaceIdiom == .pad && traitCollection.horizontalSizeClass == .regular
  }

  private var isLandscapePhone: Bool {
    traitCollection.userInterfaceIdiom == .phone && UIApplication.shared.statusBarOrientation.isLandscape
  }

  // MARK: - Lifecycle

  init() {
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) { fatalError() }

  override func viewDidLoad() {
    super.viewDidLoad()

    view.addSubview(backgroundOverlayView)
    view.addSubview(contentView)

    contentView.backgroundColor = .white

    let panGestureRecognizer = UIPanGestureRecognizer(target: self, action: #selector(handlePanGesture))
    contentView.addGestureRecognizer(panGestureRecognizer)

    contentView.addSubview(handleView)

    contentView.addSubview(closeButton)

    closeButton.addTarget(self, action: #selector(closeView), for: .touchUpInside)
    contentView.isHidden = true

    makeConstraints()
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    show()
  }

  override func viewDidLayoutSubviews() {
    yPosition = contentView.isHidden ? view.frame.maxY : initialDrawerYPosition
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    view.setNeedsUpdateConstraints()
  }

  override func updateViewConstraints() {
    let allCorners: CACornerMask = [
      .layerMinXMinYCorner, .layerMaxXMinYCorner,
      .layerMinXMaxYCorner, .layerMaxXMaxYCorner,
    ]

    let onlyTopCorners: CACornerMask = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    contentView.layer.maskedCorners = showAsPopup ? allCorners : onlyTopCorners

    handleView.isHidden = showAsPopup

    // Don't remake constraints if not needed.
    if yPosition == initialDrawerYPosition { return }

    contentView.snp.remakeConstraints {
      if showAsPopup || isLandscapePhone {
        $0.bottom.centerX.equalToSuperview()
        $0.width.equalTo(maxHorizontalWidth)

        // If contents can't fit the screen height, we have to add top constraint
        // in order to squeeze views in the `contentView`.
        // At the moment it only happens on iPhone 5S in landscape mode.
        //
        // Otherwise no top constraint is set, it will take as much space as needed.
        $0.top.greaterThanOrEqualToSuperview()
      } else {
        $0.leading.trailing.bottom.equalToSuperview()
      }
    }

    super.updateViewConstraints()
  }

  // MARK: - Constraints setup

  private func makeConstraints() {
    backgroundOverlayView.snp.makeConstraints {
      let bottomInset = parent?.view.safeAreaInsets.bottom ?? 0
      $0.leading.trailing.top.equalToSuperview()
      $0.bottom.equalToSuperview().offset(bottomInset)
    }

    handleView.snp.remakeConstraints {
      $0.width.equalTo(UX.handleWidth)
      $0.height.equalTo(UX.handleHeight)

      $0.centerX.equalTo(contentView)
      $0.top.equalTo(contentView).offset((UX.handleMargin - UX.handleHeight) / 2)
    }

    closeButton.snp.makeConstraints {
      $0.top.equalToSuperview().inset(10)
      $0.right.equalToSuperview().inset(7)
      $0.size.equalTo(26)
    }
  }

  // MARK: - Animations

  @objc private func handlePanGesture(_ pan: UIPanGestureRecognizer) {
    // The view shouldn't be draggable on iPads
    if showAsPopup { return }

    let translation = pan.translation(in: contentView)
    yPosition += translation.y
    pan.setTranslation(CGPoint.zero, in: contentView)

    if pan.state != .ended { return }

    let projectedVelocity = project(
      initialVelocity: pan.velocity(in: contentView).y,
      decelerationRate: UIScrollView.DecelerationRate.normal.rawValue)

    let nextYPosition: CGFloat

    let bottomHalfOfChildView = view.frame.height - (contentView.frame.height / 2)
    let pannedPastHalfOfViewHeight = yPosition > bottomHalfOfChildView

    let closeByVelocity = projectedVelocity + yPosition > contentView.frame.maxY

    if pannedPastHalfOfViewHeight || closeByVelocity {
      nextYPosition = view.frame.maxY
    } else {
      nextYPosition = 0
    }

    UIView.animate(
      withDuration: animationDuration,
      animations: {
        self.yPosition = nextYPosition

      }
    ) { _ in
      if nextYPosition > 0 {
        self.view.removeFromSuperview()
        self.removeFromParent()
      }
    }
  }

  private func show() {
    contentView.isHidden = false
    UIView.animate(withDuration: animationDuration) {
      self.yPosition = self.initialDrawerYPosition
    }
  }

  @objc private func closeView() {
    close()
  }

  func close() {
    UIView.animate(
      withDuration: animationDuration,
      animations: {
        self.yPosition = self.view.frame.maxY
      }
    ) { _ in
      self.view.removeFromSuperview()
      self.removeFromParent()
    }
  }

  // Distance travelled after decelerating to zero velocity at a constant rate (credit: a WWDC video)
  private func project(initialVelocity: CGFloat, decelerationRate: CGFloat) -> CGFloat {
    return (initialVelocity / 1000.0) * decelerationRate / (1.0 - decelerationRate)
  }
}
