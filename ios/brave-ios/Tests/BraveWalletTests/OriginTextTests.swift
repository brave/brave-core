import XCTest

@testable import BraveWallet

final class OriginTextTests: XCTestCase {
  func testFindsETldAtHostEndBeforePort() {
    let origin = "https://sub.example.com:8443"

    guard
      let range = eTldPlusOneRange(
        in: origin,
        eTldPlusOne: "example.com"
      )
    else {
      return XCTFail("Expected the eTLD+1 to be found in the host")
    }

    XCTAssertEqual(String(origin[..<range.lowerBound]), "https://sub.")
    XCTAssertEqual(String(origin[range]), "example.com")
    XCTAssertEqual(String(origin[range.upperBound...]), ":8443")
  }

  func testFindsLastOccurrenceWhenDomainRepeatsInSubdomain() {
    let origin = "https://example.com.example.com"

    guard
      let range = eTldPlusOneRange(
        in: origin,
        eTldPlusOne: "example.com"
      )
    else {
      return XCTFail("Expected the eTLD+1 suffix to be found")
    }

    XCTAssertEqual(String(origin[..<range.lowerBound]), "https://example.com.")
    XCTAssertEqual(String(origin[range]), "example.com")
  }

  func testReturnsNilWhenETldIsNotFound() {
    XCTAssertNil(
      eTldPlusOneRange(in: "https://notexample.net", eTldPlusOne: "example.com")
    )
  }

  func testRejectsEmptyETld() {
    XCTAssertNil(
      eTldPlusOneRange(in: "https://example.com", eTldPlusOne: "")
    )
  }
}
