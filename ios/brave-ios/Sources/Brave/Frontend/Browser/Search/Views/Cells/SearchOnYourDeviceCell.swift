// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import Storage
import UIKit

class SearchOnYourDeviceCell: UICollectionViewCell, CollectionViewReusable {

  struct ViewModel {
    let tileURL: URL
    let isPrivateBrowsing: Bool
    let title: String
    let subtitle: String
    let badge: UIImage
  }

  private let stackView = UIStackView().then {
    $0.spacing = 16.0
    $0.alignment = .center
  }

  private let imageContainerView = UIView().then {
    $0.layer.cornerRadius = 8.0
    $0.backgroundColor = UIColor(braveSystemName: .containerHighlight)
  }
  private let imageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
  }

  private let textStackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .leading
    $0.spacing = 0
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  private let titleLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 15.0)
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.lineBreakMode = .byTruncatingTail
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  private let subtitleLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 13.0)
    $0.textColor = UIColor(braveSystemName: .textSecondary)
    $0.lineBreakMode = .byTruncatingTail
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  private let badgeImageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.tintColor = UIColor(braveSystemName: .iconSecondary)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .clear

    contentView.addSubview(stackView)
    imageContainerView.addSubview(imageView)
    stackView.addArrangedSubview(imageContainerView)
    stackView.addArrangedSubview(textStackView)
    stackView.addArrangedSubview(badgeImageView)
    textStackView.addArrangedSubview(titleLabel)
    textStackView.addArrangedSubview(subtitleLabel)

    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    imageContainerView.snp.makeConstraints {
      $0.width.height.equalTo(36.0)
    }

    imageView.snp.makeConstraints {
      $0.width.height.equalTo(20.0)
      $0.center.equalToSuperview()
    }

    badgeImageView.snp.makeConstraints {
      $0.width.height.equalTo(20.0)
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func setSite(_ site: Site, isPrivateBrowsing: Bool) {
    imageView.loadFavicon(
      for: site.tileURL,
      isPrivateBrowsing: isPrivateBrowsing
    )
    if site.siteType == .tab {
      titleLabel.text = site.title

      let detailTextForTabSuggestions = NSMutableAttributedString()
      detailTextForTabSuggestions.append(
        NSAttributedString(
          string: Strings.searchSuggestionOpenTabActionTitle,
          attributes: [
            .font: DynamicFontHelper.defaultHelper.smallSizeBoldWeightAS,
            .foregroundColor: UIColor(braveSystemName: .textSecondary),
          ]
        )
      )
      detailTextForTabSuggestions.append(
        NSAttributedString(
          string: " · \(site.url)",
          attributes: [
            .font: DynamicFontHelper.defaultHelper.smallSizeRegularWeightAS,
            .foregroundColor: UIColor(braveSystemName: .textSecondary),
          ]
        )
      )
      subtitleLabel.attributedText = detailTextForTabSuggestions
    } else {
      titleLabel.text = site.title
      subtitleLabel.text = site.url
    }
    badgeImageView.image = site.siteType.icon!.template
  }
}
