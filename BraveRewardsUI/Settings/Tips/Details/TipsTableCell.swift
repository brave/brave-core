/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class TipsTableCell: UITableViewCell, TableViewReusable {
  
  private let siteStackView = UIStackView().then {
    $0.alignment = .center
    $0.spacing = 18.0
  }
  
  private let attentionBackgroundFillView = UIView().then {
    $0.backgroundColor = Colors.blurple100
  }
  
  let tokenView = BATUSDPairView(batAmountConfig: {
    $0.appearanceTextColor = Colors.grey800
    $0.font = .systemFont(ofSize: 14.0, weight: .medium)
  }, batKindConfig: {
    $0.appearanceTextColor = Colors.grey800
    $0.font = .systemFont(ofSize: 12.0)
  }, usdConfig: {
    $0.appearanceTextColor = Colors.grey700
    $0.font = .systemFont(ofSize: 10.0)
  }).then {
    $0.axis = .vertical
    $0.alignment = .trailing
    $0.spacing = 0.0
  }
  
  let siteImageView = UIImageView().then {
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.contentMode = .scaleAspectFit
    $0.snp.makeConstraints {
      $0.width.equalTo(28.0)
    }
  }
  let verifiedStatusImageView = UIImageView(image: UIImage(frameworkResourceNamed: "icn-verify")).then {
    $0.isHidden = true
  }
  private let siteAndTypeStackView = UIStackView().then {
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.axis = .vertical
    $0.alignment = .leading
  }
  let siteNameLabel = UILabel().then {
    $0.appearanceTextColor = Colors.grey800
    $0.font = .systemFont(ofSize: 14.0, weight: .medium)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.numberOfLines = 0
  }
  let typeNameLabel = UILabel().then {
    $0.appearanceTextColor = SettingsUX.bodyTextColor
    $0.font = .systemFont(ofSize: 13.0, weight: .medium)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.numberOfLines = 0
  }
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: nil)
    
    backgroundColor = .white
    
    insertSubview(attentionBackgroundFillView, belowSubview: contentView)
    contentView.addSubview(siteStackView)
    contentView.addSubview(tokenView)
    contentView.addSubview(verifiedStatusImageView)
    siteStackView.addArrangedSubview(siteImageView)
    siteStackView.addArrangedSubview(siteAndTypeStackView)
    siteAndTypeStackView.addArrangedSubview(siteNameLabel)
    siteAndTypeStackView.addArrangedSubview(typeNameLabel)
    
    siteStackView.snp.makeConstraints {
      $0.top.bottom.equalTo(contentView).inset(10.0)
      $0.leading.equalTo(contentView).inset(15.0)
      $0.trailing.lessThanOrEqualTo(tokenView.snp.leading).offset(-10.0)
    }
    verifiedStatusImageView.snp.makeConstraints {
      $0.top.equalTo(siteStackView)
      $0.leading.equalTo(siteImageView.snp.trailing).offset(-4.0)
    }
    tokenView.snp.makeConstraints {
      $0.trailing.equalTo(contentView).inset(15.0)
      $0.centerY.equalTo(contentView)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  // MARK: - Unavailable
  
  // swiftlint:disable unused_setter_value
  @available(*, unavailable)
  override var textLabel: UILabel? {
    get { return super.textLabel }
    set { }
  }
  
  @available(*, unavailable)
  override var detailTextLabel: UILabel? {
    get { return super.detailTextLabel }
    set { }
  }
  
  @available(*, unavailable)
  override var imageView: UIImageView? {
    get { return super.imageView }
    set { }
  }
  // swiftlint:enable unused_setter_value
}

