// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveRewards
import BraveUI

/// A controller for displaying BATValue options with their equivalent dollar value.
/// When the user selects an option, the selected index is returned in the completion block.
/// If you pass false for `isSelectionPrecise`, the value will be prefixed with "Up to"
class BATValueOptionsSelectionViewController: OptionsSelectionViewController<BATValue> {
  
  private let ledger: BraveLedger?
  private var isSelectionPrecise: Bool
  
  init(ledger: BraveLedger,
       options: [BATValue],
       isSelectionPrecise: Bool = true,
       selectedOptionIndex: Int = 0,
       optionSelected: @escaping (_ selectedIndex: Int) -> Void) {
    self.isSelectionPrecise = isSelectionPrecise
    self.ledger = ledger
    super.init(options: options, selectedOptionIndex: selectedOptionIndex, optionSelected: optionSelected)
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = super.tableView(tableView, cellForRowAt: indexPath)

    var displayString = options[indexPath.row].displayString
    if !isSelectionPrecise {
      displayString = String.localizedStringWithFormat(Strings.settingsAutoContributeUpToValue, displayString)
    }
    let attributedText = NSMutableAttributedString(string: displayString, attributes: [
      .foregroundColor: Colors.grey800,
      .font: UIFont.systemFont(ofSize: 14.0, weight: .medium)
    ])
    
    attributedText.append(NSAttributedString(string: " \(Strings.BAT)", attributes: [
      .foregroundColor: Colors.grey700,
      .font: UIFont.systemFont(ofSize: 12.0)
    ]))
    
    cell.textLabel?.attributedText = attributedText
    return cell
  }
}
