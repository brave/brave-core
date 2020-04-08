/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards
import BraveUI

extension TippingViewController {
  
  /// `BraveRewardsTippingViewController`'s loaded view
  class View: UIView {
    
    var gesturalDismissExecuted: (() -> Void)?
    
    func updateConfirmationInfo(name: String, tipAmount: Double, recurringDate: String?) {
      let isMonthly = optionSelectionView.isMonthly
      
      confirmationView.faviconImageView.image = overviewView.faviconImageView.image
      confirmationView.faviconImageView.backgroundColor = overviewView.faviconImageView.backgroundColor
      confirmationView.subtitleLabel.text = isMonthly ? Strings.tippingMonthlyTitle : Strings.tippingOneTimeTitle
      
      confirmationView.infoLabel.text = "\(name)\n\(tipAmount) \(Strings.BAT)\(isMonthly ? ", \(Strings.tippingRecurring)" : "")"
      
      if isMonthly, let recurringDate = recurringDate {
        confirmationView.monthlyTipLabel.attributedText = {
          let paragraphStyle = NSMutableParagraphStyle()
          paragraphStyle.alignment = .center
          paragraphStyle.lineBreakMode = .byWordWrapping
          paragraphStyle.lineSpacing = 8.0
          
          let text = NSMutableAttributedString(string: "\(Strings.tippingRecurringDetails)\n", attributes: [
            .font: UIFont.systemFont(ofSize: 14.0, weight: .medium),
            .foregroundColor: Colors.grey300
          ])
          
          text.append(NSAttributedString(string: recurringDate, attributes: [
            .font: UIFont.systemFont(ofSize: 14.0, weight: .medium),
            .foregroundColor: Colors.orange500
          ]))
          
          text.addAttribute(.paragraphStyle, value: paragraphStyle, range: NSRange(location: 0, length: text.length))
          return text
        }()
      }
      
      confirmationView.monthlyTipLabel.isHidden = !isMonthly
    }
    
    func setTippingConfirmationVisible(_ visible: Bool, animated: Bool = true) {
      if confirmationView.isHidden == !visible {
        // nothing to do
        return
      }
      
      if visible {
        addSubview(confirmationView)
        confirmationView.isHidden = false
        confirmationView.snp.makeConstraints {
          $0.edges.equalTo(self)
        }
        if animated {
          confirmationView.stackView.transform = CGAffineTransform(translationX: 0, y: bounds.height)
          confirmationView.backgroundColor = TippingConfirmationView.UX.backgroundColor.withAlphaComponent(0.0)
          
          UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
            self.confirmationView.backgroundColor = TippingConfirmationView.UX.backgroundColor
          }, completion: nil)
          UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 0.7, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
            self.confirmationView.stackView.transform = .identity
          }, completion: nil)
        }
      } else {
        if animated {
          UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
            self.confirmationView.backgroundColor = TippingConfirmationView.UX.backgroundColor.withAlphaComponent(0.0)
          }, completion: nil)
          UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 0.7, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
            self.confirmationView.stackView.transform = CGAffineTransform(translationX: 0, y: self.bounds.height)
          }, completion: { _ in
            self.confirmationView.isHidden = true
            self.confirmationView.removeFromSuperview()
          })
        } else {
          self.confirmationView.isHidden = true
          self.confirmationView.removeFromSuperview()
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
    
    private let confirmationView = TippingConfirmationView().then {
      $0.isHidden = true
    }
    
    private let contentView = UIView().then {
      $0.backgroundColor = .clear
      $0.layer.shadowRadius = 4.0
      $0.layer.shadowOffset = CGSize(width: 0, height: 2)
      $0.layer.shadowOpacity = 0.35
    }
    
    let overviewView = TippingOverviewView()
    
    let optionSelectionView = TippingSelectionView().then {
      // iPhone only... probably
      $0.layer.shadowRadius = 8.0
      $0.layer.shadowOffset = CGSize(width: 0, height: -2)
      $0.layer.shadowOpacity = 0.35
      $0.layer.maskedCorners = [.layerMinXMaxYCorner, .layerMaxXMaxYCorner]
    }
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      addSubview(backgroundView)
      addSubview(contentView)
      contentView.addSubview(overviewView)
      contentView.addSubview(optionSelectionView)
      
      overviewView.scrollView.delegate = self
      
      backgroundView.snp.makeConstraints {
        $0.edges.equalTo(self)
      }
      optionSelectionView.snp.makeConstraints {
        $0.bottom.leading.trailing.equalTo(contentView)
      }
      overviewView.snp.makeConstraints {
        $0.top.leading.trailing.equalTo(contentView)
        $0.bottom.equalTo(self.optionSelectionView.snp.top)
      }
      
      updateForTraits()
    }
    
    func updateForTraits() {
      let isWideLayout = traitCollection.horizontalSizeClass == .regular
      contentView.snp.remakeConstraints {
        if isWideLayout {
          $0.center.equalTo(self)
          $0.width.equalTo(500.0)
          $0.height.equalTo(600.0)
        } else {
          $0.top.equalTo(self.safeAreaLayoutGuide).offset(38.0)
          $0.leading.trailing.bottom.equalTo(self)
        }
      }
      contentView.layer.cornerRadius = isWideLayout ? 8.0 : 0.0
      optionSelectionView.layer.cornerRadius = contentView.layer.cornerRadius
      contentView.layer.shadowOpacity = isWideLayout ? 0.35 : 0.0
      optionSelectionView.clipsToBounds = isWideLayout
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
      super.traitCollectionDidChange(previousTraitCollection)
      updateForTraits()
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
    
    private var isPassedDismissalThreshold = false
    private var isDismissingByGesture = false
  }
}

extension TippingViewController.View: BasicAnimationControllerDelegate {
  func animatePresentation(context: UIViewControllerContextTransitioning) {
    context.containerView.addSubview(self)
    snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    frame = context.containerView.bounds
    let isWideLayout = context.containerView.traitCollection.horizontalSizeClass == .regular
    
    // Prepare
    layoutIfNeeded()
    backgroundView.alpha = 0.0
    if isWideLayout {
      contentView.transform = CGAffineTransform(translationX: 0, y: bounds.height)
    } else {
      overviewView.transform = CGAffineTransform(translationX: 0, y: bounds.height)
      optionSelectionView.transform = CGAffineTransform(translationX: 0, y: optionSelectionView.bounds.height)
    }
    
    // Animate
    UIView.animate(withDuration: 0.15) {
      self.backgroundView.alpha = 1.0
    }
    UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [], animations: {
      if isWideLayout {
        self.contentView.transform = .identity
      } else {
        self.overviewView.transform = .identity
        self.optionSelectionView.transform = .identity
      }
    }, completion: nil)
    context.completeTransition(true)
  }
  func animateDismissal(context: UIViewControllerContextTransitioning) {
    // Animate
    let isWideLayout = context.containerView.traitCollection.horizontalSizeClass == .regular
    UIView.animate(withDuration: 0.15) {
      self.backgroundView.alpha = 0.0
    }
    UIView.animate(withDuration: 0.35, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
      if isWideLayout {
        self.contentView.transform = CGAffineTransform(translationX: 0, y: self.bounds.height)
      } else {
        self.overviewView.transform = CGAffineTransform(translationX: 0, y: self.bounds.height)
        self.optionSelectionView.transform = CGAffineTransform(translationX: 0, y: self.optionSelectionView.bounds.height)
      }
    }) { _ in
      self.removeFromSuperview()
      context.completeTransition(true)
    }
    setTippingConfirmationVisible(false, animated: true)
  }
}

extension TippingViewController.View: UIScrollViewDelegate {
  
  private var isGesturalDismissEnabled: Bool {
    return self.traitCollection.horizontalSizeClass == .compact
  }
  
  private var dismissalThreshold: CGFloat {
    return -65.0
  }
  
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    // Don't adjust transform once its going to dismiss (dismiss animation will handle that) or if we're on iPad layout
    if !isGesturalDismissEnabled || isDismissingByGesture { return }
    
    let offset = -min(0, scrollView.contentOffset.y + scrollView.contentInset.top)
    overviewView.transform = CGAffineTransform(translationX: 0, y: offset)
    // Offset change in overview transform with negative transform on the scroll view itself
    overviewView.scrollView.transform = CGAffineTransform(translationX: 0, y: -offset)
    
    // Deceleration cannot trigger the dismissal, so we shouldn't update the dismiss button even if it does decelerate
    // temporarly passed the threshold (from a hard flick, for instance)
    if scrollView.isDecelerating { return }
    
    if scrollView.contentOffset.y + scrollView.contentInset.top < dismissalThreshold {
      if !isPassedDismissalThreshold {
        isPassedDismissalThreshold = true
        overviewView.dismissButton.isHighlighted = true
        
        UIView.animate(withDuration: 0.1, delay: 0, usingSpringWithDamping: 0.8, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
          self.overviewView.dismissButton.transform = CGAffineTransform(scaleX: 1.2, y: 1.2)
        }, completion: { _ in
          UIView.animate(withDuration: 0.2, delay: 0, usingSpringWithDamping: 0.8, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
            self.overviewView.dismissButton.transform = .identity
          }, completion: nil)
        })
        
        UIImpactFeedbackGenerator(style: .medium).bzzt()
      }
    } else {
      if isPassedDismissalThreshold {
        isPassedDismissalThreshold = false
        overviewView.dismissButton.isHighlighted = false
      }
    }
  }
  
  func scrollViewDidEndDragging(_ scrollView: UIScrollView, willDecelerate decelerate: Bool) {
    if isGesturalDismissEnabled && scrollView.contentOffset.y + scrollView.contentInset.top < dismissalThreshold {
      isDismissingByGesture = true
      self.gesturalDismissExecuted?()
    }
  }
}
