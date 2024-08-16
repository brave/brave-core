// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Preferences
import Shared
import SnapKit
import UIKit

/// The foreground view of the New Tab Page video ad player. It contains the
/// cancel button and handles user tap gestures to play/pause the video.
class NewTabPageVideoAdButtonsView: UIView {
  var tappedBackgroundVideo: (() -> Bool)?
  var tappedCancelButton: (() -> Void)?

  private let playPauseButtonImage = BluredImageView().then {
    $0.alpha = 0
  }

  private let cancelButton = CancelButton()

  init() {
    super.init(frame: .zero)

    backgroundColor = .clear

    addSubview(cancelButton)
    addSubview(playPauseButtonImage)

    let tapGesture = UITapGestureRecognizer(
      target: self,
      action: #selector(self.backgroundVideoTapped(sender:))
    )
    addGestureRecognizer(tapGesture)

    cancelButton.snp.makeConstraints {
      $0.top.equalTo(self.safeAreaLayoutGuide.snp.top).offset(10)
      $0.right.equalTo(self.safeAreaLayoutGuide.snp.right).offset(-10)
    }
    playPauseButtonImage.snp.makeConstraints {
      $0.centerX.equalToSuperview()
      $0.centerY.equalToSuperview().offset(20)
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    cancelButton.snp.remakeConstraints {
      $0.top.equalTo(self.safeAreaLayoutGuide.snp.top).offset(10)
      $0.right.equalTo(self.safeAreaLayoutGuide.snp.right).offset(-10)
    }
  }

  private func tappedVideoCancelButton() {
    tappedCancelButton?()
  }

  @objc private func backgroundVideoTapped(sender: UITapGestureRecognizer) {
    let location = sender.location(in: self)
    if let view = super.hitTest(location, with: nil), view == cancelButton {
      tappedVideoCancelButton()
      return
    }

    guard let playStarted = tappedBackgroundVideo?() else {
      return
    }

    if playStarted {
      playPauseButtonImage.imageView.image = UIImage(braveSystemNamed: "leo.play.circle")!
        .withAlignmentRectInsets(.zero)
    } else {
      playPauseButtonImage.imageView.image = UIImage(braveSystemNamed: "leo.pause.circle")!
        .withAlignmentRectInsets(.zero)
    }
    showAndFadeOutImage(imageView: playPauseButtonImage)
  }

  private func showAndFadeOutImage(imageView: UIView) {
    imageView.alpha = 0
    UIView.animate(
      withDuration: 0.1,
      delay: 0,
      options: .allowUserInteraction,
      animations: {
        imageView.alpha = 1
      },
      completion: { _ in
        UIView.animate(
          withDuration: 0.3,
          delay: 0.5,
          options: .allowUserInteraction,
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
}

extension NewTabPageVideoAdButtonsView {
  private class CancelButton: SpringButton {
    let imageView = UIImageView(
      image: UIImage(braveSystemNamed: "leo.close")!
    ).then {
      $0.tintColor = .white
      $0.contentMode = .scaleAspectFit
      $0.preferredSymbolConfiguration = .init(
        font: .preferredFont(for: .title3, weight: .regular),
        scale: .small
      )
    }

    private let backgroundView = UIVisualEffectView(
      effect: UIBlurEffect(style: .systemUltraThinMaterialDark)
    ).then {
      $0.clipsToBounds = true
      $0.isUserInteractionEnabled = false
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      clipsToBounds = true
      accessibilityLabel = Strings.CancelString

      addSubview(backgroundView)

      backgroundView.contentView.addSubview(imageView)

      backgroundView.snp.makeConstraints {
        $0.edges.equalToSuperview()
        $0.width.equalTo(self.snp.height)
      }
      imageView.snp.makeConstraints {
        $0.edges.equalToSuperview().inset(UIEdgeInsets(equalInset: 3))
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
    let imageView = UIImageView().then {
      $0.tintColor = .white
      $0.contentMode = .scaleAspectFit
      $0.preferredSymbolConfiguration = .init(
        font: .preferredFont(for: .title1, weight: .regular),
        scale: .large
      )
    }

    init() {
      super.init(frame: .zero)

      clipsToBounds = true

      addSubview(backgroundView)

      backgroundView.contentView.addSubview(imageView)

      backgroundView.snp.makeConstraints {
        $0.edges.equalToSuperview()
        $0.width.equalTo(self.snp.height)
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
