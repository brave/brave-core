// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import BraveShared
import SnapKit
import Shared
import UIKit
import BraveNews

/// A set of feed related components that overlay the New Tab Page when Brave News is enabled
class NewTabPageFeedOverlayView: UIView {

  let headerView = FeedSectionHeaderView().then {
    $0.alpha = 0.0
    $0.setContentHuggingPriority(.required, for: .vertical)
  }

  let loaderView = LoaderView(size: .small).then {
    $0.tintColor = .white
    $0.isHidden = true
    $0.isUserInteractionEnabled = false
  }

  let newContentAvailableButton = NewContentAvailableButton().then {
    $0.alpha = 0.0
  }

  func showNewContentAvailableButton() {
    let button = newContentAvailableButton
    button.transform = CGAffineTransform(translationX: 0, y: -100)
    button.alpha = 0.0
    UIView.animate(
      withDuration: 0.3, delay: 0, usingSpringWithDamping: 1.0, initialSpringVelocity: 0, options: [.beginFromCurrentState],
      animations: {
        button.alpha = 1.0
        button.transform = .identity
      })
  }

  func hideNewContentAvailableButton() {
    let button = newContentAvailableButton
    UIView.animate(
      withDuration: 0.3, delay: 0, usingSpringWithDamping: 1.0, initialSpringVelocity: 0, options: [.beginFromCurrentState],
      animations: {
        button.alpha = 0.0
        button.transform = CGAffineTransform(translationX: 0, y: -100)
      },
      completion: { _ in
        button.transform = .identity
        button.isLoading = false  // Reset state
      })
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(newContentAvailableButton)
    addSubview(headerView)
    addSubview(loaderView)

    clipsToBounds = true

    headerView.snp.makeConstraints {
      $0.top.leading.trailing.equalToSuperview()
    }

    newContentAvailableButton.snp.makeConstraints {
      $0.top.equalTo(headerView.snp.bottom).offset(16)
      $0.centerX.equalToSuperview()
    }

    loaderView.snp.makeConstraints {
      $0.centerX.equalToSuperview()
      $0.bottom.equalToSuperview().inset(16)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    // Disable taps on this view
    if let view = super.hitTest(point, with: event), view != self, view.isDescendant(of: self) {
      return view
    }
    return nil
  }
}

class NewContentAvailableButton: SpringButton {
  private let textLabel = UILabel().then {
    $0.textAlignment = .center
    $0.text = Strings.BraveNews.contentAvailableButtonTitle
    $0.numberOfLines = 0
    $0.textColor = .white
    $0.font = .systemFont(ofSize: 14.0, weight: .semibold)
  }

  private let loaderView = LoaderView(size: .small).then {
    $0.tintColor = .white
    $0.alpha = 0.0
  }

  var isLoading: Bool = false {
    didSet {
      UIView.animate(
        withDuration: 0.1,
        animations: {
          self.textLabel.alpha = self.isLoading ? 0 : 1
          self.loaderView.alpha = self.isLoading ? 1 : 0
        },
        completion: { _ in
          if !self.isLoading {
            self.loaderView.stop()
          }
        })
      if isLoading {
        loaderView.start()
      }

      textLabelConstraint?.isActive = !isLoading
      loaderConstraint?.isActive = isLoading

      UIView.animate(
        withDuration: 0.4, delay: 0, usingSpringWithDamping: 0.8, initialSpringVelocity: 0, options: [.beginFromCurrentState],
        animations: {
          self.layoutIfNeeded()
        })
    }
  }

  private var textLabelConstraint: Constraint?
  private var loaderConstraint: Constraint?

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .braveBlurpleTint
    layer.cornerCurve = .continuous

    clipsToBounds = true
    layer.shadowOffset = CGSize(width: 0, height: 1)
    layer.shadowRadius = 3
    layer.shadowOpacity = 0.3

    addSubview(textLabel)
    addSubview(loaderView)

    textLabel.snp.makeConstraints {
      self.textLabelConstraint = $0.edges.equalToSuperview().inset(UIEdgeInsets(top: 10, left: 18, bottom: 10, right: 18)).constraint
    }

    loaderView.snp.makeConstraints {
      self.loaderConstraint = $0.edges.equalToSuperview().inset(10).constraint
    }

    self.loaderConstraint?.isActive = false
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    layer.cornerRadius = bounds.height / 2.0
    layer.shadowPath = UIBezierPath(roundedRect: bounds, cornerRadius: layer.cornerRadius).cgPath
  }
}
