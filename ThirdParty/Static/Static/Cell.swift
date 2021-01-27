import UIKit

public protocol Cell: class {
    static func description() -> String
    static func nib() -> UINib?

    func configure(row: Row)
}

extension Cell {
    public static func nib() -> UINib? {
        return nil
    }
}

extension Cell where Self: UITableViewCell {
    public func configure(row: Row) {
        accessibilityIdentifier = row.accessibilityIdentifier
        textLabel?.text = row.text
        detailTextLabel?.text = row.detailText
        imageView?.image = row.image
        accessoryType = row.accessory.type
        accessoryView = row.accessory.view
    }
}
