// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Static
import BraveShared

/// The same style switch accessory view as in Static framework, except will not be recreated each time the Cell
/// is configured, since it will be stored as is in `Row.Accessory.view`
class SwitchAccessoryView: UISwitch {
    typealias ValueChange = (Bool) -> Void
    
    init(initialValue: Bool, valueChange: (ValueChange)? = nil) {
        self.valueChange = valueChange
        super.init(frame: .zero)
        isOn = initialValue
        addTarget(self, action: #selector(valueChanged), for: .valueChanged)
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    var valueChange: ValueChange?
    
    @objc func valueChanged() {
        valueChange?(self.isOn)
    }
}

/// Just creates a switch toggle `Row` which updates a `Preferences.Option<Bool>`
func BoolRow(title: String, option: Preferences.Option<Bool>, onValueChange: SwitchAccessoryView.ValueChange? = nil) -> Row {
    return Row(
        text: title,
        accessory: .view(SwitchAccessoryView(initialValue: option.value, valueChange: onValueChange ?? { option.value = $0 })),
        cellClass: MultilineValue1Cell.self,
        uuid: option.key
    )
}

class MultilineButtonCell: ButtonCell {
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        textLabel?.numberOfLines = 0
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}

class MultilineValue1Cell: Value1Cell {
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        textLabel?.numberOfLines = 0
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}

class MultilineSubtitleCell: SubtitleCell {
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        textLabel?.numberOfLines = 0
        detailTextLabel?.numberOfLines = 0
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
