/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SnapKit
import SwiftUI

extension UILayoutPriority {
  /// The priority used for the container view's width & height when using ContentSizeBehavior.preferredContentSize
  /// or ContentSizeBehavior.fixedSize.
  ///
  /// Must be higher than `UILayoutPriority.defaultHigh` or width/height constraints with be ignored
  fileprivate static let popoverPreferredOrFixedSize = UILayoutPriority(rawValue: 850.0)
}

/// A popover which presents a `UIViewController` from a point of origin
///
/// - note: You must use `present(from:on:)` from an instantiated `PopoverController` to present a popover. Presenting
/// another way will result in undefined behavior
public class PopoverController: UIViewController {

  /// The preferred popover width when using `ContentSizeBehavior.preferredContentSize` or
  /// `ContentSizeBehavior.fixedSize`
  public static let preferredPopoverWidth: CGFloat = 320.0
  /// Defines the behavior of the arrow direction and how the popover presents itself
  public enum ArrowDirectionBehavior {
    /// Determines the direction of the popover based on the origin of the popover
    ///
    /// If the y origin of the popover is more than halfway to the bottom of the presenting view controller, it will
    /// attempt to present with the arrow pointing down, The same is true but opposite logic for if the y origin is less
    /// than half the height
    case automatic
    /// Forces a specific arrow direction regardless of the origin and content height
    case forcedDirection(ArrowDirection)
  }

  /// Defines the behavior of how the popover sizes itself to fit the content
  public enum ContentSizeBehavior {
    /// The popover content view's size will be tied to the content controller view's size
    case autoLayout(_ configuration: ContentSizeConfiguration? = nil)
    /// The popover will size itself based on `UIViewController.preferredContentSize`
    case preferredContentSize
    /// The popover content view will be fixed to a given size
    case fixedSize(CGSize)
  }
  
  /// Defines configuration details of `ContentSizeBehavior.autoLayout`
  /// The popover will resize itself according to size configuration type chosen
  public struct ContentSizeConfiguration {
    /// The custom width determined by configuration type
    var preferredWidth: CGFloat?
    /// The custom height determined by configuration type
    var preferredHeight: CGFloat?
    /// The configuration which sets popover width to phone width under max amount
    public static var phoneWidth: Self {
      return .init(
        preferredWidth: min(440, UIScreen.main.bounds.width)
      )
    }
    /// The configuration which sets popover height to phone width under max amount
    public static var phoneHeight: Self {
      return .init(
        preferredHeight: min(750, UIScreen.main.bounds.height)
      )
    }
    /// The configuration which sets popover height / width to phone width under max amount
    public static var phoneBounds: Self {
      return .init(
        preferredWidth: min(440, UIScreen.main.bounds.width),
        preferredHeight: min(750, UIScreen.main.bounds.height)
      )
    }
  }

  /// Outer margins around the presented popover to the edge of the screen (or safe area)
  public var outerMargins = UIEdgeInsets(top: 10.0, left: 10.0, bottom: 10.0, right: 10.0)

  /// Whether or not to automatically add a margins when the popover is presented so the user can dismiss it more
  /// easily.
  ///
  /// Currently this only adds a bottom-margin for portrait presentations where the popover is being presented
  /// from the top of the screen
  public var addsConvenientDismissalMargins = true

  /// The amount of space to add for convenient dismissals
  public var convenientDismissalMargin: CGFloat = 80.0

  /// The distance from the popover arrow to the origin view
  public var arrowDistance: CGFloat = 0

  /// The arrow direction behavior for this popover
  public var arrowDirectionBehavior: ArrowDirectionBehavior = .automatic

  /// Whether or not to automatically dismiss the popup when the device orientation changes
  var dismissesOnOrientationChanged = true
  
  /// The origin preview to use.
  public struct OriginPreview {
    /// The view to place on top of the origin. A snapshot will be taken of this view and placed where the
    /// popover is presented from
    public var view: UIView
    /// Whether or not to add a background focus rings animation behind the `view` provided
    public var displaysFocusRings: Bool
    /// An action to take when the user taps on the `view` provided. Defaults to nil which will dismiss
    /// the popover. If you handle this manually it is up to you to call `dismissPopover` on the popover
    /// controller.
    public var action: ((PopoverController) -> Void)?
    
    public init(view: UIView, displaysFocusRings: Bool = true, action: ((PopoverController) -> Void)? = nil) {
      self.view = view
      self.displaysFocusRings = displaysFocusRings
      self.action = action
    }
  }
  
  /// Whether or not to display a preview where the popover originated from
  public var previewForOrigin: OriginPreview? {
    didSet {
      if oldValue != nil && previewForOrigin == nil {
        // Preview can be removed during presentation
        let originFocusView = self.originFocusView
        let originSnapshotView = self.originSnapshotView
        self.originFocusView = nil
        self.originSnapshotView = nil
        let animator = UIViewPropertyAnimator(duration: 0.1, curve: .linear) {
          originFocusView?.alpha = 0
          originSnapshotView?.alpha = 0
        }
        animator.addCompletion { _ in
          originFocusView?.removeFromSuperview()
          originSnapshotView?.removeFromSuperview()
        }
        animator.startAnimation()
      }
    }
  }

  /// Defines the desired color for the entire popup menu
  /// Child controller may specify their own `backgroundColor`, however arrow/carrot color is also handled here
  public var color: UIColor? {
    didSet {
      containerView.color = color
    }
  }

  /// Allows the presenter to know when the popover was dismissed by some gestural action.
  public var popoverDidDismiss: ((_ popoverController: PopoverController) -> Void)?

  public let contentSizeBehavior: ContentSizeBehavior

  private var containerViewHeightConstraint: NSLayoutConstraint?
  private var containerViewWidthConstraint: NSLayoutConstraint?

  /// Create a popover displaying a content controller
  public init(contentController: UIViewController & PopoverContentComponent, contentSizeBehavior: ContentSizeBehavior = .autoLayout()) {
    self.contentController = contentController
    self.contentSizeBehavior = contentSizeBehavior

    super.init(nibName: nil, bundle: nil)

    if let navigationController = contentController as? UINavigationController {
      if case .autoLayout = contentSizeBehavior {
        assertionFailure("`autoLayout` content size behavior is not supported with UINavigationController, please use `preferredContentSize` or `fixedSize`")
      }
      navigationController.delegate = self
      navigationController.interactivePopGestureRecognizer?.delegate = self
    }

    self.modalPresentationStyle = .overCurrentContext
    self.transitioningDelegate = self
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  private var autoLayoutTopConstraint: NSLayoutConstraint?
  private var autoLayoutBottomConstraint: NSLayoutConstraint?
  private var panGesture: UIPanGestureRecognizer?
  
  private var originFocusView: PopoverOriginFocusView?
  private var originSnapshotView: UIView?

  override public func viewDidLoad() {
    super.viewDidLoad()

    backgroundOverlayView.backgroundColor = UIColor(white: 0.0, alpha: 0.2)
    let backgroundTap = UITapGestureRecognizer(target: self, action: #selector(tappedBackgroundOverlay(_:)))
    backgroundOverlayView.isAccessibilityElement = true
    backgroundOverlayView.accessibilityLabel = contentController.closeActionAccessibilityLabel
    backgroundOverlayView.accessibilityTraits = [.button]
    backgroundOverlayView.addGestureRecognizer(backgroundTap)

    let pan = UIPanGestureRecognizer(target: self, action: #selector(pannedPopover(_:)))
    pan.delegate = self
    view.addGestureRecognizer(pan)
    panGesture = pan

    containerView.translatesAutoresizingMaskIntoConstraints = false

    view.addSubview(backgroundOverlayView)
    view.addSubview(containerView)

    backgroundOverlayView.snp.makeConstraints { make in
      make.edges.equalTo(self.view)
    }

    addChild(contentController)
    containerView.contentView.addSubview(contentController.view)
    contentController.didMove(toParent: self)
    
    containerView.contentView.backgroundColor = contentController.popoverBackgroundColor

    switch contentSizeBehavior {
    case .autoLayout(let configuration):
      contentController.view.snp.makeConstraints { make in
        autoLayoutTopConstraint =
          make.top.equalTo(self.containerView.contentView)
          .constraint.layoutConstraints.first
        autoLayoutBottomConstraint =
          make.bottom.equalTo(self.containerView.contentView)
          .constraint.layoutConstraints.first
        make.leading.trailing.equalTo(self.containerView.contentView)
      }
      
      if let preferredWidth = configuration?.preferredWidth {
        containerViewWidthConstraint = containerView.widthAnchor.constraint(equalToConstant: preferredWidth)
        containerViewWidthConstraint?.priority = .popoverPreferredOrFixedSize
        containerViewWidthConstraint?.isActive = true
      }
      
      if let preferredHeight = configuration?.preferredHeight {
        containerViewHeightConstraint = containerView.heightAnchor.constraint(equalToConstant: preferredHeight + PopoverUX.arrowSize.height)
        containerViewHeightConstraint?.priority = .popoverPreferredOrFixedSize
        containerViewHeightConstraint?.isActive = true
      }
    case .preferredContentSize:
      containerViewHeightConstraint = containerView.heightAnchor.constraint(equalToConstant: contentController.preferredContentSize.height + PopoverUX.arrowSize.height)
      containerViewHeightConstraint?.priority = .popoverPreferredOrFixedSize
      containerViewHeightConstraint?.isActive = true

      containerViewWidthConstraint = containerView.widthAnchor.constraint(equalToConstant: contentController.preferredContentSize.width)
      containerViewWidthConstraint?.priority = .popoverPreferredOrFixedSize
      containerViewWidthConstraint?.isActive = true

    case .fixedSize(let size):
      containerViewHeightConstraint = containerView.heightAnchor.constraint(equalToConstant: size.height + PopoverUX.arrowSize.height)
      containerViewHeightConstraint?.priority = .popoverPreferredOrFixedSize
      containerViewHeightConstraint?.isActive = true

      containerViewWidthConstraint = containerView.widthAnchor.constraint(equalToConstant: size.width)
      containerViewWidthConstraint?.priority = .popoverPreferredOrFixedSize
      containerViewWidthConstraint?.isActive = true
    }
  }
  
  override public var preferredStatusBarStyle: UIStatusBarStyle {
    return .default
  }

  override public func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    switch contentSizeBehavior {
    case .preferredContentSize, .fixedSize(_):
      containerView.layoutIfNeeded()
      if contentController.extendEdgeIntoArrow {
        contentController.view.frame = containerView.contentView.bounds
        break
      }
      var rect = containerView.contentView.bounds
      rect.size.height -= PopoverUX.arrowSize.height
      if containerView.arrowDirection == .up {
        rect.origin.y = PopoverUX.arrowSize.height
      }
      contentController.view.frame = rect

    case .autoLayout:
      if contentController.extendEdgeIntoArrow {
        autoLayoutTopConstraint?.constant = 0
        autoLayoutBottomConstraint?.constant = 0
      } else {
        if containerView.arrowDirection == .up {
          autoLayoutBottomConstraint?.constant = 0
          autoLayoutTopConstraint?.constant = PopoverUX.arrowSize.height
        } else {
          autoLayoutTopConstraint?.constant = 0
          autoLayoutBottomConstraint?.constant = -PopoverUX.arrowSize.height
        }
      }
    }
    
    if let presentationContext, panGesture?.state == .possible {
      let translationDelta = anchorPointDelta(from: presentationContext, popoverRect: containerView.frame)
      containerView.arrowOrigin = CGPoint(x: containerView.bounds.midX + translationDelta.x, y: 0.0)
    }
  }

  // MARK: - UI

  public private(set) var contentController: UIViewController & PopoverContentComponent

  private let containerView = ContainerView()

  public let backgroundOverlayView = UIView()

  override public func preferredContentSizeDidChange(forChildContentContainer container: UIContentContainer) {
    if case .preferredContentSize = contentSizeBehavior {
      var size = contentController.preferredContentSize
      if size == .zero {
        // Do nothing, keep it whatever it is currently
        return
      }

      size.width = min(size.width, UIScreen.main.bounds.width - outerMargins.left - outerMargins.right)
      size.height = min(size.height, UIScreen.main.bounds.height - view.safeAreaInsets.top - view.safeAreaInsets.bottom - outerMargins.top - outerMargins.bottom - arrowDistance)
      if contentController.view.bounds.size == size { return }

      contentController.view.setNeedsLayout()

      if let nc = self.contentController as? UINavigationController {
        nc.viewControllers.filter { $0.isViewLoaded }.forEach { vc in
          var frame = vc.view.frame
          frame.size.height = size.height + PopoverUX.arrowSize.height
          vc.view.frame = frame
        }
      }
      containerViewHeightConstraint?.constant = size.height + PopoverUX.arrowSize.height
      containerViewWidthConstraint?.constant = size.width
      view.setNeedsLayout()
      UIViewPropertyAnimator(duration: 0.4, dampingRatio: 0.9) { [self] in
        view.layoutIfNeeded()
      }.startAnimation()
    }
  }

  // MARK: - Presentation

  /// Context around the popover presenation
  private struct PresentationContext {
    /// Which view the popover is originating from
    var originView: UIView
    /// The origin's view center in the presenting view controller's coordinate system
    var convertedOriginViewCenter: CGPoint
    /// The initial size of the popover during presentation
    var presentedSize: CGSize
  }

  private var presentationContext: PresentationContext?

  /// Generate the anchor point delta based on the center of the origin view and the size of the container
  private func anchorPointDelta(from context: PresentationContext, popoverRect rect: CGRect) -> CGPoint {
    var deltaY = rect.height / 2.0
    if containerView.arrowDirection == .up {
      deltaY *= -1
    }

    return CGPoint(
      x: context.convertedOriginViewCenter.x - rect.midX,
      y: deltaY
    )
  }

  /// Presents the popover from a specific view's region
  ///
  /// - parameter view: The view to have the popover present from (scaling from the location of this view)
  /// - parameter viewController: The view controller to present this popover on
  public func present(from view: UIView, on viewController: UIViewController, completion: (() -> Void)? = nil) {
    let convertedOriginViewCenter = viewController.view.convert(view.center, from: view.superview)

    switch arrowDirectionBehavior {
    case .automatic:
      if convertedOriginViewCenter.y >= viewController.view.bounds.height / 2.0 {
        containerView.arrowDirection = .down
      } else {
        containerView.arrowDirection = .up
      }
    case .forcedDirection(let direction):
      containerView.arrowDirection = direction
    }

    if contentController.extendEdgeIntoArrow {
      switch containerView.arrowDirection {
      case .up:
        contentController.additionalSafeAreaInsets = UIEdgeInsets(top: PopoverUX.arrowSize.height, left: 0, bottom: 0, right: 0)
      case .down:
        contentController.additionalSafeAreaInsets = UIEdgeInsets(top: 0, left: 0, bottom: PopoverUX.arrowSize.height, right: 0)
      }
    }

    let isPortrait = UIDevice.current.orientation.isPortrait
    if addsConvenientDismissalMargins && isPortrait && containerView.arrowDirection == .up && outerMargins.bottom < convenientDismissalMargin {
      outerMargins.bottom = convenientDismissalMargin
    }

    let contentSize: CGSize

    switch contentSizeBehavior {
    case .autoLayout:
      let constrainedWidth = viewController.view.bounds.width
      let constrainedHeight = viewController.view.bounds.height

      contentSize = contentController.view.systemLayoutSizeFitting(CGSize(width: constrainedWidth - outerMargins.left - outerMargins.right, height: constrainedHeight - outerMargins.top - outerMargins.bottom))
    case .preferredContentSize:
      contentSize = contentController.preferredContentSize
    case .fixedSize(let size):
      contentSize = size
    }

    presentationContext = PresentationContext(
      originView: view,
      convertedOriginViewCenter: convertedOriginViewCenter,
      presentedSize: contentSize
    )

    viewController.present(self, animated: true, completion: completion)
  }
  
  public func dismissPopover(_ completion: (() -> Void)? = nil) {
    dismiss(animated: true) { [weak self] in
      guard let self = self else { return }
      
      self.popoverDidDismiss?(self)
      completion?()
    }
  }

  override public func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)

    if dismissesOnOrientationChanged {
      dismissPopover()
    }
  }
  
  @objc private func tappedOriginView() {
    if let action = previewForOrigin?.action {
      action(self)
    } else {
      dismissPopover()
    }
  }
}

// MARK: - Actions
extension PopoverController {

  @objc private func tappedBackgroundOverlay(_ tap: UITapGestureRecognizer) {
    if tap.state == .ended {
      if contentController.popoverShouldDismiss(self) {
        // Not sure if we want this after dismissal completes or right away. Could always create a
        // `popoverWillDismiss` to put before and `did` after
        dismissPopover()
      }
    }
  }

  @objc private func pannedPopover(_ pan: UIPanGestureRecognizer) {
    func _computedOffsetBasedOnRubberBandingResistance(distance x: CGFloat, constant c: CGFloat = 0.55, dimension d: CGFloat) -> CGFloat {
      /*
             f(x, d, c) = (x * d * c) / (d + c * x)

             where,
             x – distance from the edge
             c – constant (UIScrollView uses 0.55)
             d – dimension, either width or height
             */
      return (x * d * c) / (d + c * x)
    }

    guard let context = presentationContext else { return }

    var scale: CGFloat

    switch containerView.arrowDirection {
    case .up:
      scale = 1.0 - (-pan.translation(in: pan.view).y / containerView.bounds.height)
    case .down:
      scale = 1.0 - pan.translation(in: pan.view).y / containerView.bounds.height
    }

    scale = max(0.0, scale)
    if scale > 1 {
      scale =
        1.0
        + _computedOffsetBasedOnRubberBandingResistance(
          distance: scale - 1.0,
          constant: 0.15,
          dimension: containerView.bounds.height
        )
    }

    containerView.transform = .identity  // Reset to get unaltered frame
    let translationDelta = anchorPointDelta(from: context, popoverRect: containerView.frame)

    containerView.transform = CGAffineTransform(translationX: translationDelta.x, y: translationDelta.y)
      .scaledBy(x: scale, y: scale)
      .translatedBy(x: -translationDelta.x, y: -translationDelta.y)

    if pan.state == .ended {
      let passedVelocityThreshold: Bool
      let velocityThreshold: CGFloat = 100.0

      switch containerView.arrowDirection {
      case .up:
        passedVelocityThreshold = pan.velocity(in: pan.view).y < -velocityThreshold
      case .down:
        passedVelocityThreshold = pan.velocity(in: pan.view).y > velocityThreshold
      }

      if contentController.popoverShouldDismiss(self) && (passedVelocityThreshold || scale < 0.5) {
        dismissPopover()
      } else {
        UIView.animate(
          withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.7, initialSpringVelocity: 0, options: [.beginFromCurrentState, .allowUserInteraction],
          animations: {
            self.containerView.transform = .identity
          })
      }
    }

    if pan.state == .cancelled {
      UIView.animate(
        withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.7, initialSpringVelocity: 0, options: [.beginFromCurrentState, .allowUserInteraction],
        animations: {
          self.containerView.transform = .identity
        })
    }
  }
}

// MARK: - BasicAnimationControllerDelegate
extension PopoverController: BasicAnimationControllerDelegate {

  public func animatePresentation(context: UIViewControllerContextTransitioning) {
    guard let popoverContext = presentationContext else {
      context.completeTransition(false)
      return
    }

    context.containerView.addSubview(view)

    let originViewFrame = view.convert(popoverContext.originView.frame, from: popoverContext.originView.superview)
    let originLayoutGuide = UILayoutGuide()
    context.containerView.addLayoutGuide(originLayoutGuide)

    originLayoutGuide.snp.makeConstraints {
      $0.top.equalTo(self.view).offset(originViewFrame.minY)
      $0.left.equalTo(self.view).offset(originViewFrame.minX)
      $0.size.equalTo(originViewFrame.size)
    }

    var origin = CGPoint.zero
    var size = popoverContext.presentedSize
    if !contentController.extendEdgeIntoArrow {
      if case .up = containerView.arrowDirection {
        origin.y = PopoverUX.arrowSize.height
      }
      size.height += PopoverUX.arrowSize.height
    }
    contentController.view.frame = CGRect(origin: origin, size: size)
    
    if let previewForOrigin, let snapshotView = previewForOrigin.view.snapshotView(afterScreenUpdates: false) {
      view.insertSubview(snapshotView, aboveSubview: backgroundOverlayView)
      let tapGesture = UITapGestureRecognizer(target: self, action: #selector(tappedOriginView))
      snapshotView.addGestureRecognizer(tapGesture)
      snapshotView.snp.makeConstraints {
        $0.edges.equalTo(originLayoutGuide)
      }
      snapshotView.isAccessibilityElement = true
      snapshotView.accessibilityTraits = [.button]
      snapshotView.accessibilityLabel = previewForOrigin.view.accessibilityLabel
      self.originSnapshotView = snapshotView
      
      if previewForOrigin.displaysFocusRings {
        let originFocusView: PopoverOriginFocusView = .init()
        view.insertSubview(originFocusView, belowSubview: snapshotView)
        originFocusView.snp.makeConstraints {
          $0.center.equalTo(originLayoutGuide)
          let length = max(previewForOrigin.view.bounds.width, previewForOrigin.view.bounds.height) + 8
          $0.size.equalTo(length)
        }
        originFocusView.layoutIfNeeded()
        self.originFocusView = originFocusView
      }
    }

    containerView.snp.makeConstraints {
      switch containerView.arrowDirection {
      case .down:
        $0.bottom.equalTo(originLayoutGuide.snp.top).offset(-arrowDistance)
      case .up:
        $0.top.equalTo(originLayoutGuide.snp.bottom).offset(arrowDistance)
      }
      $0.top.greaterThanOrEqualTo(self.view.safeAreaLayoutGuide.snp.top).offset(outerMargins.top)
      $0.bottom.lessThanOrEqualTo(self.view.safeAreaLayoutGuide.snp.bottom).offset(-outerMargins.bottom)
      $0.left.greaterThanOrEqualTo(self.view.safeAreaLayoutGuide.snp.left).offset(outerMargins.left)
      $0.right.lessThanOrEqualTo(self.view.safeAreaLayoutGuide.snp.right).offset(-outerMargins.right)
      $0.centerX.equalTo(originLayoutGuide).priority(.high)
    }

    backgroundOverlayView.alpha = 0.0
    originFocusView?.alpha = 0.0
    UIViewPropertyAnimator(duration: 0.2, curve: .linear) { [self] in
      backgroundOverlayView.alpha = 1.0
      originFocusView?.alpha = 1.0
    }
    .startAnimation()
    
    containerView.alpha = 0.0
    UIViewPropertyAnimator(duration: 0.2, dampingRatio: 1.0) { [self] in
      containerView.alpha = 1.0
    }
    .startAnimation()
    
    view.layoutIfNeeded()
    let translationDelta = anchorPointDelta(from: popoverContext, popoverRect: containerView.frame)

    containerView.arrowOrigin = CGPoint(x: containerView.bounds.midX + translationDelta.x, y: 0.0)

    containerView.transform = CGAffineTransform(translationX: translationDelta.x, y: translationDelta.y)
      .scaledBy(x: 0.001, y: 0.001)
      .translatedBy(x: -translationDelta.x, y: -translationDelta.y)

    UIView.animate(
      withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.7, initialSpringVelocity: 0, options: [.beginFromCurrentState, .allowUserInteraction],
      animations: {
        self.containerView.transform = .identity
      },
      completion: { _ in
        self.originFocusView?.beginAnimating()
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
          UIAccessibility.post(notification: .screenChanged, argument: self.contentController.view)
        }
      })
    context.completeTransition(true)
  }

  public func animateDismissal(context: UIViewControllerContextTransitioning) {
    guard let popoverContext = presentationContext else {
      context.completeTransition(false)
      return
    }
    
    self.originFocusView?.stopAnimating()

    UIViewPropertyAnimator(duration: 0.15, curve: .linear) {
      self.backgroundOverlayView.alpha = 0.0
    }
    .startAnimation()

    UIViewPropertyAnimator(duration: 0.3, dampingRatio: 1.0) {
      self.containerView.alpha = 0.0
    }
    .startAnimation()
    
    let oldTransform = containerView.transform

    containerView.transform = .identity  // Reset to get unaltered frame
    let translationDelta = anchorPointDelta(from: popoverContext, popoverRect: containerView.frame)
    containerView.transform = oldTransform  // Make sure to animate transform from a possibly altered transform

    UIView.animate(
      withDuration: 0.15, delay: 0, options: [.beginFromCurrentState, .allowUserInteraction],
      animations: {
        self.containerView.transform = CGAffineTransform(translationX: translationDelta.x, y: translationDelta.y)
          .scaledBy(x: 0.001, y: 0.001)
          .translatedBy(x: -translationDelta.x, y: -translationDelta.y)
      }
    ) { finished in
      context.completeTransition(finished)
    }
  }
}

// MARK: - UIViewControllerTransitioningDelegate
extension PopoverController: UIViewControllerTransitioningDelegate {

  public func animationController(forPresented presented: UIViewController, presenting: UIViewController, source: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: self, direction: .presenting)
  }

  public func animationController(forDismissed dismissed: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: self, direction: .dismissing)
  }
}

extension PopoverController: UIGestureRecognizerDelegate {

  public func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    return contentController.isPanToDismissEnabled
  }
}

extension PopoverController: UINavigationControllerDelegate {
  public func navigationController(_ navigationController: UINavigationController, willShow viewController: UIViewController, animated: Bool) {

    if case .preferredContentSize = contentSizeBehavior {
      var size = viewController.preferredContentSize
      size.width = min(size.width, UIScreen.main.bounds.width - outerMargins.left - outerMargins.right)
      size.height = min(size.height, UIScreen.main.bounds.height - containerView.frame.origin.y - view.safeAreaInsets.bottom - arrowDistance)
      navigationController.preferredContentSize = size
    }
  }
}

extension PopoverController {
  private class PopoverHostingController<Content>: UIHostingController<Content>, PopoverContentComponent where Content: View & PopoverContentComponent {
    weak var popoverController: PopoverController?
    
    override init(rootView: Content) {
      super.init(rootView: rootView)
#if compiler(>=5.8)
      if #available(iOS 16.4, *) {
        safeAreaRegions = []
      } else {
        disableSafeArea()
      }
#else
      disableSafeArea()
#endif
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
    
    override func viewDidLoad() {
      super.viewDidLoad()
      view.backgroundColor = .clear
    }
    
    override func viewDidLayoutSubviews() {
      super.viewDidLayoutSubviews()
      // For some reason these 2 calls are required in order for the `UIHostingController` to layout
      // correctly. Without this it for some reason becomes taller than what it needs to be despite its
      // `sizeThatFits(_:)` calls returning the correct value once the parent does layout.
      //
      // This is also required even on iOS 16 when the SwiftUI view changes its size presentation
      view.setNeedsUpdateConstraints()
      view.updateConstraintsIfNeeded()
      
      // Animates changes.
      UIViewPropertyAnimator(duration: 0.4, dampingRatio: 0.9) { [self] in
        popoverController?.view.layoutIfNeeded()
      }
      .startAnimation()
    }
    
    @available(iOS, introduced: 15.0, obsoleted: 16.4, message: "This hack is replaced by UIHostingController.safeAreaRegions")
    func disableSafeArea() {
      // https://gist.github.com/steipete/da72299613dcc91e8d729e48b4bb582c
      guard let viewClass = object_getClass(view) else { return }
      
      let viewSubclassName = String(cString: class_getName(viewClass)).appending("_IgnoreSafeArea")
      if let viewSubclass = NSClassFromString(viewSubclassName) {
        object_setClass(view, viewSubclass)
      } else {
        guard let viewClassNameUtf8 = (viewSubclassName as NSString).utf8String else { return }
        guard let viewSubclass = objc_allocateClassPair(viewClass, viewClassNameUtf8, 0) else { return }
        
        if let method = class_getInstanceMethod(UIView.self, #selector(getter: UIView.safeAreaInsets)) {
          let safeAreaInsets: @convention(block) (AnyObject) -> UIEdgeInsets = { _ in
            return .zero
          }
          class_addMethod(viewSubclass, #selector(getter: UIView.safeAreaInsets), imp_implementationWithBlock(safeAreaInsets), method_getTypeEncoding(method))
        }
        
        if let method2 = class_getInstanceMethod(viewClass, NSSelectorFromString("keyboardWillShowWithNotification:")) {
          let keyboardWillShow: @convention(block) (AnyObject, AnyObject) -> Void = { _, _ in }
          class_addMethod(viewSubclass, NSSelectorFromString("keyboardWillShowWithNotification:"), imp_implementationWithBlock(keyboardWillShow), method_getTypeEncoding(method2))
        }
        
        objc_registerClassPair(viewSubclass)
        object_setClass(view, viewSubclass)
      }
    }
    
    // Unfortunately due to UIHostingController bugs not syncing safe areas properly when SwiftUI changes
    // the size of the popover (except in iOS 15?), this is not supported in SwiftUI
    var extendEdgeIntoArrow: Bool {
      false
    }
    
    var isPanToDismissEnabled: Bool {
      rootView.isPanToDismissEnabled
    }
    
    func popoverShouldDismiss(_ popoverController: PopoverController) -> Bool {
      rootView.popoverShouldDismiss(popoverController)
    }
    
    var closeActionAccessibilityLabel: String {
      rootView.closeActionAccessibilityLabel
    }
    
    var popoverBackgroundColor: UIColor {
      rootView.popoverBackgroundColor
    }
  }
  
  /// Create a popover displaying a SwiftUI view hierarchy as its content
  public convenience init<Content: View & PopoverContentComponent>(
    content: Content,
    autoLayoutConfiguration: ContentSizeConfiguration? = nil
  ) {
    let contentController = PopoverHostingController(rootView: content)
    self.init(
      contentController: contentController,
      contentSizeBehavior: .autoLayout(autoLayoutConfiguration)
    )
    contentController.popoverController = self
  }
}
