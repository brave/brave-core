/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards

class GrantsListViewController: UIViewController {
  
  let ledger: BraveLedger
  
  init(ledger: BraveLedger) {
    self.ledger = ledger
    
    super.init(nibName: nil, bundle: nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  var grantsView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  override func loadView() {
    view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = Strings.Grants
    
    if let grants = ledger.walletInfo?.grants, !grants.isEmpty {
      grants.forEach {
        if let value = BATValue(probi: $0.probi) {
          let isAd = $0.type == "ads"
          grantsView.stackView.addArrangedSubview(
            GrantsItemView(
              amount: value.displayString,
              expirationDate: isAd ? nil : Date(timeIntervalSince1970: TimeInterval($0.expiryTime))
            )
          )
        }
      }
    }
  }
}

extension GrantsListViewController {
  class View: UIView {
    private let scrollView = UIScrollView().then {
      $0.alwaysBounceVertical = true
      $0.delaysContentTouches = false
    }
    
    fileprivate let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 10.0
    }
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      backgroundColor = SettingsUX.backgroundColor
      
      addSubview(scrollView)
      scrollView.addSubview(stackView)
      
      scrollView.snp.makeConstraints {
        $0.edges.equalTo(self)
      }
      scrollView.contentLayoutGuide.snp.makeConstraints {
        $0.width.equalTo(self)
      }
      stackView.snp.makeConstraints {
        $0.edges.equalTo(self.scrollView.contentLayoutGuide).inset(10.0)
      }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}
