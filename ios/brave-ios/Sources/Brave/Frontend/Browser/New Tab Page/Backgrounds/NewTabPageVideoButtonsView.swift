// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Preferences
import Shared
import SnapKit
import UIKit

/// The foreground view of the New Tab Page video player. It contains the cancel
/// button and handles user tap gestures to play/pause the video.
class NewTabPageVideoButtonsView: UIView {
  var tappedBackground: (() -> Bool)?
  var tappedCancelButton: (() -> Void)?

  private let playButtonImage = BluredImageView(imageName: "ntt_play_button").then {
    $0.isHidden = true
    $0.alpha = 0
  }

  private let pauseButtonImage = BluredImageView(imageName: "ntt_pause_button").then {
    $0.isHidden = true
    $0.alpha = 0
  }

  private let cancelButton = CancelButton().then {
    $0.isHidden = false
  }

  init() {
    super.init(frame: .zero)

    backgroundColor = .clear

    addSubview(cancelButton)

    addSubview(playButtonImage)
    addSubview(pauseButtonImage)

    let tapGesture: UITapGestureRecognizer = UITapGestureRecognizer(
      target: self,
      action: #selector(self.videoTapped(sender:))
    )
    tapGesture.numberOfTapsRequired = 1
    addGestureRecognizer(tapGesture)
  }

  private func tappedVideoCancelButton() {
    tappedCancelButton?()
  }

  @objc private func videoTapped(sender: UITapGestureRecognizer) {
    let location = sender.location(in: self)
    if let view = super.hitTest(location, with: nil), view == cancelButton {
      tappedVideoCancelButton()
      return
    }

    guard let playStarted = tappedBackground?() else {
      return
    }

    if playStarted {
      playButtonImage.isHidden = false
      pauseButtonImage.isHidden = true
      showAndFadeOutImage(imageView: playButtonImage)
    } else {
      pauseButtonImage.isHidden = false
      playButtonImage.isHidden = true
      showAndFadeOutImage(imageView: pauseButtonImage)
    }
  }

  private func showAndFadeOutImage(imageView: UIView) {
    imageView.alpha = 0
    UIView.animate(
      withDuration: 0.1,
      animations: {
        imageView.alpha = 1
      },
      completion: { _ in
        UIView.animate(
          withDuration: 0.3,
          delay: 0.5,
          animations: {
            imageView.alpha = 0
          }
        )
      }
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    cancelButton.snp.remakeConstraints {
      $0.top.equalTo(self.safeAreaLayoutGuide.snp.top).offset(20)
      $0.right.equalTo(self.safeAreaLayoutGuide.snp.right).offset(-20)
    }

    playButtonImage.snp.remakeConstraints {
      $0.centerX.equalToSuperview()
      $0.centerY.equalToSuperview().offset(20)
    }

    pauseButtonImage.snp.remakeConstraints {
      $0.centerX.equalToSuperview()
      $0.centerY.equalToSuperview().offset(20)
    }
  }
}

extension NewTabPageVideoButtonsView {
  private class CancelButton: SpringButton {
    let imageView = UIImageView(
      image: UIImage(named: "ntt_cancel_button", in: .module, compatibleWith: nil)!
    )

    private let backgroundView = UIVisualEffectView(
      effect: UIBlurEffect(style: .systemThinMaterialDark)
    ).then {
      $0.clipsToBounds = true
      $0.isUserInteractionEnabled = false
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      clipsToBounds = true

      addSubview(backgroundView)

      backgroundView.contentView.addSubview(imageView)

      backgroundView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }

      imageView.snp.makeConstraints {
        $0.edges.equalToSuperview().inset(UIEdgeInsets(equalInset: 4))
      }
    }

    override func layoutSubviews() {
      super.layoutSubviews()

      layer.cornerRadius = bounds.height / 2.0
    }
  }

  private class BluredImageView: UIView {
    private let backgroundView = UIVisualEffectView(
      effect: UIBlurEffect(style: .systemUltraThinMaterialDark)
    ).then {
      $0.clipsToBounds = true
      $0.isUserInteractionEnabled = false
    }

    init(imageName: String) {
      super.init(frame: .zero)

      clipsToBounds = true

      let imageView = UIImageView(
        image: UIImage(named: imageName, in: .module, compatibleWith: nil)!
      )

      addSubview(backgroundView)

      backgroundView.contentView.addSubview(imageView)

      backgroundView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }

      imageView.snp.makeConstraints {
        $0.edges.equalToSuperview().inset(UIEdgeInsets(equalInset: 10))
      }
    }

    override func layoutSubviews() {
      super.layoutSubviews()
      layer.cornerRadius = bounds.height / 2.0
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}
