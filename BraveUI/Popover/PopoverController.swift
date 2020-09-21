/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import pop
import SnapKit

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
        case autoLayout
        /// The popover will size itself based on `UIViewController.preferredContentSize`
        case preferredContentSize
        /// The popover content view will be fixed to a given size
        case fixedSize(CGSize)
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
    public var arrowDistance: CGFloat = -5.0
    
    /// The arrow direction behavior for this popover
    public var arrowDirectionBehavior: ArrowDirectionBehavior = .automatic
    
    /// Whether or not to automatically dismiss the popup when the device orientation changes
    var dismissesOnOrientationChanged = true
    
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
    public init(contentController: UIViewController & PopoverContentComponent, contentSizeBehavior: ContentSizeBehavior = .autoLayout) {
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
    
    override public func viewDidLoad() {
        super.viewDidLoad()
        
        backgroundOverlayView.backgroundColor = UIColor(white: 0.0, alpha: 0.2)
        let backgroundTap = UITapGestureRecognizer(target: self, action: #selector(tappedBackgroundOverlay(_:)))
        backgroundOverlayView.isAccessibilityElement = true
        backgroundOverlayView.accessibilityLabel = contentController.closeActionAccessibilityLabel
        backgroundOverlayView.accessibilityElements = [backgroundTap]
        backgroundOverlayView.addGestureRecognizer(backgroundTap)
        
        let pan = UIPanGestureRecognizer(target: self, action: #selector(pannedPopover(_:)))
        pan.delegate = self
        view.addGestureRecognizer(pan)
        
        containerView.translatesAutoresizingMaskIntoConstraints = false
        
        view.addSubview(backgroundOverlayView)
        view.addSubview(containerView)
        
        backgroundOverlayView.snp.makeConstraints { make in
            make.edges.equalTo(self.view)
        }
        
        addChild(contentController)
        containerView.contentView.addSubview(contentController.view)
        contentController.didMove(toParent: self)
        
        switch contentSizeBehavior {
        case .autoLayout:
            contentController.view.snp.makeConstraints { make in
                make.edges.equalTo(self.containerView.contentView)
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
        return .lightContent
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
            // Layout handled through constraints
            break
        }
    }
    
    // MARK: - UI
    
    private(set) var contentController: UIViewController & PopoverContentComponent
    
    private let containerView = ContainerView()
    
    private let backgroundOverlayView = UIView()
    
    override public func preferredContentSizeDidChange(forChildContentContainer container: UIContentContainer) {
        if case .preferredContentSize = contentSizeBehavior {
            var size = contentController.preferredContentSize
            if size == .zero {
                // Do nothing, keep it whatever it is currently
                return
            }
            
            size.width = min(size.width, UIScreen.main.bounds.width - outerMargins.left - outerMargins.right)
            size.height = min(size.height, UIScreen.main.bounds.height - containerView.frame.origin.y - view.safeAreaInsets.bottom - arrowDistance)
            if contentController.view.bounds.size == size { return }
            
            contentController.view.setNeedsLayout()
            
            if let nc = self.contentController as? UINavigationController {
                nc.viewControllers.filter { $0.isViewLoaded }.forEach { vc in
                    var frame = vc.view.frame
                    frame.size.height = size.height + PopoverUX.arrowSize.height
                    vc.view.frame = frame
                }
            }
            self.containerViewHeightConstraint?.springAnimate(property: kPOPLayoutConstraintConstant, key: "constant") { animation, _ in
                animation.toValue = size.height + PopoverUX.arrowSize.height
                animation.animationDidApplyBlock = { _ in
                    if let nc = self.contentController as? UINavigationController {
                        nc.viewControllers.filter { $0.isViewLoaded }.forEach { $0.view.frame = nc.view.bounds }
                    }
                    if let nc = self.contentController as? PopoverNavigationController {
                        nc.lastPoppedController?.view.frame = nc.view.bounds
                    }
                }
            }
            self.containerViewWidthConstraint?.springAnimate(property: kPOPLayoutConstraintConstant, key: "constant") { animation, _ in
                animation.toValue = size.width
            }
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
    public func present(from view: UIView, on viewController: UIViewController) {
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
        
        let constrainedWidth = viewController.view.bounds.width - outerMargins.left - outerMargins.right
        let contentSize: CGSize
        
        switch contentSizeBehavior {
        case .autoLayout:
            contentSize = contentController.view.systemLayoutSizeFitting(CGSize(width: constrainedWidth, height: viewController.view.bounds.height - outerMargins.top - outerMargins.bottom))
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
        
        viewController.present(self, animated: true)
    }
    
    override public func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
        
        if dismissesOnOrientationChanged {
            dismiss(animated: true)
        }
    }
}

// MARK: - Actions
extension PopoverController {
    
    @objc private func tappedBackgroundOverlay(_ tap: UITapGestureRecognizer) {
        if tap.state == .ended {
            if contentController.popoverShouldDismiss(self) {
                dismiss(animated: true)
                // Not sure if we want this after dismissal completes or right away. Could always create a
                // `popoverWillDismiss` to put before and `did` after
                popoverDidDismiss?(self)
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
        let rotationPercent: CGFloat
        
        switch containerView.arrowDirection {
        case .up:
            scale = 1.0 - (-pan.translation(in: pan.view).y / containerView.bounds.height)
            rotationPercent = -pan.translation(in: pan.view).x / containerView.bounds.width
        case .down:
            scale = 1.0 - pan.translation(in: pan.view).y / containerView.bounds.height
            rotationPercent = pan.translation(in: pan.view).x / containerView.bounds.width
        }
        
        scale = max(0.0, scale)
        if scale > 1 {
            scale = 1.0 + _computedOffsetBasedOnRubberBandingResistance(
                distance: scale - 1.0,
                constant: 0.15,
                dimension: containerView.bounds.height
            )
        }
        
        let rotation = _computedOffsetBasedOnRubberBandingResistance(
            distance: rotationPercent * (CGFloat.pi / 2.0),
            constant: 0.4,
            dimension: containerView.bounds.width
        )
        
        containerView.transform = .identity // Reset to get unaltered frame
        let translationDelta = anchorPointDelta(from: context, popoverRect: containerView.frame)
        
        containerView.transform = CGAffineTransform(translationX: translationDelta.x, y: translationDelta.y)
            .scaledBy(x: scale, y: scale)
            .rotated(by: rotation)
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
                dismiss(animated: true)
            } else {
                UIView.animate(withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.7, initialSpringVelocity: 0, options: [.beginFromCurrentState, .allowUserInteraction], animations: {
                    self.containerView.transform = .identity
                })
            }
        }
        
        if pan.state == .cancelled {
            UIView.animate(withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.7, initialSpringVelocity: 0, options: [.beginFromCurrentState, .allowUserInteraction], animations: {
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
        backgroundOverlayView.basicAnimate(property: kPOPViewAlpha, key: "alpha") { animation, _ in
            animation.toValue = 1.0
            animation.duration = 0.3
        }
        
        containerView.alpha = 0.0
        containerView.springAnimate(property: kPOPViewAlpha, key: "alpha") { animation, inProgress in
            animation.toValue = 1.0
            animation.springSpeed = 16.0
            animation.springBounciness = 6.0
            animation.clampMode = POPAnimationClampFlags.end.rawValue
        }
        
        view.layoutIfNeeded()
        let translationDelta = anchorPointDelta(from: popoverContext, popoverRect: containerView.frame)
        
        containerView.arrowOrigin = CGPoint(x: containerView.bounds.midX + translationDelta.x, y: 0.0)
        
        containerView.transform = CGAffineTransform(translationX: translationDelta.x, y: translationDelta.y)
            .scaledBy(x: 0.001, y: 0.001)
            .translatedBy(x: -translationDelta.x, y: -translationDelta.y)
        
        UIView.animate(withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.7, initialSpringVelocity: 0, options: [.beginFromCurrentState, .allowUserInteraction], animations: {
            self.containerView.transform = .identity
        })
        context.completeTransition(true)
    }
    
    public func animateDismissal(context: UIViewControllerContextTransitioning) {
        guard let popoverContext = presentationContext else {
            context.completeTransition(false)
            return
        }
        
        backgroundOverlayView.basicAnimate(property: kPOPViewAlpha, key: "alpha") { animation, _ in
            animation.toValue = 0.0
            animation.duration = 0.15
        }
        
        containerView.springAnimate(property: kPOPViewAlpha, key: "alpha") { animation, inProgress in
            animation.toValue = 0.0
            animation.springSpeed = 16.0
            animation.springBounciness = 6.0
            animation.clampMode = POPAnimationClampFlags.end.rawValue
        }
        
        let oldTransform = containerView.transform
        let rotationAngle = atan2(oldTransform.b, oldTransform.a)
        
        containerView.transform = .identity // Reset to get unaltered frame
        let translationDelta = anchorPointDelta(from: popoverContext, popoverRect: containerView.frame)
        containerView.transform = oldTransform // Make sure to animate transform from a possibly altered transform
        
        UIView.animate(withDuration: 0.15, delay: 0, options: [.beginFromCurrentState, .allowUserInteraction], animations: {
            self.containerView.transform = CGAffineTransform(translationX: translationDelta.x, y: translationDelta.y)
                .scaledBy(x: 0.001, y: 0.001)
                .rotated(by: rotationAngle)
                .translatedBy(x: -translationDelta.x, y: -translationDelta.y)
        }) { finished in
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
