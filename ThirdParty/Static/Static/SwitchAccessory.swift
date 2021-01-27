import UIKit

class SwitchAccessory : UISwitch {
    typealias ValueChange = (Bool) -> ()
    
    init(initialValue: Bool, valueChange: (ValueChange)? = nil) {
        self.valueChange = valueChange
        super.init(frame: .zero)
        setOn(initialValue, animated: false)
        addTarget(self, action: #selector(valueChanged), for: .valueChanged)
    }
    
    fileprivate init() {super.init(frame: .zero)}
    fileprivate override init(frame: CGRect) {super.init(frame: frame)}
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    var valueChange : ValueChange?
    @objc func valueChanged() {
        valueChange?(self.isOn)
    }
}
