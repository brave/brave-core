/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class TippingOverviewView: UIView {
  private struct UX {
    static let backgroundColor = Colors.blurple100
    static let headerBackgroundColor = Colors.grey600
    static let headerHeight: CGFloat = 98.0
    static let faviconBackgroundColor = Colors.neutral100
    static let faviconSize = CGSize(width: 88.0, height: 88.0)
    static let titleColor = Colors.grey800
    static let bodyColor = Colors.grey700
  }
  
  let dismissButton = DismissButton().then {
    $0.layer.borderWidth = 1.0 / UIScreen.main.scale
    $0.layer.borderColor = UIColor.black.withAlphaComponent(0.4).cgColor
  }
  
  let headerView = UIImageView().then {
    $0.backgroundColor = UX.headerBackgroundColor
    $0.contentMode = .scaleAspectFill
    $0.clipsToBounds = true
  }
  
  let grabberView = GrabberView(style: .dark)
  
  let watermarkImageView = UIImageView(image: UIImage(frameworkResourceNamed: "tipping-bat-watermark"))
  
  let heartsImageView = UIImageView(image: UIImage(frameworkResourceNamed: "hearts"))
  
  let faviconImageView = PublisherIconCircleImageView(size: UX.faviconSize.width, inset: 14).then {
    $0.backgroundColor = UX.faviconBackgroundColor
    $0.layer.borderColor = UIColor.white.cgColor
    $0.layer.borderWidth = 2.0
  }
  
  let verifiedImageView = UIImageView(image: UIImage(frameworkResourceNamed: "icn-verified-large")).then {
    $0.isHidden = true
  }
  
  let publisherNameLabel = UILabel().then {
    $0.appearanceTextColor = .white
    $0.font = .systemFont(ofSize: 20.0, weight: .medium)
  }
  
  let socialStackView = UIStackView().then {
    $0.spacing = 20.0
    // Hide these icons until a later date (ref: https://github.com/brave/brave-ios/issues/2147) when we
    // can make the icons actual buttons that lead to their social media sites (ref:
    // https://github.com/brave/brave-ios/issues/1712)
    $0.isHidden = true
  }
  
  private let bodyStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 10.0
  }
  
  let disclaimerView = LinkLabel().then {
    $0.appearanceTextColor = Colors.grey700
    $0.font = UIFont.systemFont(ofSize: 12.0)
    $0.textContainerInset = UIEdgeInsets(top: 8.0, left: 8.0, bottom: 8.0, right: 8.0)
    $0.text = "\(Strings.tippingUnverifiedDisclaimer) \(Strings.disclaimerLearnMore)"
    $0.setURLInfo([Strings.disclaimerLearnMore: "learn-more"])
    $0.backgroundColor = .white
    $0.layer.cornerRadius = 4.0
    $0.isHidden = true
  }
  
  let titleLabel = UILabel().then {
    $0.text = Strings.tippingOverviewTitle
    $0.appearanceTextColor = UX.titleColor
    $0.font = .systemFont(ofSize: 23.0, weight: .semibold)
    $0.numberOfLines = 0
  }
  
  let bodyLabel = UILabel().then {
    $0.text = Strings.tippingOverviewBody 
    $0.appearanceTextColor = UX.bodyColor
    $0.font = .systemFont(ofSize: 17.0)
    $0.numberOfLines = 0
  }
  
  let scrollView = UIScrollView().then {
    $0.alwaysBounceVertical = true
    $0.showsVerticalScrollIndicator = false
    $0.contentInset = UIEdgeInsets(top: UX.headerHeight, left: 0.0, bottom: 0.0, right: 0.0)
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = UX.backgroundColor
    
    clipsToBounds = true
    layer.cornerRadius = 8.0
    layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    
    addSubview(scrollView)
    addSubview(headerView)
    headerView.addSubview(watermarkImageView)
    headerView.addSubview(grabberView)
    headerView.addSubview(publisherNameLabel)
    addSubview(dismissButton)
    // headerView.addSubview(heartsImageView)
    scrollView.addSubview(socialStackView)
    addSubview(faviconImageView)
    addSubview(verifiedImageView)
    scrollView.addSubview(bodyStackView)
    bodyStackView.addArrangedSubview(disclaimerView)
    bodyStackView.setCustomSpacing(15.0, after: disclaimerView)
    bodyStackView.addArrangedSubview(titleLabel)
    bodyStackView.addArrangedSubview(bodyLabel)
    
    scrollView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    scrollView.contentLayoutGuide.snp.makeConstraints {
      $0.width.equalTo(self)
    }
    headerView.snp.makeConstraints {
      $0.top.leading.trailing.equalTo(self)
      $0.height.equalTo(UX.headerHeight)
    }
    dismissButton.snp.makeConstraints {
      $0.top.trailing.equalToSuperview().inset(8.0)
    }
    watermarkImageView.snp.makeConstraints {
      $0.top.leading.equalTo(self.headerView)
    }
    grabberView.snp.makeConstraints {
      $0.centerX.equalTo(self)
      $0.top.equalTo(self).offset(5.0)
    }
    faviconImageView.snp.makeConstraints {
      $0.centerY.equalTo(self.headerView.snp.bottom).offset(-8.0)
      $0.leading.equalTo(self).offset(25.0)
      $0.size.equalTo(UX.faviconSize)
    }
    verifiedImageView.snp.makeConstraints {
      $0.centerX.equalTo(faviconImageView.snp.trailing).offset(-11)
      $0.centerY.equalTo(faviconImageView.snp.top).offset(11)
    }
    publisherNameLabel.snp.makeConstraints {
      $0.leading.equalTo(faviconImageView.snp.trailing).offset(15)
      $0.bottom.equalTo(headerView).inset(10)
      $0.trailing.equalTo(headerView).inset(15)
    }
    socialStackView.snp.makeConstraints {
      $0.top.equalTo(self.scrollView.contentLayoutGuide).offset(20.0)
      $0.trailing.equalTo(self).offset(-20.0)
    }
    bodyStackView.snp.makeConstraints {
      $0.top.equalTo(self.socialStackView.snp.bottom).offset(25.0)
      $0.leading.trailing.equalTo(self).inset(25.0)
      $0.bottom.equalTo(self.scrollView.contentLayoutGuide).inset(25.0)
    }
    updateForTraits()
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraits()
  }
  
  func updateForTraits() {
    grabberView.isHidden = traitCollection.horizontalSizeClass == .regular
  }
}
