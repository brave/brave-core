// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SnapKit
import pop
import BraveUI

public class SKUPurchaseView: UIView {
  
  enum ViewState {
    case overview
    case processing
    case complete
  }
  
  var gesturalDismissExecuted: (() -> Void)?
  
  let detailView = SKUPurchaseDetailsView()
  let buyButton = SKUPurchaseButtonView()
  
  var viewState: ViewState = .overview {
    didSet {
      switch (oldValue, viewState) {
      case (.overview, .processing):
        self.detailView.processingView.alpha = 0.0
        self.detailView.processingView.isHidden = false
        
        // For some reason, UIView animations glitch out, pop ones work fine
        if let constraint = self.heightConstraint?.layoutConstraints.first {
          constraint.springAnimate(property: kPOPLayoutConstraintConstant, key: "constant", { (animation, _) in
            animation.toValue = 300
          })
        }
        
        UIView.animate(withDuration: 0.15, animations: {
          self.buyButton.alpha = 0.0
          self.buyButton.transform = CGAffineTransform(translationX: 0, y: self.buyButton.bounds.height)
          self.detailView.headerView.alpha = 0.0
          self.detailView.bodyStackView.alpha = 0.001
          self.detailView.dismissButton.alpha = 0.0
          self.detailView.grabberView.alpha = 0.0
        }, completion: { _ in
          self.detailView.bodyStackView.removeFromSuperview()
          self.detailView.processingView.snp.makeConstraints {
            $0.bottom.equalTo(self.detailView.scrollView)
          }
          self.detailView.scrollView.contentInset = .zero
          UIView.animate(withDuration: 0.15, animations: {
            self.detailView.processingView.alpha = 1.0
          })
        })
        detailView.scrollView.isScrollEnabled = false
        detailView.processingView.loaderView.start()
      case (.processing, .complete):
        UIView.animate(withDuration: 0.15, animations: {
          self.detailView.processingView.alpha = 0.0
        }, completion: { _ in
          self.detailView.processingView.removeFromSuperview()
          self.detailView.completedView.alpha = 0.0
          self.detailView.completedView.isHidden = false
          self.detailView.completedView.snp.makeConstraints {
            $0.bottom.equalTo(self.detailView.scrollView)
          }
          self.detailView.scrollView.isScrollEnabled = true
          UIView.animate(withDuration: 0.15, animations: {
            self.detailView.completedView.alpha = 1.0
            self.detailView.dismissButton.alpha = 1.0
            self.detailView.grabberView.alpha = 1.0
          })
        })
      default:
        // Unsupported
        assertionFailure("Unsupported view state chage")
      }
    }
  }
  
  var isShowingInsufficientFundsView: Bool = false {
    didSet {
      buyButton.isHidden = isShowingInsufficientFundsView
      detailView.insufficientBalanceView.isHidden = !isShowingInsufficientFundsView
      if traitCollection.horizontalSizeClass != .regular {
        // Update height if needed
        updateForTraits()
      }
    }
  }
  
  // MARK: -
  
  private struct UX {
    static let overlayBackgroundColor = UIColor(white: 0.0, alpha: 0.5)
  }
  
  private let backgroundView = UIView().then {
    $0.backgroundColor = UX.overlayBackgroundColor
  }
  
  private let contentView = UIView().then {
    $0.backgroundColor = .clear
    $0.layer.shadowRadius = 4.0
    $0.layer.shadowOffset = CGSize(width: 0, height: 2)
    $0.layer.shadowOpacity = 0.35
  }
  
  private var contentSizeObvserver: NSKeyValueObservation?
  
  public override init(frame: CGRect) {
    super.init(frame: frame)
    
    contentView.clipsToBounds = true
    
    addSubview(backgroundView)
    addSubview(contentView)
    contentView.addSubview(detailView)
    contentView.addSubview(buyButton)
    
    detailView.scrollView.delegate = self
    contentSizeObvserver = detailView.scrollView.observe(\.contentSize, options: [.initial, .new], changeHandler: { [weak self] _, _ in
      self?.updateBuyButtonBackground()
    })
    
    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    buyButton.snp.makeConstraints {
      $0.bottom.leading.trailing.equalTo(contentView)
    }
    detailView.snp.makeConstraints {
      $0.edges.equalTo(contentView)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override public func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraits()
  }
  
  public override func layoutSubviews() {
    super.layoutSubviews()
    
    if viewState == .overview {
      var ci = detailView.scrollView.contentInset
      if isShowingInsufficientFundsView {
        ci.bottom = 0
      } else {
        ci.bottom = buyButton.systemLayoutSizeFitting(
          CGSize(width: bounds.size.width, height: 0),
          withHorizontalFittingPriority: .required,
          verticalFittingPriority: .fittingSizeLevel
        ).height
      }
      detailView.scrollView.contentInset = ci
    }
  }
  
  private var heightConstraint: Constraint?
  
  func updateForTraits() {
    if bounds.size.width == 0.0 {
      // Can't calculate height yet
      return
    }
    let isWideLayout = traitCollection.horizontalSizeClass == .regular

    contentView.snp.remakeConstraints {
      $0.top.greaterThanOrEqualTo(self.safeAreaLayoutGuide).offset(38.0)
      if isWideLayout {
        $0.center.equalTo(self)
        $0.width.equalTo(500.0)
        $0.bottom.lessThanOrEqualTo(self.safeAreaLayoutGuide).inset(38.0)
      } else {
        $0.leading.trailing.bottom.equalTo(self)
      }
      
      let boundingSize = CGSize(width: isWideLayout ? 500 : bounds.size.width, height: 0)
      if viewState == .overview {
        self.heightConstraint = $0.height.equalTo(
          [detailView.headerView, detailView.bodyStackView, isShowingInsufficientFundsView ? nil : buyButton]
            .compactMap { $0 }
            .reduce(0.0, {
              $0 + $1.systemLayoutSizeFitting(
                boundingSize,
                withHorizontalFittingPriority: .required,
                verticalFittingPriority: .fittingSizeLevel)
                .height
            }) + safeAreaInsets.bottom
        ).priority(.medium).constraint
      } else {
        self.heightConstraint = $0.height.equalTo(300).constraint
      }
    }
    
    contentView.layer.cornerRadius = isWideLayout ? 8.0 : 0.0
    contentView.layer.shadowOpacity = isWideLayout ? 0.35 : 0.0
  }
  
  private var isPassedDismissalThreshold = false
  private var isDismissingByGesture = false
}

extension SKUPurchaseView: BasicAnimationControllerDelegate {
  public func animatePresentation(context: UIViewControllerContextTransitioning) {
    context.containerView.addSubview(self)
    snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    frame = context.containerView.bounds
    let isWideLayout = context.containerView.traitCollection.horizontalSizeClass == .regular
    
    updateForTraits()
    
    // Prepare
    layoutIfNeeded()
    backgroundView.alpha = 0.0
    if isWideLayout {
      contentView.transform = CGAffineTransform(translationX: 0, y: bounds.height)
    } else {
      detailView.transform = CGAffineTransform(translationX: 0, y: bounds.height)
      buyButton.transform = CGAffineTransform(translationX: 0, y: buyButton.bounds.height)
    }
    
    // Animate
    UIView.animate(withDuration: 0.15) {
      self.backgroundView.alpha = 1.0
    }
    UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [], animations: {
      if isWideLayout {
        self.contentView.transform = .identity
      } else {
        self.detailView.transform = .identity
        self.buyButton.transform = .identity
      }
    })
    context.completeTransition(true)
  }
  public func animateDismissal(context: UIViewControllerContextTransitioning) {
    // Animate
    let isWideLayout = context.containerView.traitCollection.horizontalSizeClass == .regular
    UIView.animate(withDuration: 0.15) {
      self.backgroundView.alpha = 0.0
    }
    UIView.animate(withDuration: 0.35, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
      if isWideLayout {
        self.contentView.transform = CGAffineTransform(translationX: 0, y: self.bounds.height)
      } else {
        self.detailView.transform = CGAffineTransform(translationX: 0, y: self.bounds.height)
        self.buyButton.transform = CGAffineTransform(translationX: 0, y: self.buyButton.bounds.height)
      }
    }) { _ in
      self.removeFromSuperview()
      context.completeTransition(true)
    }
  }
}

extension SKUPurchaseView: UIScrollViewDelegate {
  
  private var isGesturalDismissEnabled: Bool {
    return self.traitCollection.horizontalSizeClass == .compact && viewState != .processing
  }
  
  private var dismissalThreshold: CGFloat {
    return -65.0
  }
  
  private func updateBuyButtonBackground() {
    let scrollView = detailView.scrollView
    let isScrollingUnderBuyButton = scrollView.contentOffset.y < -(scrollView.bounds.height - buyButton.bounds.height - scrollView.contentSize.height)
    if isScrollingUnderBuyButton && !buyButton.backgroundVisible {
      buyButton.backgroundVisible = true
    } else if !isScrollingUnderBuyButton && buyButton.backgroundVisible {
      buyButton.backgroundVisible = false
    }
  }
  
  public func scrollViewDidScroll(_ scrollView: UIScrollView) {
    updateBuyButtonBackground()
    
    // Don't adjust transform once its going to dismiss (dismiss animation will handle that) or if we're on iPad layout
    if !isGesturalDismissEnabled || isDismissingByGesture { return }
    
    let offset = -min(0, scrollView.contentOffset.y + scrollView.contentInset.top)
    detailView.transform = CGAffineTransform(translationX: 0, y: offset)
    // Offset change in overview transform with negative transform on the scroll view itself
    detailView.scrollView.transform = CGAffineTransform(translationX: 0, y: -offset)
    
    // Deceleration cannot trigger the dismissal, so we shouldn't update the dismiss button even if it does decelerate
    // temporarly passed the threshold (from a hard flick, for instance)
    if scrollView.isDecelerating { return }
    
    if scrollView.contentOffset.y + scrollView.contentInset.top < dismissalThreshold {
      if !isPassedDismissalThreshold {
        isPassedDismissalThreshold = true
        detailView.dismissButton.isHighlighted = true
        
        UIView.animate(withDuration: 0.1, delay: 0, usingSpringWithDamping: 0.8, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
          self.detailView.dismissButton.transform = CGAffineTransform(scaleX: 1.2, y: 1.2)
        }, completion: { _ in
          UIView.animate(withDuration: 0.2, delay: 0, usingSpringWithDamping: 0.8, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
            self.detailView.dismissButton.transform = .identity
          })
        })
        
        UIImpactFeedbackGenerator(style: .medium).bzzt()
      }
    } else {
      if isPassedDismissalThreshold {
        isPassedDismissalThreshold = false
        detailView.dismissButton.isHighlighted = false
      }
    }
  }
  
  public func scrollViewDidEndDragging(_ scrollView: UIScrollView, willDecelerate decelerate: Bool) {
    if isGesturalDismissEnabled && scrollView.contentOffset.y + scrollView.contentInset.top < dismissalThreshold {
      isDismissingByGesture = true
      self.gesturalDismissExecuted?()
    }
  }
}
