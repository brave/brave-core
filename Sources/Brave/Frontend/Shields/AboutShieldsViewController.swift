// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveStrings
import Shared

class AboutShieldsViewController: UIViewController {
  private let textLabel = UILabel().then {
    $0.text = Strings.Shields.aboutBraveShieldsBody
    $0.textColor = .braveLabel
    $0.font = .systemFont(ofSize: 16)
    $0.numberOfLines = 0
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    self.title = Strings.Shields.aboutBraveShieldsTitle

    view.backgroundColor = .braveBackground
    view.addSubview(textLabel)

    textLabel.snp.makeConstraints {
      $0.top.leading.trailing.equalTo(view.safeAreaLayoutGuide).inset(32)
    }
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    preferredContentSize = textLabel.systemLayoutSizeFitting(
      CGSize(width: view.bounds.size.width - 64, height: 1000),
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    ).with {
      $0.width += 64
      $0.height += 64
    }
    navigationController?.preferredContentSize = preferredContentSize
  }
}
