// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SDWebImage
import AVFoundation
import BraveUI

class PlaylistAssetFetcher {
  let itemId: String
  private let asset: AVURLAsset

  init(itemId: String, asset: AVURLAsset) {
    self.itemId = itemId
    self.asset = asset
  }

  func cancelLoading() {
    asset.cancelLoading()
  }
}

class PlaylistCell: UITableViewCell {
  var itemId: String?
  let thumbnailGenerator = PlaylistThumbnailRenderer()

  private let thumbnailMaskView = CAShapeLayer().then {
    $0.fillColor = UIColor.white.cgColor
  }

  private let thumbnailHolder = UIView().then {
    $0.backgroundColor = .black
    $0.contentMode = .scaleAspectFit
    $0.layer.cornerRadius = 5.0
    $0.layer.cornerCurve = .continuous
    $0.layer.masksToBounds = true
  }

  let loadingView = UIActivityIndicatorView(style: .medium).then {
    $0.hidesWhenStopped = true
    $0.tintColor = .white
  }

  let iconView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.layer.cornerRadius = 5.0
    $0.layer.cornerCurve = .continuous
    $0.layer.masksToBounds = true
  }

  let titleLabel = UILabel().then {
    $0.textColor = .bravePrimary
    $0.numberOfLines = 0
    $0.font = .preferredFont(for: .callout, weight: .medium)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
    $0.setContentHuggingPriority(.required, for: .vertical)
  }

  let detailLabel = UILabel().then {
    $0.textColor = .secondaryBraveLabel
    $0.numberOfLines = 0
    $0.font = .preferredFont(forTextStyle: .footnote)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
    $0.setContentHuggingPriority(.required, for: .vertical)
  }

  private let infoStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 5.0
  }

  private let separator = UIView().then {
    $0.backgroundColor = UIColor(white: 1.0, alpha: 0.15)
  }

  func prepareForDisplay() {
    titleLabel.text = nil
    detailLabel.text = nil
    iconView.image = nil
    thumbnailGenerator.cancel()
    iconView.cancelFaviconLoad()
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    preservesSuperviewLayoutMargins = false

    contentView.addSubview(thumbnailHolder)
    contentView.addSubview(infoStackView)
    contentView.addSubview(separator)
    
    thumbnailHolder.addSubview(iconView)
    thumbnailHolder.addSubview(loadingView)
    
    infoStackView.addArrangedSubview(titleLabel)
    infoStackView.addArrangedSubview(detailLabel)
    
    if traitCollection.preferredContentSizeCategory > .extraLarge {
      thumbnailHolder.snp.makeConstraints {
        $0.leading.trailing.equalToSuperview().inset(12.0)
        $0.height.equalTo(thumbnailHolder.snp.width).multipliedBy(9.0 / 16.0)
        $0.top.equalToSuperview().inset(8.0)
      }

      iconView.snp.makeConstraints {
        $0.center.equalToSuperview()
        $0.leading.trailing.top.bottom.equalToSuperview().priority(.high)
        $0.width.height.equalToSuperview()
      }

      loadingView.snp.makeConstraints {
        $0.center.equalToSuperview()
      }

      infoStackView.snp.makeConstraints {
        $0.leading.trailing.equalToSuperview().inset(12.0)
        $0.top.equalTo(thumbnailHolder.snp.bottom).offset(8.0)
        $0.bottom.equalToSuperview().inset(8.0)
      }

      separator.snp.makeConstraints {
        $0.leading.equalTo(titleLabel.snp.leading)
        $0.trailing.bottom.equalToSuperview()
        $0.height.equalTo(1.0 / UIScreen.main.scale)
      }
    } else {
      thumbnailHolder.snp.makeConstraints {
        // Keeps a 94.0px width on iPhone-X as per design
        $0.width.equalTo(contentView.snp.width).multipliedBy(0.30)
        $0.height.equalTo(thumbnailHolder.snp.width).multipliedBy(9.0 / 16.0)
        $0.leading.equalToSuperview().inset(12.0)
        $0.centerY.equalToSuperview()
        $0.top.greaterThanOrEqualToSuperview().inset(8.0)
        $0.bottom.lessThanOrEqualToSuperview().inset(8.0)
      }

      iconView.snp.makeConstraints {
        $0.center.equalToSuperview()
        $0.leading.trailing.top.bottom.equalToSuperview().priority(.high)
        $0.width.height.equalToSuperview()
      }

      loadingView.snp.makeConstraints {
        $0.center.equalToSuperview()
      }

      infoStackView.snp.makeConstraints {
        $0.leading.equalTo(thumbnailHolder.snp.trailing).offset(8.0)
        $0.trailing.equalToSuperview().inset(12.0)
        $0.centerY.equalToSuperview()
        $0.top.greaterThanOrEqualToSuperview().inset(8.0)
        $0.bottom.lessThanOrEqualToSuperview().inset(8.0)
      }

      separator.snp.makeConstraints {
        $0.leading.equalTo(titleLabel.snp.leading)
        $0.trailing.bottom.equalToSuperview()
        $0.height.equalTo(1.0 / UIScreen.main.scale)
      }
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override var layoutMargins: UIEdgeInsets {
    get {
      return .zero
    }

    set {  // swiftlint:disable:this unused_setter_value
      super.layoutMargins = .zero
    }
  }

  override var separatorInset: UIEdgeInsets {
    get {
      return UIEdgeInsets(top: 0, left: self.titleLabel.frame.origin.x, bottom: 0, right: 0)
    }

    set {  // swiftlint:disable:this unused_setter_value
      super.separatorInset = UIEdgeInsets(top: 0, left: self.titleLabel.frame.origin.x, bottom: 0, right: 0)
    }
  }
}
