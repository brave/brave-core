// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

public class AdView: UIView {
  let adContentButton = AdContentButton()

  public override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(adContentButton)

    adContentButton.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
