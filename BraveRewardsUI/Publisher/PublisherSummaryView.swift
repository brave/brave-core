/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class PublisherSummaryView: UIView {
  
  var autoContributeChanged: ((Bool) -> Void)?
  
  func setAutoContribute(enabled: Bool) {
    autoContributeRow.toggleSwitch.isOn = enabled
  }
  
  lazy var monthlyTipView = DetailActionableRow().then {
    $0.textLabel.text = Strings.tipSiteMonthly
  }
  
  private let scrollView = UIScrollView().then {
    $0.contentInsetAdjustmentBehavior = .never
    $0.delaysContentTouches = false
  }
  private let stackView = UIStackView().then {
    $0.spacing = 8.0
    $0.axis = .vertical
  }
  
  private let autoContributeStackView = UIStackView().then {
    $0.spacing = 8.0
    $0.axis = .vertical
  }
  
  let publisherView = PublisherView()
  let attentionView = PublisherAttentionView()
  private let autoContributeRow = SwitchRow().then {
    $0.textLabel.text = Strings.autoContributeSwitchLabel
    $0.toggleSwitch.isOn = true
  }
  let tipButton = ActionButton(type: .system).then {
    $0.tintColor = Colors.blurple500
    $0.titleLabel?.font = .systemFont(ofSize: 14.0, weight: .bold)
    $0.setTitle(Strings.publisherSendTip.uppercased(), for: .normal)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  public override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(scrollView)
    scrollView.addSubview(stackView)
    
    stackView.addArrangedSubview(publisherView)
    stackView.setCustomSpacing(20.0, after: publisherView)
    
    stackView.addArrangedSubview(autoContributeStackView)
    autoContributeStackView.addArrangedSubview(attentionView)
    autoContributeStackView.addArrangedSubview(SeparatorView())
    autoContributeStackView.addArrangedSubview(autoContributeRow)
    autoContributeStackView.addArrangedSubview(monthlyTipView)
    autoContributeStackView.addArrangedSubview(SeparatorView())
    
    stackView.setCustomSpacing(20.0, after: stackView.arrangedSubviews.last!)
    stackView.addArrangedSubview(tipButton)
    
    autoContributeRow.valueChanged = { enabled in
      self.autoContributeChanged?(enabled)
    }
    
    scrollView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    scrollView.contentLayoutGuide.snp.makeConstraints {
      $0.width.equalTo(self)
      $0.bottom.equalTo(stackView).offset(20.0)
    }
    stackView.snp.makeConstraints {
      $0.top.equalTo(self.scrollView.contentLayoutGuide.snp.top).offset(20.0)
      $0.leading.trailing.equalTo(self).inset(25.0)
    }
    tipButton.snp.makeConstraints {
      $0.height.equalTo(40.0)
    }
  }
  
  func updateViewVisibility(globalAutoContributionEnabled: Bool) {
    autoContributeStackView.isHidden = !globalAutoContributionEnabled
  }
}

extension PublisherSummaryView: WalletContentView {
  var innerScrollView: UIScrollView? {
    return scrollView
  }
  var displaysRewardsSummaryButton: Bool {
    return true
  }
}

