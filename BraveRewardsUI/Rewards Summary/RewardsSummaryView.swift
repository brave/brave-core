/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class RewardsSummaryView: UIView {
  private struct UX {
    static let monthYearColor = Colors.blurple500
    static let gradientColors: [UIColor] = [Colors.blurple100, .white, .white]
    static let gradientLocations: [NSNumber] = [ 0.0, 0.4, 1.0 ]
    static let buttonHeight = 48.0
  }
  
  let rewardsSummaryButton = RewardsSummaryViewButton()
  let monthYearLabel = UILabel().then {
    $0.appearanceTextColor = UX.monthYearColor
    $0.font = .systemFont(ofSize: 22.0)
  }
  let scrollView = UIScrollView()
  let gradientView = GradientView().then {
    $0.gradientLayer.colors = UX.gradientColors.map { $0.cgColor }
    $0.gradientLayer.locations = UX.gradientLocations
  }
  let stackView = UIStackView().then {
    $0.axis = .vertical
  }
  let emptySummaryView = EmptyWalletSummaryView()
  
  var rows: [RowView] = [] {
    willSet {
      stackView.arrangedSubviews.forEach {
        $0.removeFromSuperview()
      }
    }
    didSet {
      if rows.isEmpty {
        stackView.addArrangedSubview(emptySummaryView)
      } else {
        rows.forEach {
          stackView.addArrangedSubview($0)
          if $0 !== rows.last {
            stackView.addArrangedSubview(SeparatorView())
          }
        }
      }
      if let disclaimerView = disclaimerView {
        if let finalRow = rows.last {
          stackView.setCustomSpacing(10.0, after: finalRow)
        }
        stackView.addArrangedSubview(disclaimerView)
      }
    }
  }
  
  /// A disclaimer view to show below the rows (Used when the user has auto-contribute enabled
  /// and has a portion of BAT designated to unverified publishers
  var disclaimerView: WalletDisclaimerView? {
    willSet {
      disclaimerView?.removeFromSuperview()
    }
    didSet {
      if let disclaimerView = disclaimerView {
        if let finalRow = rows.last {
          stackView.setCustomSpacing(10.0, after: finalRow)
        }
        stackView.addArrangedSubview(disclaimerView)
      }
    }
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(gradientView)
    addSubview(rewardsSummaryButton)
    addSubview(monthYearLabel)
    addSubview(scrollView)
    scrollView.addSubview(stackView)
    
    rewardsSummaryButton.snp.makeConstraints {
      $0.top.leading.trailing.equalTo(self)
      $0.height.equalTo(UX.buttonHeight)
    }
    monthYearLabel.snp.makeConstraints {
      $0.top.equalTo(self.rewardsSummaryButton.titleLabel.snp.bottom).offset(4.0)
      $0.leading.equalTo(self.rewardsSummaryButton.titleLabel)
      $0.trailing.equalTo(self.rewardsSummaryButton)
    }
    scrollView.snp.makeConstraints {
      $0.top.equalTo(self.monthYearLabel.snp.bottom).offset(20.0)
      $0.leading.trailing.bottom.equalTo(self)
    }
    scrollView.contentLayoutGuide.snp.makeConstraints {
      $0.width.equalTo(self)
      $0.bottom.equalTo(self.stackView).offset(20.0)
    }
    stackView.snp.makeConstraints {
      $0.top.equalTo(self.scrollView.contentLayoutGuide.snp.top)
      $0.leading.trailing.equalTo(self).inset(22.0)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    scrollView.layoutIfNeeded()
    gradientView.frame = scrollView.frame
  }
}

extension RewardsSummaryView: WalletContentView {
  var innerScrollView: UIScrollView? {
    return nil
  }
  var displaysRewardsSummaryButton: Bool {
    return false
  }
}
