// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import AVKit
import Data
import Favicon

class VideoPlayerInfoBar: UIView {
  private let controlStackView = UIStackView().then {
    $0.spacing = 16
  }

  private let leftStackView = UIStackView().then {
    $0.spacing = 16
    $0.alignment = .center
  }

  private let blurView = UIVisualEffectView(effect: UIBlurEffect(style: .systemMaterialDark)).then {
    $0.contentView.backgroundColor = #colorLiteral(red: 0.231372549, green: 0.2431372549, blue: 0.3098039216, alpha: 0.8)
  }

  let sidePanelButton = UIButton().then {
    $0.setImage(UIImage(named: "playlist_split_navigation", in: .module, compatibleWith: nil)!, for: .normal)
    $0.contentEdgeInsets = .init(top: 8, left: 16, bottom: 8, right: 16)
  }

  let favIconImageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.layer.cornerRadius = 6.0
    $0.layer.cornerCurve = .continuous
    $0.layer.masksToBounds = true
    $0.isUserInteractionEnabled = true
  }

  let titleLabel = UILabel().then {
    $0.textColor = .white
    $0.font = .systemFont(ofSize: 15.0, weight: .medium)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  let pictureInPictureButton = UIButton().then {
    $0.imageView?.contentMode = .scaleAspectFit
    $0.setImage(UIImage(named: "playlist_pip", in: .module, compatibleWith: nil)!, for: .normal)
    $0.isHidden = !AVPictureInPictureController.isPictureInPictureSupported()
    $0.contentEdgeInsets = .init(equalInset: 8)
  }

  let fullscreenButton = UIButton().then {
    $0.imageView?.contentMode = .scaleAspectFit
    $0.setImage(UIImage(named: "playlist_fullscreen", in: .module, compatibleWith: nil)!, for: .normal)
    $0.contentEdgeInsets = .init(equalInset: 8)
  }

  let exitButton = UIButton().then {
    $0.imageView?.contentMode = .scaleAspectFit
    $0.setImage(UIImage(named: "playlist_exit", in: .module, compatibleWith: nil)!, for: .normal)
    $0.isHidden = true
    $0.contentEdgeInsets = .init(equalInset: 8)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(blurView)
    addSubview(leftStackView)
    addSubview(titleLabel)
    addSubview(controlStackView)

    [sidePanelButton, favIconImageView].forEach({
      leftStackView.addArrangedSubview($0)
    })
    [pictureInPictureButton, fullscreenButton, exitButton].forEach({
      controlStackView.addArrangedSubview($0)
    })

    blurView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    leftStackView.snp.makeConstraints {
      $0.leading.equalToSuperview().inset(20.0)
      $0.top.bottom.equalToSuperview().inset(8.0)
    }

    favIconImageView.snp.makeConstraints {
      $0.width.height.equalTo(28.0)
      $0.top.bottom.equalToSuperview().inset(12.0)
    }

    titleLabel.snp.makeConstraints {
      $0.leading.equalTo(favIconImageView.snp.trailing).offset(13.0)
      $0.top.bottom.equalToSuperview().inset(20.0)
    }

    controlStackView.snp.makeConstraints {
      $0.leading.greaterThanOrEqualTo(titleLabel.snp.trailing).offset(20.0)
      $0.trailing.equalToSuperview().offset(-20.0)
      $0.top.bottom.equalToSuperview().inset(8.0)
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func updateFavIcon(domain: String, isPrivateBrowsing: Bool) {
    favIconImageView.cancelFaviconLoad()
    favIconImageView.clearMonogramFavicon()
    favIconImageView.contentMode = .scaleAspectFit
    favIconImageView.image = Favicon.defaultImage

    if let url = URL(string: domain) {
      favIconImageView.loadFavicon(for: url, isPrivateBrowsing: isPrivateBrowsing)
    }
  }

  func clearFavIcon() {
    favIconImageView.clearMonogramFavicon()
    favIconImageView.image = nil
  }
}
