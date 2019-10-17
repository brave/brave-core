// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveRewards

/// A controller for displaying BATValue options with their equivalent dollar value.
/// When the user selects an option, the selected index is returned in the completion block.
class BATValueOptionsSelectionViewController: OptionsSelectionViewController<BATValue> {
  
  private let ledger: BraveLedger?
  
  init(ledger: BraveLedger, options: [BATValue],
       selectedOptionIndex: Int = 0,
       optionSelected: @escaping (_ selectedIndex: Int) -> Void) {
    self.ledger = ledger
    super.init(options: options, selectedOptionIndex: selectedOptionIndex, optionSelected: optionSelected)
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = super.tableView(tableView, cellForRowAt: indexPath)

    let displayString = options[indexPath.row].displayString
    let dollarAmount = ledger?.dollarStringForBATAmount(options[indexPath.row].doubleValue) ?? ""
    
    let attributedText = NSMutableAttributedString(string: displayString, attributes: [
      .foregroundColor: Colors.grey100,
      .font: UIFont.systemFont(ofSize: 14.0, weight: .medium)
    ])
    
    attributedText.append(NSAttributedString(string: " BAT", attributes: [
      .foregroundColor: Colors.grey200,
      .font: UIFont.systemFont(ofSize: 12.0)
    ]))
    
    attributedText.append(NSAttributedString(string: " (\(dollarAmount))", attributes: [
      .foregroundColor: Colors.grey200,
      .font: UIFont.systemFont(ofSize: 10.0)
    ]))
    
    cell.textLabel?.attributedText = attributedText
    return cell
  }
}
