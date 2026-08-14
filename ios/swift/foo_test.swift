import XCTest

@testable import BraveCoreSwift

class BraveCoreSwiftTests: XCTestCase {
  func testInternalFoo() {
    let foo = InternalFooClass()
  }

  func testFoo() {
    let foo = FooClass()
  }
}
