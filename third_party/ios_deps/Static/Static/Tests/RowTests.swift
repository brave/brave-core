import XCTest
import Static

class RowTests: XCTestCase {

    func testInit() {
        let selection: Selection = {}
        let context: Row.Context = [
            "Hello": "world"
        ]

        let row = Row(text: "Title", detailText: "Detail", selection: selection, cellClass: ButtonCell.self, context: context, uuid: "1234", accessibilityIdentifier: "TitleRow")
        XCTAssertEqual("1234", row.uuid)
        XCTAssertEqual("Title", row.text!)
        XCTAssertEqual("Detail", row.detailText!)
        XCTAssertEqual("world", row.context?["Hello"] as? String)
        XCTAssertEqual("TitleRow", row.accessibilityIdentifier)
    }

    func testInitWithImage() {
        let image = UIImage(named: "Setting")
        let row = Row(image: image)
        XCTAssertEqual(row.image, image)
    }

    func testInitWithAccessoryType() {
        let accessory: Row.Accessory = .checkmark

        let row = Row(accessory: accessory)

        XCTAssertTrue(row.accessory == accessory)
    }

    func testInitWithSelectableAccessoryType() {
        let selection: Selection = {}
        let accessory: Row.Accessory = .detailButton(selection)

        let row = Row(accessory: accessory)

        XCTAssertEqual(row.accessory, accessory)
        XCTAssertTrue(row.accessory.selection != nil)
    }

    func testInitWithAccessoryView() {
        let view = UIView()
        let accessory: Row.Accessory = .view(view)

        let row = Row(accessory: accessory)

        XCTAssertEqual(row.accessory, accessory)
        XCTAssertEqual(row.accessory.view!, view)
    }

    func testHashable() {
        let row = Row()
        var hash = [
            row: "hi"
        ]

        XCTAssertEqual("hi", hash[row]!)
    }

    func testEquatable() {
        let row1 = Row()
        let row2 = Row()

        XCTAssertEqual(row1, row1)
        XCTAssertFalse(row1 == row2)
    }
}
