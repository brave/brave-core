/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

protocol DisplayableOption {
  var displayString: String { get }
}

extension String: DisplayableOption {
  var displayString: String {
    return self
  }
}

/// A view controller which allows a user to select a single option from a set
class OptionsSelectionViewController<OptionType: DisplayableOption>: UIViewController, UITableViewDelegate, UITableViewDataSource {
  /// The list of options
  let options: [OptionType]
  /// The selected option's index
  private(set) var selectedOptionIndex: Int {
    didSet {
      contentView.tableView.do {
        $0.cellForRow(at: IndexPath(row: oldValue, section: 0))?.accessoryType = .none
        $0.cellForRow(at: IndexPath(row: selectedOptionIndex, section: 0))?.accessoryType = .checkmark
      }
    }
  }
  /// A closure executed when the user taps on an option
  let optionSelected: (Int) -> Void
  
  init(options: [OptionType],
       selectedOptionIndex: Int = 0,
       optionSelected: @escaping (_ selectedIndex: Int) -> Void) {
    self.options = options
    self.selectedOptionIndex = selectedOptionIndex
    self.optionSelected = optionSelected
    
    super.init(nibName: nil, bundle: nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  private var contentView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  override func loadView() {
    view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    contentView.tableView.delegate = self
    contentView.tableView.dataSource = self
  }
  
  // MARK: - Delegate
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    selectedOptionIndex = indexPath.row
    optionSelected(selectedOptionIndex)
    tableView.deselectRow(at: indexPath, animated: true)
  }
  
  // MARK: - DataSource
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return options.count
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(withIdentifier: "OptionCell", for: indexPath)
    cell.backgroundColor = .white
    cell.textLabel?.appearanceTextColor = .black
    cell.textLabel?.text = options[indexPath.row].displayString
    cell.textLabel?.font = .systemFont(ofSize: 14.0)
    cell.textLabel?.appearanceTextColor = Colors.grey800
    cell.textLabel?.numberOfLines = 0
    cell.accessoryType = selectedOptionIndex == indexPath.row ? .checkmark : .none
    return cell
  }
}

extension OptionsSelectionViewController {
  private class View: UIView {
    let tableView = UITableView(frame: .zero, style: .grouped)
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      tableView.tableHeaderView = UIView(frame: CGRect(x: 0, y: 0, width: 0, height: CGFloat.leastNormalMagnitude))
      tableView.backgroundView = UIView().then { $0.backgroundColor = SettingsUX.backgroundColor }
      tableView.appearanceSeparatorColor = UIColor(white: 0.85, alpha: 1.0)
      tableView.register(UITableViewCell.self, forCellReuseIdentifier: "OptionCell")
      
      addSubview(tableView)
      
      tableView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}
