// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import Shared
import BraveUI
import BraveCore
import Combine

class WalletConnectionView: UIControl {
  private let scrollView = UIScrollView()

  private let stackView: UIStackView = {
    let result = UIStackView()
    result.axis = .horizontal
    result.spacing = 20
    result.alignment = .center
    result.isUserInteractionEnabled = false
    result.setContentCompressionResistancePriority(.required, for: .vertical)
    return result
  }()

  private let iconImageView: UIImageView = {
    let result = UIImageView(image: UIImage(braveSystemNamed: "leo.lock.open")!)
    result.tintColor = .white
    result.contentMode = .scaleAspectFit
    result.setContentHuggingPriority(.required, for: .horizontal)
    result.setContentCompressionResistancePriority(.required, for: .horizontal)
    result.setContentCompressionResistancePriority(.required, for: .vertical)
    return result
  }()

  private let titleLabel: UILabel = {
    let result = UILabel()
    result.textColor = .white
    result.font = .preferredFont(for: .subheadline, weight: .regular)
    result.adjustsFontForContentSizeCategory = false
    result.numberOfLines = 0
    result.text = Strings.Wallet.dappsConnectionNotificationTitle
    result.setContentCompressionResistancePriority(.required, for: .horizontal)
    result.setContentCompressionResistancePriority(.required, for: .vertical)
    return result
  }()
  
  private var cancellable: AnyCancellable?
  
  var origin: URLOrigin
  
  init(origin: URLOrigin) {
    self.origin = origin
    super.init(frame: .zero)
    setup()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
      fatalError()
  }
  
  private func setup() {
    let contentInset = 24.0
    addSubview(scrollView)
    scrollView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    
    scrollView.addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(contentInset)
    }
    stackView.addArrangedSubview(iconImageView)
    stackView.addArrangedSubview(titleLabel)
    
    scrollView.contentLayoutGuide.snp.makeConstraints {
      $0.width.equalToSuperview()
      $0.top.bottom.equalTo(stackView).inset(contentInset)
    }
    
    iconImageView.snp.makeConstraints {
      $0.width.height.equalTo(20)
    }
    
    snp.makeConstraints {
      $0.height.equalTo(stackView).inset(-contentInset).priority(.low)
    }

    layer.backgroundColor = UIColor.braveBlurpleTint.cgColor
    layer.cornerRadius = 10
    
    titleLabel.attributedText = titleText(for: origin)
    updateAccessibilityTraits()
    
    // font size adjustment with a maximum content size category of .medium
    cancellable = NotificationCenter.default
      .publisher(for: UIContentSizeCategory.didChangeNotification, object: nil)
      .sink { [weak self] _ in
        guard let self = self else { return }
        self.titleLabel.attributedText = self.titleText(for: self.origin)
        self.updateAccessibilityTraits()
      }
  }
  
  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    guard superview != nil else { return }
    snp.makeConstraints {
      $0.height.lessThanOrEqualToSuperview().dividedBy(2)
    }
  }
  
  private func font(forTextStyle textStyle: UIFont.TextStyle, weight: UIFont.Weight) -> UIFont {
    var sizeCategory = UIApplication.shared.preferredContentSizeCategory
    if sizeCategory.isAccessibilityCategory { // set a maximum size category
      sizeCategory = .accessibilityMedium
    }
    let traitCollection = UITraitCollection(preferredContentSizeCategory: sizeCategory)
    return UIFont.preferredFont(for: textStyle, weight: weight, traitCollection: traitCollection)
  }
  
  private func titleText(for origin: URLOrigin) -> NSAttributedString {
    let regularFont = self.font(forTextStyle: .subheadline, weight: .regular)
    guard let originString = origin.url?.host else {
      return NSAttributedString(
        string: Strings.Wallet.dappsConnectionNotificationTitle,
        attributes: [.font: regularFont]
      )
    }
    let emphasisedFont = self.font(forTextStyle: .subheadline, weight: .bold)

    if let originEtldPlusOne = origin.url?.baseDomain {
      // eTLD+1 available, bold it
      let displayString = String.localizedStringWithFormat(Strings.Wallet.dappsConnectionNotificationOriginTitle, originString)
      let rangeForEldPlusOne = (displayString as NSString).range(of: originEtldPlusOne)
      let string = NSMutableAttributedString(
        string: displayString,
        attributes: [.font: regularFont]
      )
      string.setAttributes([.font: emphasisedFont], range: rangeForEldPlusOne)
      return string
    } else {
      // eTLD+1 unavailable
      let displayString = String.localizedStringWithFormat(Strings.Wallet.dappsConnectionNotificationOriginTitle, originString)
      return NSAttributedString(
        string: displayString,
        attributes: [.font: regularFont]
      )
    }
  }
  
  private func updateAccessibilityTraits() {
    isAccessibilityElement = true
    accessibilityLabel = titleLabel.text
    accessibilityTraits = [.staticText, .button]
    titleLabel.isAccessibilityElement = false
  }
}
