// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Foundation
import Shared
import UIKit

public class BraveNewsErrorView: UIView, FeedCardContent {

  public var refreshButtonTapped: (() -> Void)?

  private let backgroundView = FeedCardBackgroundView()

  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .center
    $0.spacing = 8
  }

  public let refreshButton = ActionButton().then {
    var configuration = UIButton.Configuration.plain()
    configuration.baseBackgroundColor = .clear
    configuration.contentInsets = NSDirectionalEdgeInsets(
      top: 6,
      leading: 12,
      bottom: 6,
      trailing: 12
    )
    configuration.titleTextAttributesTransformer = UIConfigurationTextAttributesTransformer {
      incoming in
      var outgoing = incoming
      outgoing.font = .systemFont(ofSize: 15.0, weight: .semibold)
      return outgoing
    }
    $0.configuration = configuration
    $0.backgroundColor = UIColor(braveSystemName: .buttonBackground)
    $0.setTitle(Strings.BraveNews.refresh, for: .normal)
    $0.setTitleColor(UIColor(braveSystemName: .schemesOnPrimary), for: .normal)
    $0.layer.borderWidth = 0
    $0.loaderView = LoaderView(size: .small).then {
      $0.tintColor = .white
    }
  }

  public let titleLabel = UILabel().then {
    $0.textAlignment = .center
    $0.textColor = .white
    $0.font = .systemFont(ofSize: 22, weight: .semibold)
    $0.numberOfLines = 0
  }

  public let errorMessageLabel = UILabel().then {
    $0.textAlignment = .center
    $0.textColor = .white
    $0.font = .systemFont(ofSize: 16)
    $0.numberOfLines = 0
  }

  public required init() {
    super.init(frame: .zero)

    addSubview(backgroundView)
    addSubview(stackView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(24)
    }

    stackView.addStackViewItems(
      .view(
        UIImageView(image: UIImage(named: "brave-today-error", in: .module, compatibleWith: nil)!)
      ),
      .customSpace(12),
      .view(titleLabel),
      .view(errorMessageLabel),
      .customSpace(20),
      .view(refreshButton)
    )

    refreshButton.addTarget(self, action: #selector(tappedRefreshButton), for: .touchUpInside)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  @objc private func tappedRefreshButton() {
    refreshButtonTapped?()
  }

  // unused
  public var actionHandler: ((Int, FeedItemAction) -> Void)?
  public var contextMenu: FeedItemMenu?
}
