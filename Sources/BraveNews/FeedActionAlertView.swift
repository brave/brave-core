// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import Strings

/// A small non-interactive alert modal that displays an image, title and message and automatically
/// disappears after a given amount of time.
///
/// Present it using `present(on:dismissAfter:)`
public class FeedActionAlertView: UIView {
  private let baseView = UIVisualEffectView(effect: UIBlurEffect(style: .dark)).then {
    $0.layer.cornerRadius = 10.0
    $0.layer.cornerCurve = .continuous
    $0.clipsToBounds = true
  }

  private let imageView = UIImageView().then {
    $0.setContentHuggingPriority(.required, for: .vertical)
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.tintColor = .white
  }

  private let titleLabel = UILabel().then {
    $0.textAlignment = .center
    $0.numberOfLines = 0
    $0.font = .systemFont(ofSize: 16, weight: .semibold)
    $0.textColor = .white
  }

  private let messageLabel = UILabel().then {
    $0.textAlignment = .center
    $0.numberOfLines = 0
    $0.font = .systemFont(ofSize: 14)
    $0.textColor = .white

  }

  public init(image: UIImage, title: String, message: String) {
    super.init(frame: .zero)

    backgroundColor = .clear
    isUserInteractionEnabled = false
    accessibilityViewIsModal = true

    imageView.image = image.template
    titleLabel.text = title
    messageLabel.text = message

    let stackView = UIStackView(arrangedSubviews: [
      imageView,
      titleLabel,
      messageLabel,
    ]).then {
      $0.axis = .vertical
      $0.alignment = .center
      $0.spacing = 4
      $0.setCustomSpacing(12, after: imageView)
    }

    addSubview(baseView)
    baseView.contentView.addSubview(stackView)

    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(20)
    }
    baseView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  /// Present this alert on `viewController` and automatically hide it after a specific `interval`
  ///
  /// Defaults to dismissing after 2.5s
  public func present(on viewController: UIViewController, dismissingAfter interval: TimeInterval = 2.5) {
    viewController.view.addSubview(self)
    snp.makeConstraints {
      $0.center.equalToSuperview()
      $0.leading.top.greaterThanOrEqualTo(safeAreaLayoutGuide).inset(16)
      $0.trailing.bottom.lessThanOrEqualTo(safeAreaLayoutGuide).inset(16)
      $0.width.lessThanOrEqualToSuperview().multipliedBy(0.75)
    }

    baseView.alpha = 0.0
    UIView.animate(
      withDuration: 0.3, delay: 0, usingSpringWithDamping: 1.0, initialSpringVelocity: 0, options: [.beginFromCurrentState],
      animations: {
        self.baseView.alpha = 1.0
      },
      completion: { _ in
        let title = self.titleLabel.accessibilityLabel ?? ""
        let message = self.messageLabel.accessibilityLabel ?? ""
        if UIAccessibility.isVoiceOverRunning, !title.isEmpty || !message.isEmpty {
          // Dismiss after announcment ends instead of after specific interval
          NotificationCenter.default.addObserver(self, selector: #selector(self.voiceOverAnnouncmentCompleted), name: UIAccessibility.announcementDidFinishNotification, object: nil)
          DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            // Have to wait a second because the screen changing after the alert controller or
            // context menu goes away causes a screen changed notificaiton which resets focus.
            // When the screen resets focus it reads out the default focus of the screen, and
            // interrupts the announcment
            UIAccessibility.post(notification: .announcement, argument: "\(title): \(message)")
          }
        } else {
          DispatchQueue.main.asyncAfter(deadline: .now() + interval) {
            self.dismiss()
          }
        }
      })
  }

  @objc private func voiceOverAnnouncmentCompleted() {
    dismiss()
  }

  private func dismiss() {
    UIView.animate(
      withDuration: 0.15, delay: 0, usingSpringWithDamping: 1.0, initialSpringVelocity: 0, options: [.beginFromCurrentState],
      animations: {
        self.baseView.alpha = 0.0
      },
      completion: { _ in
        self.removeFromSuperview()
      })
  }
}

extension FeedActionAlertView {
  public static func feedDisabledAlertView(for item: FeedItem) -> FeedActionAlertView {
    FeedActionAlertView(
      image: UIImage(named: "disable.feed.source.alert", in: .module, compatibleWith: nil)!,
      title: Strings.BraveNews.disabledAlertTitle,
      message: String(format: Strings.BraveNews.disabledAlertBody, item.source.name)
    )
  }
}
