// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Shared
import UIKit

class SyncAddDeviceInformationView: UIStackView {

  private var titleLabel = UILabel().then {
    $0.translatesAutoresizingMaskIntoConstraints = false
    $0.font = UIFont.systemFont(ofSize: 22, weight: UIFont.Weight.semibold)
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  private var descriptionLabel = UILabel().then {
    $0.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.numberOfLines = 0
    $0.lineBreakMode = .byTruncatingTail
    $0.adjustsFontSizeToFitWidth = true
    $0.minimumScaleFactor = 0.5
    $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    axis = .vertical
    spacing = 20
    alignment = .leading

    addArrangedSubview(titleLabel)
    addArrangedSubview(descriptionLabel)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  func updateLabels(isFirstIndex: Bool) {
    titleLabel.text = isFirstIndex ? Strings.syncAddDeviceScan : Strings.syncAddDeviceWords

    if isFirstIndex {
      let description = Strings.syncAddDeviceScanDescription
      let attributedDescription = NSMutableAttributedString(string: description)

      if let lastSentenceRange = lastSentenceRange(text: description) {
        attributedDescription.addAttributes(
          [
            .font: UIFont.systemFont(ofSize: 13, weight: UIFont.Weight.semibold),
            .foregroundColor: UIColor.braveErrorLabel,
          ],
          range: lastSentenceRange
        )
      }

      descriptionLabel.attributedText = attributedDescription
    } else {
      // The button name should be the same as in codewords instructions.
      let buttonName = Strings.scanSyncCode
      let addDeviceWords = String(format: Strings.syncAddDeviceWordsDescription, buttonName)
      let description = NSMutableAttributedString(string: addDeviceWords)
      let fontSize = descriptionLabel.font.pointSize

      let boldRange = (addDeviceWords as NSString).range(of: buttonName)
      description.addAttribute(
        .font,
        value: UIFont.boldSystemFont(ofSize: fontSize),
        range: boldRange
      )

      if let lastSentenceRange = lastSentenceRange(text: addDeviceWords) {
        description.addAttributes(
          [
            .font: UIFont.systemFont(ofSize: 13, weight: UIFont.Weight.semibold),
            .foregroundColor: UIColor.braveErrorLabel,
          ],
          range: lastSentenceRange
        )
      }

      descriptionLabel.attributedText = description
    }
  }

  private func lastSentenceRange(text: String) -> NSRange? {
    guard let lastSentence = text.split(separator: "\n").last else { return nil }
    return (text as NSString).range(of: String(lastSentence))
  }
}

class SyncAddDeviceCodeView: UIStackView {

  // View is created to create white frame around the QR code
  // It is done to solve problems scanning QR code with Android devices
  // Do not change how QR code is presented (size, background and inset)
  // Or do not try to Brave Logo in the middle
  private let qrCodeContainerView = UIView().then {
    $0.backgroundColor = .white
  }

  private let qrCodeView: SyncQRCodeView

  private let codeWordsContainerView = UIView().then {
    $0.backgroundColor = UIColor(braveSystemName: .containerBackground)
    $0.layer.cornerRadius = 8
    $0.layer.cornerCurve = .continuous
    $0.layer.borderColor = UIColor(braveSystemName: .dividerSubtle).cgColor
    $0.layer.borderWidth = 1.0
    $0.isHidden = true
  }

  private lazy var codewordsView = UILabel().then {
    $0.font = UIFont.monospacedSystemFont(ofSize: 15.0, weight: UIFont.Weight.regular)
    $0.lineBreakMode = NSLineBreakMode.byWordWrapping
    $0.textColor = UIColor(braveSystemName: .textPrimary)
    $0.textAlignment = .left
    $0.numberOfLines = 0
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
  }

  var syncChainCode: String? {
    return codewordsView.text
  }

  required init(syncAPI: BraveSyncAPI) {
    qrCodeView = SyncQRCodeView(syncApi: syncAPI)

    super.init(frame: .zero)

    codewordsView.do {
      $0.text = syncAPI.getTimeLimitedWords(fromWords: syncAPI.getSyncCode())
    }

    axis = .vertical
    alignment = .center

    addArrangedSubview(qrCodeContainerView)
    addArrangedSubview(codeWordsContainerView)

    qrCodeContainerView.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview()
      $0.width.equalTo(qrCodeContainerView.snp.height)
    }

    codeWordsContainerView.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview()
    }

    qrCodeContainerView.addSubview(qrCodeView)

    qrCodeView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(12)
    }

    codeWordsContainerView.addSubview(codewordsView)

    codewordsView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(12)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  func swapCodeViewType(_ isFirstIndex: Bool) {
    qrCodeContainerView.isHidden = !isFirstIndex
    codeWordsContainerView.isHidden = isFirstIndex
  }
}
