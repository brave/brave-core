import UIKit

open class ButtonCell: UITableViewCell, Cell {

    // MARK: - Initializers

    public override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: .default, reuseIdentifier: reuseIdentifier)
        initialize()
    }

    public required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
        initialize()
    }


    // MARK: - UIView

    open override func tintColorDidChange() {
        super.tintColorDidChange()
        textLabel?.textColor = tintColor
    }


    // MARK: - Private

    private func initialize() {
        tintColorDidChange()
    }
}
