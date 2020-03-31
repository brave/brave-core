/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class CreateWalletButton: ActionButton {
  
  var isCreatingWallet: Bool = false {
    didSet {
      isLoading = isCreatingWallet
      if isCreatingWallet {
        setTitle(Strings.creatingWallet.uppercased(), for: .normal)
        // Make the load start when the new title label is in place (since its animated)
      } else {
        setTitle(titleText, for: .normal)
      }
    }
  }
  
  private let titleText: String
  
  override var isHighlighted: Bool {
    didSet {
      // Replicating usual UIButton highlight animation
      UIView.animate(withDuration: isHighlighted ? 0.05 : 0.4, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [.beginFromCurrentState], animations: {
        self.titleLabel?.alpha = self.isHighlighted ? 0.3 : 1.0
      }, completion: nil)
    }
  }
  
  init(titleText: String) {
    self.titleText = titleText
    super.init(frame: .zero)
    loaderView = LoaderView(size: .small)
    loaderPlacement = .right
    setTitle(titleText, for: .normal)
    titleLabel?.font = .systemFont(ofSize: 14.0, weight: .bold)
  }
}
