/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Static
import UIKit

public class OptionSelectionViewController<OptionType: RepresentableOptionType>: TableViewController {
  public typealias SelectedOptionChanged = (OptionSelectionViewController<OptionType>, OptionType) -> Void

  let options: [OptionType]
  private let optionChanged: SelectedOptionChanged

  public var selectedOption: OptionType {
    didSet {
      optionChanged(self, selectedOption)
    }
  }

  public var headerText: String? {
    didSet {
      guard let text = headerText else {
        dataSource.sections[0].header = nil
        return
      }
      dataSource.sections[0].header = .title(text)
    }
  }

  public var footerText: String? {
    didSet {
      guard let text = footerText else {
        dataSource.sections[0].footer = nil
        return
      }
      dataSource.sections[0].footer = .title(text)
    }
  }

  public init(
    headerText: String? = nil,
    footerText: String? = nil,
    style: UITableView.Style = .grouped,
    options: [OptionType],
    selectedOption: OptionType? = nil,
    optionChanged: @escaping SelectedOptionChanged) {
    assert(!options.isEmpty, "There should always be at least 1 option to choose from")

    self.options = options
    self.selectedOption = selectedOption ?? options.first!
    self.optionChanged = optionChanged

    super.init(style: style)
      
    var header: Section.Extremity?
    var footer: Section.Extremity?

    if let headerText = headerText {
      header = .title(headerText)
    }
      
    if let footerText = footerText {
      footer = .title(footerText)
    }

    dataSource.sections = [
      Section(
        header: header,
        rows: options.map { o in
          Row(
            text: o.displayString,
            selection: { [unowned self] in
              // Update selected option
              self.selectedOption = o
              self.updateRowsForSelectedOption()
              self.navigationController?.popViewController(animated: true)
            }, image: o.image, accessory: o == selectedOption ? .checkmark : .none)
        },
        footer: footer
      )
    ]
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .braveGroupedBackground
    view.tintColor = .braveBlurpleTint
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  private func updateRowsForSelectedOption() {
    for (idx, option) in options.enumerated() {
      if option.key == selectedOption.key {
        dataSource.sections[0].rows[idx].accessory = .checkmark
      } else {
        dataSource.sections[0].rows[idx].accessory = .none
      }
    }
  }
}
