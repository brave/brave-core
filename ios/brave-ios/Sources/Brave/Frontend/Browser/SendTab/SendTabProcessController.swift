/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveUI
import BraveCore
import BraveStrings

class SendTabProcessController: SendTabTransitioningController {
  
  enum StatusType {
    case progress
    case completed

    var processTitle: String {
      switch self {
      case .progress:
        return Strings.OpenTabs.sendingWebpageProgressTitle
      case .completed:
        return Strings.OpenTabs.sendingWebpageCompletedTitle
      }
    }

    var processImage: UIImage? {
      switch self {
      case .progress:
        return UIImage(named: "tab-progress", in: .module, compatibleWith: nil)
      case .completed:
        return UIImage(named: "tab-completed", in: .module, compatibleWith: nil)
      }
    }
  }
  
  struct UX {
    static let contentInset = 20.0
    static let maxTabletWidth = 400
    static let maxPhoneWidth = 300
  }
  
  private let processTypeImageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.tintColor = .braveLabel
  }

  private let processInformationLabel = UILabel().then {
    $0.textAlignment = .center
    $0.textColor = .braveLabel
    $0.numberOfLines = 0
  }
  
  private let containerView = UIView().then {
    $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    $0.layer.cornerRadius = 10.0
    $0.layer.cornerCurve = .continuous
    $0.clipsToBounds = true
  }
  
  private var statusType: StatusType {
    didSet {
      processTypeImageView.image = self.statusType.processImage
      processInformationLabel.text = self.statusType.processTitle
    }
  }
  
  private var dataSource: SendableTabInfoDataSource
  
  private var sendTabAPI: BraveSendTabAPI
  
  init(type: StatusType, data: SendableTabInfoDataSource, sendTabAPI: BraveSendTabAPI) {
    statusType = type
    dataSource = data
    self.sendTabAPI = sendTabAPI
    
    super.init()
    
    containerView.backgroundColor = .secondaryBraveBackground
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    updateLayoutConstraints()
    updateFont()
    
    changeProcessStatus()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    
    updateFont()
  }
  
  private func updateLayoutConstraints() {
    view.addSubview(containerView)
    
    containerView.snp.makeConstraints {
      $0.centerX.centerY.equalToSuperview()
      if traitCollection.verticalSizeClass == .compact {
        $0.width.greaterThanOrEqualTo(UX.maxPhoneWidth)
      } else {
        if traitCollection.horizontalSizeClass == .regular {
          $0.width.greaterThanOrEqualTo(UX.maxTabletWidth)
        } else {
          $0.width.equalToSuperview().multipliedBy(0.75)
        }
      }
      $0.height.equalTo(containerView.snp.width)
    }
    
    processTypeImageView.image = statusType.processImage
    containerView.addSubview(processTypeImageView)
    
    processTypeImageView.snp.makeConstraints {
      $0.size.equalTo(80)
      $0.centerY.centerX.equalToSuperview()
    }
      
    processInformationLabel.text = statusType.processTitle
    containerView.addSubview(processInformationLabel)
    
    processInformationLabel.snp.makeConstraints {
      $0.bottom.equalToSuperview().inset(45)
      $0.leading.trailing.equalToSuperview().inset(15)
    }
  }
  
  private func updateFont() {
    let clampedTraitCollection = self.traitCollection.clampingSizeCategory(maximum: .accessibilityExtraLarge)
    
    let informationFont = UIFont.preferredFont(forTextStyle: .title3, compatibleWith: clampedTraitCollection)
    processInformationLabel.font = .systemFont(ofSize: informationFont.pointSize, weight: .semibold)
  }
  
  private func changeProcessStatus() {
    if let deviceCacheId = dataSource.deviceCacheID() {
      sendTabAPI.sendActiveTab(
        toDevice: deviceCacheId,
        tabTitle: dataSource.displayTitle,
        activeURL: dataSource.sendableURL)
    }
    
    Task.delayed(bySeconds: 3) { @MainActor in
      self.statusType = .completed
      
      Task.delayed(bySeconds: 2) { @MainActor in
        self.dismiss(animated: true)
      }
    }
  }
}
