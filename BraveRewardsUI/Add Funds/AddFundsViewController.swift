/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards

class AddFundsViewController: UIViewController {
  
  var addFundsView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  let state: RewardsState
  
  init(state: RewardsState) {
    self.state = state
    super.init(nibName: nil, bundle: nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func loadView() {
    self.view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = Strings.AddFundsVCTitle
    
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
    
    addFundsView.faqLinkTapped = { [unowned self] url in
      self.state.delegate?.loadNewTabWithURL(url)
    }
    
    let map: [(TokenAddressView.TokenKind, String?)] = [
//      (.bitcoin, state.ledger.btcAddress),
//      (.ethereum, state.ledger.ethAddress),
//      (.basicAttentionToken, state.ledger.batAddress),
//      (.litecoin, state.ledger.ltcAddress)
    ]
    addFundsView.tokenViews = map.map { (kind, address) in
      TokenAddressView(tokenKind: kind).then {
        $0.addressTextView.text = address
        $0.viewQRCodeButtonTapped = { [weak self] addressView in
          self?.tappedViewTokenQRCode(addressView)
        }
      }
    }
  }
  
  // MARK: - Actions
  
  private func tappedViewTokenQRCode(_ addressView: TokenAddressView) {
    addFundsView.tokenViews.forEach {
      if $0 !== addressView {
        $0.setQRCode(image: nil)
      }
    }
    let map: [TokenAddressView.TokenKind: String] = [
      .bitcoin: "",
//      .ethereum: state.ledger.ethAddress,
//      .basicAttentionToken: state.ledger.batAddress,
//      .litecoin: state.ledger.ltcAddress
    ].compactMapValues({ $0 })
    guard let address = map[addressView.tokenKind] else { return }
    let qrCode = "\(addressView.tokenKind.codePrefix):\(address)"
    addressView.setQRCode(image: QRCode.image(for: qrCode, size: CGSize(width: 90, height: 90)))
  }
  
  @objc private func tappedDone() {
    dismiss(animated: true)
  }
}
