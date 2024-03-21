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

class NewTabPageVideoButtons: UIView {
  var tappedPlayButton: ((UIControl) -> Void)?

  private let playButton = UIButton(frame: CGRect(x: 100, y: 100, width: 100, height: 50)).then {
    $0.isHidden = false
  }

  private var playFinished = false

  init() {
    super.init(frame: .zero)

    backgroundColor = .clear

    playButton.backgroundColor = .systemBlue
    playButton.setTitle("Play/pause", for: .normal)
    playButton.addTarget(self, action: #selector(tappedPlayButtonProxy(_:)), for: .touchUpInside)
    addSubview(playButton)

    playButton.frame = CGRectMake(
      (self.frame.size.width - 120),
      (self.frame.size.height) / 2,
      100,
      50
    )
  }

  func addPlayPauseButton() {
    playButton.isHidden = false
  }

  func setPlayFinished() {
    playButton.setTitle("Finished...", for: .normal)
    playButton.backgroundColor = .systemRed
    playFinished = true
  }

  func setPlay25Reached() {
    playButton.setTitle("25 Pecent", for: .normal)
    playButton.backgroundColor = .systemMint
  }

  func setPlayStarted() {
    if playFinished {
      playButton.backgroundColor = .systemBlue
      playButton.setTitle("Play/pause", for: .normal)
      playFinished = false
    }
  }

  func setPlayPaused() {
  }

  @objc private func tappedPlayButtonProxy(_ sender: UIControl) {
    tappedPlayButton?(sender)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    // Disable taps on this view
    if let view = super.hitTest(point, with: event), view == playButton {
      return view
    }
    return nil
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    playButton.frame = CGRectMake(
      (self.frame.size.width - 120),
      (self.frame.size.height) / 2,
      100,
      50
    )
  }
}
