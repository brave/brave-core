/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Static

/// Defines something that be used as an option type where theres usually a static set of options in a selectable list
public protocol RepresentableOptionType: Equatable {
    /// A key that can be used to define this option type
    var key: String { get }
    /// The string to show to users when presenting this option
    var displayString: String { get }
    /// An image to display next to the option
    var image: UIImage? { get }
}

// Default to no images
extension RepresentableOptionType {
    public var image: UIImage? {
        return nil
    }
}

/// Automatically infer `key` and equality when Self already provides a rawValue (mostly String enum's)
extension RepresentableOptionType where Self: RawRepresentable, Self.RawValue: Equatable {
    
    public var key: String {
        return String(describing: rawValue)
    }
    
    public static func == (lhs: Self, rhs: Self) -> Bool {
        return lhs.rawValue == rhs.rawValue
    }
}

class OptionSelectionViewController<OptionType: RepresentableOptionType>: TableViewController {
    typealias SelectedOptionChanged = (OptionSelectionViewController<OptionType>, OptionType) -> Void
    
    let options: [OptionType]
    private let optionChanged: SelectedOptionChanged
    
    var selectedOption: OptionType {
        didSet {
            optionChanged(self, selectedOption)
        }
    }
    
    var headerText: String? {
        didSet {
            guard let text = headerText else {
                dataSource.sections[0].header = nil
                return
            }
            dataSource.sections[0].header = .title(text)
        }
    }
    
    var footerText: String? {
        didSet {
            guard let text = footerText else {
                dataSource.sections[0].footer = nil
                return
            }
            dataSource.sections[0].footer = .title(text)
        }
    }
    
    init(options: [OptionType], selectedOption: OptionType? = nil, optionChanged: @escaping SelectedOptionChanged) {
        assert(options.count > 0, "There should always be at least 1 option to choose from")
        
        self.options = options
        self.selectedOption = selectedOption ?? options.first!
        self.optionChanged = optionChanged
        
        super.init(style: .grouped)
        
        dataSource.sections = [
            Section(
                rows: options.map { o in
                    Row(text: o.displayString, selection: { [unowned self] in
                        // Update selected option
                        self.selectedOption = o
                        self.updateRowsForSelectedOption()
                        self.navigationController?.popViewController(animated: true)
                    }, image: o.image, accessory: o == selectedOption ? .checkmark : .none)
                }
            )
        ]
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
