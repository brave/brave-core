// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

/// The view to use for settings screens that display a table view
class SettingsTableView: UIView {
  let tableView = UITableView(frame: .zero, style: .grouped)
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    tableView.backgroundView = UIView().then {
      $0.backgroundColor = SettingsUX.backgroundColor
    }
    tableView.tableHeaderView = UIView(frame: CGRect(x: 0, y: 0, width: 0, height: CGFloat.leastNormalMagnitude))
    tableView.separatorInset = .zero
    
    addSubview(tableView)
    tableView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
