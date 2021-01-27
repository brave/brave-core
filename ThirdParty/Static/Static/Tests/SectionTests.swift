import XCTest
@testable import Static

class SectionTests: XCTestCase {

    func testInit() {
        let rows = [Row(text: "Row")]
        let section = Section(header: "Header", rows: rows, footer: "Footer", uuid: "1234")
        XCTAssertEqual("1234", section.uuid)
        XCTAssertEqual("Header", section.header!._title!)
        XCTAssertEqual(rows, section.rows)
        XCTAssertEqual("Footer", section.footer!._title!)
    }

    func testExtermityViews() {
        let header = UIView()
        let footer = UIView()
        let section = Section(header: .view(header), footer: .view(footer))
        XCTAssertEqual(header, section.header!._view!)
        XCTAssertEqual(footer, section.footer!._view!)
    }

    func testHashable() {
        let section = Section()
        var hash = [
            section: "hi"
        ]

        XCTAssertEqual("hi", hash[section]!)
    }

    func testEquatable() {
        let section1 = Section()
        let section2 = Section()

        XCTAssertEqual(section1, section1)
        XCTAssertFalse(section1 == section2)
    }
}
