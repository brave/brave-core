// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Shared
import Storage
import UIKit
import Data
import PlaylistUI

extension Site.SiteType {
  var icon: UIImage? {
    switch self {
    case .history:
      return UIImage(braveSystemNamed: "leo.history")
    case .bookmark:
      return UIImage(braveSystemNamed: "leo.browser.bookmark-normal")
    case .tab:
      return UIImage(braveSystemNamed: "leo.browser.mobile-tabs")
    default:
      return nil
    }
  }
}

class SearchOnYourDeviceCell: UICollectionViewCell, CollectionViewReusable {

  struct ViewModel {
    let tileURL: URL
    let isPrivateBrowsing: Bool
    let title: String
    let subtitle: String
    let badge: UIImage
  }

  private let thumbnailLoader: MediaThumbnailLoader = .init()

  private let stackView = UIStackView().then {
    $0.spacing = 16.0
    $0.alignment = .center
  }

  private let imageContainerView = UIView().then {
    $0.layer.cornerRadius = 8.0
    $0.layer.cornerCurve = .continuous
    $0.backgroundColor = UIColor(braveSystemName: .containerHighlight)
    $0.clipsToBounds = true
  }
  private let imageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
  }
  private let thumbnailImageView = UIImageView().then {
    $0.contentMode = .scaleAspectFill
  }

  private let textStackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .leading
    $0.spacing = 4
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  private let titleLabel = UILabel().then {
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  private let subtitleLabel = UILabel().then {
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  private let badgeImageView = UIImageView()

  override var isHighlighted: Bool {
    didSet {
      UIView.animate(
        withDuration: 0.25,
        delay: 0,
        options: [.beginFromCurrentState],
        animations: {
          self.contentView.alpha = self.isHighlighted ? 0.5 : 1.0
        }
      )
    }
  }

  private let durationFormatter = DateComponentsFormatter().then {
    $0.maximumUnitCount = 3
    $0.unitsStyle = .positional
    $0.zeroFormattingBehavior = .pad
    $0.allowedUnits = [.hour, .minute, .second]
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .clear

    setTheme()

    contentView.addSubview(stackView)
    imageContainerView.addSubview(imageView)
    imageContainerView.addSubview(thumbnailImageView)
    stackView.addArrangedSubview(imageContainerView)
    stackView.addArrangedSubview(textStackView)
    stackView.addArrangedSubview(badgeImageView)
    textStackView.addArrangedSubview(titleLabel)
    textStackView.addArrangedSubview(subtitleLabel)

    stackView.snp.makeConstraints {
      $0.horizontalEdges.equalToSuperview().inset(20)
      $0.verticalEdges.equalToSuperview().inset(4)
    }

    imageContainerView.snp.makeConstraints {
      $0.width.height.equalTo(36.0)
    }

    imageView.snp.makeConstraints {
      $0.size.equalTo(20.0)
      $0.center.equalToSuperview()
    }
    thumbnailImageView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    badgeImageView.snp.makeConstraints {
      $0.size.equalTo(20.0)
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    setTheme()
  }

  private func setTheme() {
    var sizeCategory = UIApplication.shared.preferredContentSizeCategory
    if sizeCategory.isAccessibilityCategory {
      sizeCategory = .medium
    }
    let traitCollection = UITraitCollection(preferredContentSizeCategory: sizeCategory)

    titleLabel.do {
      $0.textColor = UIColor(braveSystemName: .textPrimary)
      $0.lineBreakMode = .byTruncatingTail

      let font = UIFont.preferredFont(
        for: .subheadline,
        weight: .regular,
        traitCollection: traitCollection
      )
      $0.font = font
    }

    subtitleLabel.do {
      $0.textColor = UIColor(braveSystemName: .textSecondary)
      $0.lineBreakMode = .byTruncatingTail

      let font = UIFont.preferredFont(
        for: .footnote,
        weight: .regular,
        traitCollection: traitCollection
      )
      $0.font = font
    }

    badgeImageView.do {
      $0.contentMode = .scaleAspectFit
      $0.tintColor = UIColor(braveSystemName: .iconSecondary)
    }
  }

  func setSite(_ site: Site, isPrivateBrowsing: Bool) {
    imageView.loadFavicon(
      for: site.tileURL,
      isPrivateBrowsing: isPrivateBrowsing
    )
    imageView.isHidden = false
    thumbnailImageView.isHidden = true

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
    if let badgeImage = site.siteType.icon?.template {
      badgeImageView.image = badgeImage
    }
  }

  func setPlaylistItem(_ item: PlaylistItem) {
    Task {
      if let assetURL = URL(string: item.mediaSrc), let pageURL = URL(string: item.pageSrc) {
        do {
          try await thumbnailLoader.loadThumbnail(assetURL: assetURL, pageURL: pageURL)
          if let image = thumbnailLoader.image {
            thumbnailImageView.image = image
          } else {
            thumbnailImageView.loadFavicon(for: pageURL, isPrivateBrowsing: false)
          }
        } catch {
          thumbnailImageView.loadFavicon(for: pageURL, isPrivateBrowsing: false)
        }
      }
    }

    imageView.isHidden = true
    thumbnailImageView.isHidden = false

    titleLabel.text = item.name
    if let formattedString = durationFormatter.string(from: item.duration) {
      subtitleLabel.text = formattedString
    }
    badgeImageView.image = UIImage(braveSystemNamed: "leo.product.playlist")
  }
}
