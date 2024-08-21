// swift-format-ignore-file

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import WebKit
import XCTest

@testable import UserAgent

private class MockPhoneDevice: UIDevice {
  override var userInterfaceIdiom: UIUserInterfaceIdiom { .phone }
  override var model: String { "iPhone" }
}

private class MockPadDevice: UIDevice {
  override var userInterfaceIdiom: UIUserInterfaceIdiom { .pad }
  override var model: String { "iPad" }
}

class UserAgentBuilderTests: XCTestCase {

  private let iPhone = MockPhoneDevice()
  private let iPad = MockPadDevice()

  func testDesktopUA() {
    let iOS13DesktopUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/13.1 \
      Safari/605.1.15
      """

    let iOS14DesktopUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/14.1.1 \
      Safari/605.1.15
      """

    let iOS15DesktopUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/15.5 \
      Safari/605.1.15
      """

    let iOS16DesktopUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/16.6 \
      Safari/605.1.15
      """

    let iOS17DesktopUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/17.6.1 \
      Safari/605.1.15
      """

    let iOS13 = OperatingSystemVersion(majorVersion: 13, minorVersion: 0, patchVersion: 0)

    XCTAssertEqual(
      iOS13DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS13).build(desktopMode: true),
      "iOS 13 desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS13DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS13).build(desktopMode: true),
      "iOS 13 desktop User Agent on iPad doesn't match"
    )

    let iOS14 = OperatingSystemVersion(majorVersion: 14, minorVersion: 3, patchVersion: 0)

    XCTAssertEqual(
      iOS14DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS14).build(desktopMode: true),
      "iOS 14 desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS14DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS14).build(desktopMode: true),
      "iOS 14 desktop User Agent on iPad doesn't match"
    )

    let iOS15 = OperatingSystemVersion(majorVersion: 15, minorVersion: 0, patchVersion: 0)

    XCTAssertEqual(
      iOS15DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS15).build(desktopMode: true),
      "iOS 15 desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS15DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS15).build(desktopMode: true),
      "iOS 15 desktop User Agent on iPad doesn't match"
    )

    let iOS16 = OperatingSystemVersion(majorVersion: 16, minorVersion: 0, patchVersion: 0)

    XCTAssertEqual(
      iOS16DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS16).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS16DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS16).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPad doesn't match"
    )

    let iOS17 = OperatingSystemVersion(majorVersion: 17, minorVersion: 0, patchVersion: 1)

    XCTAssertEqual(
      iOS17DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS17).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS17DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS17).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPad doesn't match"
    )
  }

  func testSpecificMobileUA() {
    // Put specific devices here, look how mobile UA looks like on your device and do hardcoded compare
    // For general UA tests there is another regex based test.
    //
    // For iPads please remember that desktop UA is used by default on 13+,
    // switch to mobile UA in safari before pasting the results here.
    //
    // At the moment each iOS version has one corresponding Safari UA attached
    // so for example 13.1 and 13.3 have the same UA.

    // MARK: - iOS 17
    let iPhone_safari_17_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 17_6_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/17.6.1 \
      Mobile/15E148 \
      Safari/604.1
      """

    let iPad_safari_17_UA = """
      Mozilla/5.0 (iPad; CPU OS 17_6_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/17.6.1 \
      Mobile/15E148 \
      Safari/604.1
      """

    // MARK: 17.0.1
    let ios17_0_1 = OperatingSystemVersion(majorVersion: 17, minorVersion: 0, patchVersion: 1)

    XCTAssertEqual(
      iPhone_safari_17_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios17_0_1).build(desktopMode: false),
      "User agent for iOS 17.0.1 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_17_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios17_0_1).build(desktopMode: false),
      "User agent for iOS 17.0.1 iPad doesn't match."
    )

    // MARK: - iOS 16
    let iPhone_safari_16_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 16_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/16.6 \
      Mobile/15E148 \
      Safari/604.1
      """

    let iPad_safari_16_UA = """
      Mozilla/5.0 (iPad; CPU OS 16_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/16.6 \
      Mobile/15E148 \
      Safari/604.1
      """

    // MARK: 16.1

    let ios16_6 = OperatingSystemVersion(majorVersion: 16, minorVersion: 6, patchVersion: 0)

    XCTAssertEqual(
      iPhone_safari_16_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios16_6).build(desktopMode: false),
      "User agent for iOS 16.6 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_16_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios16_6).build(desktopMode: false),
      "User agent for iOS 16.6 iPad doesn't match."
    )

    //// -------------------

    // MARK: - iOS 15
    let iPhone_safari_15_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 15_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/15.5 \
      Mobile/15E148 \
      Safari/604.1
      """

    let iPad_safari_15_UA = """
      Mozilla/5.0 (iPad; CPU OS 15_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/15.5 \
      Mobile/15E148 \
      Safari/604.1
      """

    // MARK: 15.5

    let ios15_5 = OperatingSystemVersion(majorVersion: 15, minorVersion: 5, patchVersion: 0)

    XCTAssertEqual(
      iPhone_safari_15_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios15_5).build(desktopMode: false),
      "User agent for iOS 54.0 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_15_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios15_5).build(desktopMode: false),
      "User agent for iOS 15.5 iPad doesn't match."
    )

    // MARK: - iOS 14
    let iPhone_safari_14_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/14.1.1 \
      Mobile/15E148 \
      Safari/604.1
      """

    let iPad_safari_14_UA = """
      Mozilla/5.0 (iPad; CPU OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/14.1.1 \
      Mobile/15E148 \
      Safari/604.1
      """

    // MARK: 14.3
    let ios14_0 = OperatingSystemVersion(majorVersion: 14, minorVersion: 3, patchVersion: 0)

    XCTAssertEqual(
      iPhone_safari_14_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios14_0).build(desktopMode: false),
      "User agent for iOS 14.0 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_14_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios14_0).build(desktopMode: false),
      "User agent for iOS 14.0 iPad doesn't match."
    )

    // MARK: 14.1.1
    let ios14_1_1 = OperatingSystemVersion(majorVersion: 14, minorVersion: 1, patchVersion: 1)

    XCTAssertEqual(
      iPhone_safari_14_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios14_1_1).build(desktopMode: false),
      "User agent for iOS 14.1.1 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_14_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios14_1_1).build(desktopMode: false),
      "User agent for iOS 14.1.1 iPad doesn't match."
    )

    // MARK: - iOS 13
    let iPhone_safari_13_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 13_6_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/13.1.2 \
      Mobile/15E148 \
      Safari/604.1
      """

    let iPad_safari_13_UA = """
      Mozilla/5.0 (iPad; CPU OS 13_6_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/13.1.2 \
      Mobile/15E148 \
      Safari/604.1
      """

    // MARK: 13.3.1
    let ios13_3_1 = OperatingSystemVersion(majorVersion: 13, minorVersion: 3, patchVersion: 1)

    XCTAssertEqual(
      iPhone_safari_13_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios13_3_1).build(desktopMode: false),
      "User agent for iOS 13.3.1 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_13_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios13_3_1).build(desktopMode: false),
      "User agent for iOS 13.3.1 iPad doesn't match."
    )

    // MARK: 13.7
    let ios13_7 = OperatingSystemVersion(majorVersion: 13, minorVersion: 7, patchVersion: 0)

    XCTAssertEqual(
      iPhone_safari_13_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios13_7).build(desktopMode: false),
      "User agent for iOS 13.7 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_13_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios13_7).build(desktopMode: false),
      "User agent for iOS 13.7 iPad doesn't match."
    )
  }

  func testFutureProofDesktopUA() {
    let iOS17DesktopUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/17.6.1 \
      Safari/605.1.15
      """

    let iOS34 = OperatingSystemVersion(majorVersion: 34, minorVersion: 0, patchVersion: 0)

    XCTAssertEqual(
      iOS17DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS34).build(desktopMode: true),
      "iOS 17 fallback desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS17DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS34).build(desktopMode: true),
      "iOS 17 fallback desktop User Agent on iPad doesn't match"
    )
  }

  func testFutureProofMobileUA() {
    // MARK: - iPhone iOS 15
    let ios34 = OperatingSystemVersion(majorVersion: 34, minorVersion: 0, patchVersion: 0)
    let iPhone_safari_34_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 34_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/34.0 \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPhone_safari_34_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios34).build(desktopMode: false),
      "User agent for iOS 34.0 iPhone doesn't match."
    )

    let iPad_safari_34_UA = """
      Mozilla/5.0 (iPad; CPU OS 34_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/34.0 \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPad_safari_34_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios34).build(desktopMode: false),
      "User agent for iOS 34.0 iPad doesn't match."
    )

    // MARK: - iPhone iOS 14.8, non existent version(14.8)
    let ios14_8 = OperatingSystemVersion(majorVersion: 14, minorVersion: 8, patchVersion: 0)
    let iPhone_safari_14_8_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/14.1.1 \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPhone_safari_14_8_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios14_8).build(desktopMode: false),
      "User agent for non existent iOS 14.8 iPhone doesn't match."
    )

    // MARK: - iPad iOS 14.8, non existent version(14.8)
    let iPad_safari_14_8_UA = """
      Mozilla/5.0 (iPad; CPU OS 14_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/14.1.1 \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPad_safari_14_8_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios14_8).build(desktopMode: false),
      "User agent for non existent iOS 14.8 iPad doesn't match."
    )

    // MARK: - iPhone iOS 13, non existent version(13.9.9)
    let ios13_9_9 = OperatingSystemVersion(majorVersion: 13, minorVersion: 9, patchVersion: 0)
    let iPhone_safari_13_9_9_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 13_6_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/13.1.2 \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPhone_safari_13_9_9_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios13_9_9).build(desktopMode: false),
      "User agent for non existent iOS 13.9.9 iPhone doesn't match."
    )

    // MARK: - iPad iOS 13, non existent version(13.9.9)
    let iPad_safari_13_9_9_UA = """
      Mozilla/5.0 (iPad; CPU OS 13_6_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/13.1.2 \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPad_safari_13_9_9_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios13_9_9).build(desktopMode: false),
      "User agent for non existent iOS 13.9.9 iPad doesn't match."
    )
  }

  func testUADifferences() {
    let ios13_1_1 = OperatingSystemVersion(majorVersion: 13, minorVersion: 1, patchVersion: 1)
    let ios13_1_3 = OperatingSystemVersion(majorVersion: 13, minorVersion: 1, patchVersion: 3)
    let ios14_1_1 = OperatingSystemVersion(majorVersion: 14, minorVersion: 1, patchVersion: 1)

    // Minor version difference
    XCTAssertEqual(
      UserAgentBuilder(device: iPhone, iOSVersion: ios13_1_1).build(desktopMode: false),
      UserAgentBuilder(device: iPhone, iOSVersion: ios13_1_3).build(desktopMode: false)
    )

    // Major version difference
    XCTAssertNotEqual(
      UserAgentBuilder(device: iPhone, iOSVersion: ios13_1_1).build(desktopMode: false),
      UserAgentBuilder(device: iPhone, iOSVersion: ios14_1_1).build(desktopMode: false)
    )

    // Device difference
    XCTAssertNotEqual(
      UserAgentBuilder(device: iPhone, iOSVersion: ios14_1_1).build(desktopMode: false),
      UserAgentBuilder(device: iPad, iOSVersion: ios14_1_1).build(desktopMode: false)
    )

    // Desktop mode difference
    XCTAssertNotEqual(
      UserAgentBuilder(device: iPhone, iOSVersion: ios14_1_1).build(desktopMode: false),
      UserAgentBuilder(device: iPhone, iOSVersion: ios14_1_1).build(desktopMode: true)
    )
  }

}
