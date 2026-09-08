// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Preferences
import Shared
import SnapKit
import UIKit

protocol ReaderModeBarViewDelegate: AnyObject {
  func readerModeSettingsTapped(_ view: UIView)
}

class ReaderModeBarView: UIView {
  weak var delegate: ReaderModeBarViewDelegate?

  private let readerModeButton = UIButton(type: .system).then {
    $0.setTitle(Strings.readerModeButtonTitle, for: .normal)
    $0.titleLabel?.font = .preferredFont(forTextStyle: .subheadline)
    $0.accessibilityIdentifier = "ReaderModeBarView.readerModeSettingsButton"
  }

  private let settingsButton = UIButton(type: .system).then {
    $0.setImage(UIImage(braveSystemNamed: "leo.tune"), for: .normal)
    $0.accessibilityIdentifier = "ReaderModeBarView.settingsButton"
  }

  private var privateBrowsingManager: PrivateBrowsingManager

  private var cancellables: Set<AnyCancellable> = []
  private func updateColors() {
    let browserColors = privateBrowsingManager.browserColors
    if #unavailable(iOS 26) {
      backgroundView.contentView.backgroundColor = browserColors.chromeBackground
    }
    settingsButton.tintColor = browserColors.iconDefault
    readerModeButton.setTitleColor(browserColors.textPrimary, for: .normal)
  }

  private let backgroundView = UIVisualEffectView()

  init(privateBrowsingManager: PrivateBrowsingManager) {
    self.privateBrowsingManager = privateBrowsingManager

    super.init(frame: .zero)

    addSubview(backgroundView)
    if #available(iOS 26, *) {
      backgroundView.clipsToBounds = true
      backgroundView.layer.cornerCurve = .continuous
      let glass = UIGlassEffect(style: .regular)
      glass.isInteractive = true
      backgroundView.effect = glass
    }
    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    backgroundView.contentView.addSubview(readerModeButton)
    readerModeButton.addTarget(self, action: #selector(tappedSettingsButton), for: .touchUpInside)
    readerModeButton.snp.makeConstraints {
      $0.centerX.centerY.equalToSuperview()
    }

    if #unavailable(iOS 26) {
      let borderView = UIView.separatorLine
      addSubview(borderView)
      borderView.snp.makeConstraints {
        $0.top.equalTo(snp.bottom)
        $0.leading.trailing.equalToSuperview()
      }
    }

    backgroundView.contentView.addSubview(settingsButton)
    settingsButton.addTarget(self, action: #selector(tappedSettingsButton), for: .touchUpInside)
    settingsButton.snp.makeConstraints {
      $0.trailing.equalToSuperview().inset(16)
      $0.centerY.equalToSuperview()
    }

    privateBrowsingManager
      .$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] _ in
        self?.updateColors()
      })
      .store(in: &cancellables)

    updateColors()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @objc func tappedSettingsButton(_ sender: UIButton!) {
    delegate?.readerModeSettingsTapped(sender)
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    if #available(iOS 26, *) {
      backgroundView.layer.cornerRadius = bounds.height / 2
    }
  }
}
