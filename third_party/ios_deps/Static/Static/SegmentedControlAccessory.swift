import UIKit

open class SegmentedControlAccessory: UISegmentedControl {
    public typealias ValueChange = (Int, Any?) -> ()
    var valueChange: ValueChange? = nil
    
    public init(items: [Any], selectedIndex: Int, valueChange: (ValueChange)? = nil) {
        super.init(items: items)
        
        self.valueChange = valueChange
        self.selectedSegmentIndex = selectedIndex
        addTarget(self, action: #selector(valueChanged), for: .valueChanged)
    }
    
    public init() {super.init(frame: .zero)}
    public override init(frame: CGRect) {super.init(frame: frame)}
    
    public required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    @objc func valueChanged() {
        let segmentContents: Any? = titleForSegment(at: selectedSegmentIndex) ?? imageForSegment(at: selectedSegmentIndex)
        valueChange?(selectedSegmentIndex, segmentContents)
    }
}
