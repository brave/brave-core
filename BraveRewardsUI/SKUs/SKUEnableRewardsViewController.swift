// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import BraveUI

/// Popup shown to enable rewards if the user doesn't have it enabled and they
/// tap a BUY with BAT button
public class SKUEnableRewardsViewController: UIViewController, UIViewControllerTransitioningDelegate {
  
  private let rewards: BraveRewards
  private let termsURLTapped: () -> Void
  
  public init(rewards: BraveRewards, termsURLTapped: @escaping () -> Void) {
    self.rewards = rewards
    self.termsURLTapped = termsURLTapped
    super.init(nibName: nil, bundle: nil)
    
    modalPresentationStyle = .overCurrentContext
    if #available(iOS 13.0, *) {
      isModalInPresentation = true
    }
    transitioningDelegate = self
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  var enableRewardsView: SKUEnableRewardsView {
    return view as! SKUEnableRewardsView // swiftlint:disable:this force_cast
  }
  
  public override func loadView() {
    view = SKUEnableRewardsView()
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    
    enableRewardsView.backgroundView.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(tappedBackground)))
    enableRewardsView.enableRewardsButton.addTarget(self, action: #selector(tappedEnableRewards), for: .touchUpInside)
    enableRewardsView.termsLabel.onLinkedTapped = { [weak self] _ in
      self?.termsURLTapped()
    }
  }
  
  @objc private func tappedBackground() {
    dismiss(animated: true)
  }
  
  @objc private func tappedEnableRewards() {
    rewards.ledger.createWalletAndFetchDetails({ _ in })
    dismiss(animated: true)
  }
  
  override public var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    if UIDevice.current.userInterfaceIdiom == .phone {
      return .portrait
    }
    return .all
  }
  
  // MARK: - UIViewControllerTransitioningDelegate
  
  public func animationController(forPresented presented: UIViewController, presenting: UIViewController, source: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: self, direction: .presenting)
  }
  
  public func animationController(forDismissed dismissed: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: self, direction: .dismissing)
  }
}

extension SKUEnableRewardsViewController: BasicAnimationControllerDelegate {
  public func animatePresentation(context: UIViewControllerContextTransitioning) {
    context.containerView.addSubview(view)
    view.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    view.frame = context.containerView.bounds
    
    // Prepare
    view.layoutIfNeeded()
    enableRewardsView.backgroundView.alpha = 0.0
    enableRewardsView.containerView.transform = CGAffineTransform(translationX: 0, y: view.bounds.height - enableRewardsView.containerView.bounds.height)
    
    // Animate
    UIView.animate(withDuration: 0.15) {
      self.enableRewardsView.backgroundView.alpha = 1.0
    }
    UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 0.85, initialSpringVelocity: 0, options: [], animations: {
      self.enableRewardsView.containerView.transform = .identity
    }, completion: nil)
    context.completeTransition(true)
  }
  public func animateDismissal(context: UIViewControllerContextTransitioning) {
    // Animate
    UIView.animate(withDuration: 0.15) {
      self.enableRewardsView.backgroundView.alpha = 0.0
    }
    UIView.animate(withDuration: 0.35, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
      self.enableRewardsView.containerView.transform = CGAffineTransform(translationX: 0, y: self.view.bounds.height - self.enableRewardsView.containerView.frame.minY)
    }) { _ in
      self.view.removeFromSuperview()
      context.completeTransition(true)
    }
  }
}

public class SKUEnableRewardsView: UIView {
  
  let backgroundView = UIView().then {
    $0.backgroundColor = UIColor(white: 0.0, alpha: 0.3)
  }
  
  let containerView = UIView().then {
    $0.backgroundColor = .white
    $0.layer.cornerRadius = 12
    $0.clipsToBounds = true
  }
  
  let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 12.0
  }
  
  let titleLabel = UILabel().then {
    $0.text = Strings.SKUEnableRewardsTitle
    $0.textAlignment = .center
    $0.appearanceTextColor = Colors.grey900
    $0.font = .systemFont(ofSize: 18, weight: .semibold)
    $0.numberOfLines = 0
  }
  
  let bodyLabel = UILabel().then {
    $0.text = Strings.SKUEnableRewardsBody
    $0.textAlignment = .center
    $0.appearanceTextColor = Colors.grey900
    $0.font = .systemFont(ofSize: 15)
    $0.numberOfLines = 0
  }
  
  let enableRewardsButton = FilledActionButton(type: .system).then {
    $0.appearanceBackgroundColor = Colors.blurple500
    $0.setTitle(String(format: Strings.SKUEnableRewardsButtonTitle), for: .normal)
    $0.titleLabel?.font = .systemFont(ofSize: 16.0, weight: .semibold)
  }
  
  let termsLabel = LinkLabel().then {
    $0.textAlignment = .center
    $0.font = .systemFont(ofSize: 13)
    $0.textColor = Colors.grey900
    $0.linkColor = BraveUX.braveOrange
    $0.text = Strings.SKUEnableRewardsDisclaimer
    $0.setURLInfo([Strings.SKUEnableRewardsDisclaimerLink: "terms"])
  }
  
  public override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(backgroundView)
    addSubview(containerView)
    containerView.addSubview(stackView)
    
    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    containerView.snp.makeConstraints {
      $0.top.leading.greaterThanOrEqualTo(self).inset(20)
      $0.bottom.trailing.lessThanOrEqualTo(self).inset(20)
      $0.center.equalTo(self)
      $0.width.lessThanOrEqualTo(400)
    }
    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(25)
    }
    enableRewardsButton.snp.makeConstraints {
      $0.height.equalTo(44)
    }
    
    stackView.addStackViewItems(
      .view(titleLabel),
      .view(bodyLabel),
      .customSpace(28),
      .view(enableRewardsButton),
      .customSpace(16),
      .view(termsLabel)
    )
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
