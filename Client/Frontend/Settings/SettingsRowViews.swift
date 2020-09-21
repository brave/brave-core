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

extension Row {
    /// Creates a switch toggle `Row` which updates a `Preferences.Option<Bool>`
    static func boolRow(title: String, detailText: String? = nil, option: Preferences.Option<Bool>, onValueChange: SwitchAccessoryView.ValueChange? = nil, image: UIImage? = nil) -> Row {
        return Row(
            text: title,
            detailText: detailText,
            image: image,
            accessory: .view(SwitchAccessoryView(initialValue: option.value, valueChange: onValueChange ?? { option.value = $0 })),
            cellClass: MultilineSubtitleCell.self,
            uuid: option.key
        )
    }
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

class CenteredButtonCell: ButtonCell {
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        textLabel?.textAlignment = .center
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

class ColoredDetailCell: UITableViewCell, Cell {
    
    static let colorKey = "color"
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: .value1, reuseIdentifier: reuseIdentifier)
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func configure(row: Row) {
        textLabel?.text = row.text
        detailTextLabel?.text = row.detailText
        accessoryType = row.accessory.type
        imageView?.image = row.image
        
        guard let detailColor = row.context?[ColoredDetailCell.colorKey] as? UIColor else { return }
        detailTextLabel?.appearanceTextColor = detailColor
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
