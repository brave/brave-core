// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveUI
import BraveCore

public class Welcome3PAViewController: UIViewController {

  private let contentView = WelcomeViewCallout().then {
    $0.isBottomArrowHidden = true
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()

    let backgroundView = UIView().then {
      $0.backgroundColor = UIColor.black.withAlphaComponent(0.3)
    }

    view.addSubview(backgroundView)
    backgroundView.snp.makeConstraints { $0.edges.equalToSuperview() }

    view.addSubview(contentView)
    contentView.snp.makeConstraints {
      $0.leading.trailing.greaterThanOrEqualTo(view)
      $0.centerX.centerY.equalToSuperview()
    }

    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(backgroundTapped))
    backgroundView.addGestureRecognizer(tapGesture)
  }

  @objc func backgroundTapped() {
    dismiss(animated: false)
  }

  public func setLayoutState(state: WelcomeViewCalloutState) {
    contentView.setState(state: state)
  }
}
