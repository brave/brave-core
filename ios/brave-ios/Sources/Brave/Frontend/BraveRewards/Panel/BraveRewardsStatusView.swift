// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

class BraveRewardsStatusView: UIView {
  enum VisibleStatus {
    case rewardsOff
    case rewardsOn
  }

  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 0
  }

  private let onView = StatusLabelView(
    image: UIImage(named: "rewards-panel-on", in: .module, compatibleWith: nil)!.template,
    text: Strings.Rewards.enabledStatusBody
  ).then {
    $0.isHidden = true
  }

  private let offView = StatusLabelView(
    image: UIImage(named: "rewards-panel-off", in: .module, compatibleWith: nil)!.template,
    text: Strings.Rewards.disabledStatusBody
  )

  func setVisibleStatus(status: VisibleStatus, animated: Bool = true) {
    switch status {
    case .rewardsOff:
      setVisibleView(offView, animated: animated)
    case .rewardsOn:
      setVisibleView(onView, animated: animated)
    }
  }

  private var visibleView: UIView?
  private func setVisibleView(_ view: UIView, animated: Bool) {
    if view === visibleView { return }
    if animated {
      let oldValue = visibleView
      visibleView = view
      if oldValue != nil {
        visibleView?.alpha = 0.0
      }
      UIView.animate(
        withDuration: 0.1,
        animations: {
          oldValue?.alpha = 0.0
        },
        completion: { _ in
          oldValue?.isHidden = true
          UIView.animate(
            withDuration: 0.1,
            animations: {
              self.visibleView?.isHidden = false
            }
          )
          UIView.animate(withDuration: 0.1, delay: 0.05) {
            self.visibleView?.alpha = 1.0
          }
        }
      )
    } else {
      visibleView?.isHidden = true
      visibleView = view
      view.isHidden = false
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .secondaryBraveBackground

    layer.cornerRadius = 8
    layer.cornerCurve = .continuous

    addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(UIEdgeInsets(top: 16, left: 20, bottom: 16, right: 20))
    }

    stackView.addStackViewItems(
      .view(offView),
      .view(onView)
    )

    visibleView = offView
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

private class StatusLabelView: UIStackView {
  let image: UIImage
  let text: String

  private let imageView = UIImageView().then {
    $0.tintColor = .braveLabel
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private let label = UILabel().then {
    $0.textAlignment = .left
    $0.numberOfLines = 0
    $0.font = .systemFont(ofSize: 15)
    $0.textColor = .braveLabel
  }

  init(image: UIImage, text: String) {
    self.image = image
    self.text = text

    super.init(frame: .zero)

    imageView.image = image
    label.text = text

    spacing = 12
    alignment = .center

    addStackViewItems(
      .view(imageView),
      .view(label)
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
