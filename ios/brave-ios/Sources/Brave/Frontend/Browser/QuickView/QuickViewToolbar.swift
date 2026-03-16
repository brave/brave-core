// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import OSLog
import SnapKit
import UIKit

class QuickViewToolbar: UIView {

  // MARK: - Properties

  /// Closure called when close button is tapped
  var onClose: (() -> Void)?

  // MARK: - First Row Components

  private let shieldsButton: UIButton = {
    let button = UIButton(type: .system)
    button.setImage(UIImage(braveSystemNamed: "leo.shield.done"), for: .normal)
    button.tintColor = .bravePrimary
    return button
  }()

  private let urlLabel: UILabel = {
    let label = UILabel()
    label.font = .preferredFont(forTextStyle: .subheadline)
    label.textAlignment = .center
    label.textColor = .braveLabel
    label.text = "Loading..."
    return label
  }()

  private let refreshButton: UIButton = {
    let button = UIButton(type: .system)
    button.setImage(UIImage(braveSystemNamed: "leo.browser.refresh"), for: .normal)
    button.tintColor = .bravePrimary
    return button
  }()

  // MARK: - Second Row Components

  private let backButton: UIButton = {
    let button = UIButton(type: .system)
    button.setImage(UIImage(braveSystemNamed: "leo.browser.back"), for: .normal)
    button.tintColor = .bravePrimary
    return button
  }()

  private let shareButton: UIButton = {
    let button = UIButton(type: .system)
    button.setImage(UIImage(braveSystemNamed: "leo.share.macos"), for: .normal)
    button.tintColor = .bravePrimary
    return button
  }()

  private let openInTabButton: UIButton = {
    let button = UIButton(type: .system)
    button.setImage(UIImage(braveSystemNamed: "leo.add.tab"), for: .normal)
    button.tintColor = .label
    return button
  }()

  private let closeButton: UIButton = {
    let button = UIButton(type: .system)
    button.setImage(UIImage(systemName: "xmark"), for: .normal)
    button.tintColor = .white
    button.backgroundColor = .braveBlurpleTint
    button.layer.cornerRadius = 22
    button.clipsToBounds = true
    return button
  }()

  // MARK: - Initialization

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupUI()
    setupActions()
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: - Setup

  private func setupUI() {
    backgroundColor = .braveGroupedBackground

    // First row: [shields] [URL] [refresh]
    let leftButtonsStack = UIStackView(arrangedSubviews: [shieldsButton])
    leftButtonsStack.axis = .horizontal
    leftButtonsStack.spacing = 12
    leftButtonsStack.distribution = .fillEqually

    let firstRowStack = UIStackView(arrangedSubviews: [
      leftButtonsStack,
      urlLabel,
      refreshButton,
    ])
    firstRowStack.axis = .horizontal
    firstRowStack.spacing = 12
    firstRowStack.alignment = .center

    // Second row: [back] [share] [open-in-tab] [close X]
    let centerButtonsStack = UIStackView(arrangedSubviews: [
      backButton,
      shareButton,
      openInTabButton,
    ])
    centerButtonsStack.axis = .horizontal
    centerButtonsStack.spacing = 24
    centerButtonsStack.distribution = .fillEqually

    let secondRowStack = UIStackView(arrangedSubviews: [
      centerButtonsStack,
      closeButton,
    ])
    secondRowStack.axis = .horizontal
    secondRowStack.spacing = 16
    secondRowStack.alignment = .center

    // Main vertical stack
    let mainStack = UIStackView(arrangedSubviews: [firstRowStack, secondRowStack])
    mainStack.axis = .vertical
    mainStack.spacing = 12

    addSubview(mainStack)

    // Constraints
    mainStack.snp.makeConstraints {
      $0.top.equalTo(self).offset(12)
      $0.leading.trailing.equalTo(self).inset(16)
      $0.bottom.equalTo(self.safeAreaLayoutGuide.snp.bottom).offset(-12)
    }

    // Set button sizes
    shieldsButton.snp.makeConstraints { $0.width.height.equalTo(22) }
    refreshButton.snp.makeConstraints { $0.width.height.equalTo(22) }
    backButton.snp.makeConstraints { $0.width.height.equalTo(26) }
    shareButton.snp.makeConstraints { $0.width.height.equalTo(26) }
    openInTabButton.snp.makeConstraints { $0.width.height.equalTo(26) }
    closeButton.snp.makeConstraints { $0.width.height.equalTo(44) }

    urlLabel.setContentHuggingPriority(.defaultLow, for: .horizontal)
    urlLabel.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }

  private func setupActions() {
    closeButton.addTarget(self, action: #selector(closeButtonTapped), for: .touchUpInside)
  }

  // MARK: - Actions

  @objc private func closeButtonTapped() {
    onClose?()
  }

  // MARK: - Public Methods

  /// Updates the displayed URL
  func updateURL(_ url: URL) {
    urlLabel.text = url.host ?? url.absoluteString
  }
}
