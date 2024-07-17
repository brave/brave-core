// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Shared
import UIKit

public class Welcome3PAViewController: UIViewController {

  private let contentView = WelcomeViewCallout().then {
    $0.isBottomArrowHidden = true
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    let backgroundView = UIView().then {
      $0.backgroundColor = UIColor.black.withAlphaComponent(0.3)
    }

    let closeButton = BraveButton().then {
      $0.setImage(
        UIImage(braveSystemNamed: "leo.close")!.template,
        for: .normal
      )
      $0.tintColor = UIColor(braveSystemName: .textPrimary)
      $0.accessibilityLabel = Strings.Callout.p3aCalloutCloseAccessibilityLabel
      $0.hitTestSlop = UIEdgeInsets(equalInset: -20)
    }

    view.addSubview(backgroundView)
    backgroundView.snp.makeConstraints { $0.edges.equalToSuperview() }

    view.addSubview(contentView)
    contentView.snp.makeConstraints {
      $0.leading.trailing.greaterThanOrEqualTo(view)
      $0.centerX.centerY.equalToSuperview()
    }

    contentView.addSubview(closeButton)
    closeButton.snp.makeConstraints {
      $0.top.equalToSuperview().inset(20)
      $0.trailing.equalToSuperview().inset(30)
    }

    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(backgroundTapped))
    backgroundView.addGestureRecognizer(tapGesture)

    closeButton.addTarget(self, action: #selector(backgroundTapped), for: .touchUpInside)
  }

  @objc func backgroundTapped() {
    dismiss(animated: false)
  }

  public func setLayoutState(state: WelcomeViewCalloutState) {
    contentView.setState(state: state)
  }
}
