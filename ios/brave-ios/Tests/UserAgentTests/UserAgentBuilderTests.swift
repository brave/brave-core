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

  // test bundles have a different major version than the app will have
  private var bundleMajorVersion: String {
    guard
      let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString")
        as? String,
      let majorVersion = version.components(separatedBy: ".").first
    else {
      return ""
    }
    return majorVersion
  }

  // Test BraveUserAgentBuilder for desktop with `useAppIdentifierEnabled` enabled
  func testDesktopBraveUA() {
    let iOS16 = OperatingSystemVersion(majorVersion: 16, minorVersion: 0, patchVersion: 0)
    let iOS16DesktopBraveUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Brave/\(bundleMajorVersion) \
      Version/16.0 \
      Safari/605.1.15
      """

    XCTAssertEqual(
      iOS16DesktopBraveUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS16).build(desktopMode: true),
      "iOS 16 desktop Brave User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS16DesktopBraveUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS16, useAppIdentifierEnabled: true).build(desktopMode: true),
      "iOS 16 desktop Brave User Agent on iPad doesn't match"
    )

    let iOS17 = OperatingSystemVersion(majorVersion: 17, minorVersion: 0, patchVersion: 1)
    let iOS17DesktopBraveUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Brave/\(bundleMajorVersion) \
      Version/17.0.1 \
      Safari/605.1.15
      """

    XCTAssertEqual(
      iOS17DesktopBraveUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS17, useAppIdentifierEnabled: true).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS17DesktopBraveUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS17, useAppIdentifierEnabled: true).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPad doesn't match"
    )
  }

  // Test BraveUserAgentBuilder for desktop with `useAppIdentifierEnabled` disabled or masked
  func testDesktopUA() {
    let iOS16 = OperatingSystemVersion(majorVersion: 16, minorVersion: 0, patchVersion: 0)
    let iOS16DesktopUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/16.0 \
      Safari/605.1.15
      """

    XCTAssertEqual(
      iOS16DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS16, useAppIdentifierEnabled: false).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPhone doesn't match"
    )
    XCTAssertEqual(
      iOS16DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS16, useAppIdentifierEnabled: true).build(desktopMode: true, useSafariUA: true),
      "iOS 16 desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS16DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS16, useAppIdentifierEnabled: false).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPad doesn't match"
    )
    XCTAssertEqual(
      iOS16DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS16, useAppIdentifierEnabled: true).build(desktopMode: true, useSafariUA: true),
      "iOS 16 desktop User Agent on iPad doesn't match"
    )

    let iOS17 = OperatingSystemVersion(majorVersion: 17, minorVersion: 0, patchVersion: 1)
    let iOS17DesktopUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/17.0.1 \
      Safari/605.1.15
      """

    XCTAssertEqual(
      iOS17DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS17, useAppIdentifierEnabled: false).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPhone doesn't match"
    )
    XCTAssertEqual(
      iOS17DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS17, useAppIdentifierEnabled: true).build(desktopMode: true, useSafariUA: true),
      "iOS 16 desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS17DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS17, useAppIdentifierEnabled: false).build(desktopMode: true),
      "iOS 16 desktop User Agent on iPad doesn't match"
    )
    XCTAssertEqual(
      iOS17DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS17, useAppIdentifierEnabled: true).build(desktopMode: true, useSafariUA: true),
      "iOS 16 desktop User Agent on iPad doesn't match"
    )
  }

  // Test BraveUserAgentBuilder for specific mobile version with `useAppIdentifierEnabled` enabled
  func testSpecificMobileBraveUA() {
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
      Mozilla/5.0 (iPhone; CPU iPhone OS 17_0_1 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Brave/\(bundleMajorVersion) \
      Mobile/15E148 \
      Safari/604.1
      """

    let iPad_safari_17_UA = """
      Mozilla/5.0 (iPad; CPU OS 17_0_1 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Brave/\(bundleMajorVersion) \
      Mobile/15E148 \
      Safari/604.1
      """

    // MARK: 17.0.1
    let ios17_0_1 = OperatingSystemVersion(majorVersion: 17, minorVersion: 0, patchVersion: 1)

    XCTAssertEqual(
      iPhone_safari_17_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios17_0_1, useAppIdentifierEnabled: true).build(desktopMode: false),
      "Brave user agent for iOS 17.0.1 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_17_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios17_0_1, useAppIdentifierEnabled: true).build(desktopMode: false),
      "Brave user agent for iOS 17.0.1 iPad doesn't match."
    )

    // MARK: - iOS 16
    let iPhone_safari_16_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 16_6 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Brave/\(bundleMajorVersion) \
      Mobile/15E148 \
      Safari/604.1
      """

    let iPad_safari_16_UA = """
      Mozilla/5.0 (iPad; CPU OS 16_6 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Brave/\(bundleMajorVersion) \
      Mobile/15E148 \
      Safari/604.1
      """

    // MARK: 16.1

    let ios16_6 = OperatingSystemVersion(majorVersion: 16, minorVersion: 6, patchVersion: 0)

    XCTAssertEqual(
      iPhone_safari_16_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios16_6, useAppIdentifierEnabled: true).build(desktopMode: false),
      "Brave user agent for iOS 16.6 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_16_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios16_6, useAppIdentifierEnabled: true).build(desktopMode: false),
      "Brave user agent for iOS 16.6 iPad doesn't match."
    )
  }

  // Test BraveUserAgentBuilder for specific mobile version with `useAppIdentifierEnabled` disabled or masked
  func testSpecificMobileUA() {
    // Put specific devices here, look how mobile UA looks like on your device and do hardcoded compare
    // For general UA tests there is another regex based test.
    //
    // For iPads please remember that desktop UA is used by default on 13+,
    // switch to mobile UA in safari before pasting the results here.
    //
    // At the moment, unlike when using Brave user agent, each iOS version has one corresponding
    // Safari UA attached so for example 13.1 and 13.3 won't have the same UA.

    // MARK: - iOS 17
    let iPhone_safari_17_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 17_0_1 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/17.0.1 \
      Mobile/15E148 \
      Safari/604.1
      """

    let iPad_safari_17_UA = """
      Mozilla/5.0 (iPad; CPU OS 17_0_1 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/17.0.1 \
      Mobile/15E148 \
      Safari/604.1
      """

    // MARK: 17.0.1
    let ios17_0_1 = OperatingSystemVersion(majorVersion: 17, minorVersion: 0, patchVersion: 1)

    XCTAssertEqual(
      iPhone_safari_17_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios17_0_1, useAppIdentifierEnabled: false).build(desktopMode: false),
      "User agent for iOS 17.0.1 iPhone doesn't match."
    )
    XCTAssertEqual(
      iPhone_safari_17_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios17_0_1, useAppIdentifierEnabled: true).build(desktopMode: false, useSafariUA: true),
      "User agent for iOS 17.0.1 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_17_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios17_0_1, useAppIdentifierEnabled: false).build(desktopMode: false),
      "User agent for iOS 17.0.1 iPad doesn't match."
    )
    XCTAssertEqual(
      iPad_safari_17_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios17_0_1, useAppIdentifierEnabled: true).build(desktopMode: false, useSafariUA: true),
      "User agent for iOS 17.0.1 iPad doesn't match."
    )

    // MARK: - iOS 16
    let iPhone_safari_16_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 16_6 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/16.6 \
      Mobile/15E148 \
      Safari/604.1
      """

    let iPad_safari_16_UA = """
      Mozilla/5.0 (iPad; CPU OS 16_6 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/16.6 \
      Mobile/15E148 \
      Safari/604.1
      """

    // MARK: 16.1

    let ios16_6 = OperatingSystemVersion(majorVersion: 16, minorVersion: 6, patchVersion: 0)

    XCTAssertEqual(
      iPhone_safari_16_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios16_6, useAppIdentifierEnabled: false).build(desktopMode: false),
      "User agent for iOS 16.6 iPhone doesn't match."
    )
    XCTAssertEqual(
      iPhone_safari_16_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios16_6, useAppIdentifierEnabled: true).build(desktopMode: false, useSafariUA: true),
      "User agent for iOS 16.6 iPhone doesn't match."
    )

    XCTAssertEqual(
      iPad_safari_16_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios16_6, useAppIdentifierEnabled: false).build(desktopMode: false),
      "User agent for iOS 16.6 iPad doesn't match."
    )
    XCTAssertEqual(
      iPad_safari_16_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios16_6, useAppIdentifierEnabled: true).build(desktopMode: false, useSafariUA: true),
      "User agent for iOS 16.6 iPad doesn't match."
    )
  }

  // Test BraveUserAgentBuilder for specific desktop version with `useAppIdentifierEnabled` enabled
  func testFutureProofDesktopBraveUA() {
    let iOS34DesktopBraveUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Brave/\(bundleMajorVersion) \
      Version/34.0 \
      Safari/605.1.15
      """

    let iOS34 = OperatingSystemVersion(majorVersion: 34, minorVersion: 0, patchVersion: 0)

    XCTAssertEqual(
      iOS34DesktopBraveUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS34, useAppIdentifierEnabled: true).build(desktopMode: true),
      "iOS 17 fallback desktop Brave User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS34DesktopBraveUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS34, useAppIdentifierEnabled: true).build(desktopMode: true),
      "iOS 17 fallback desktop Brave User Agent on iPad doesn't match"
    )
  }

  // Test BraveUserAgentBuilder for future desktop version with `useAppIdentifierEnabled` disabled or masked
  func testFutureProofDesktopUA() {
    let iOS34DesktopUA =
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/34.0 \
      Safari/605.1.15
      """

    let iOS34 = OperatingSystemVersion(majorVersion: 34, minorVersion: 0, patchVersion: 0)

    XCTAssertEqual(
      iOS34DesktopUA,
      UserAgentBuilder(device: iPhone, iOSVersion: iOS34, useAppIdentifierEnabled: false).build(desktopMode: true),
      "iOS 17 fallback desktop User Agent on iPhone doesn't match"
    )

    XCTAssertEqual(
      iOS34DesktopUA,
      UserAgentBuilder(device: iPad, iOSVersion: iOS34, useAppIdentifierEnabled: false).build(desktopMode: true),
      "iOS 17 fallback desktop User Agent on iPad doesn't match"
    )
  }

  // Test BraveUserAgentBuilder for future mobile version with `useAppIdentifierEnabled` enabled
  func testFutureProofMobileBraveUA() {
    // MARK: - iPhone iOS 34
    let ios34 = OperatingSystemVersion(majorVersion: 34, minorVersion: 0, patchVersion: 0)
    let iPhone_safari_34_Brave_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 18_7 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Brave/\(bundleMajorVersion) \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPhone_safari_34_Brave_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios34, useAppIdentifierEnabled: true).build(desktopMode: false),
      "User agent for iOS 34.0 iPhone doesn't match."
    )

    let iPad_safari_34_Brave_UA = """
      Mozilla/5.0 (iPad; CPU OS 18_7 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Brave/\(bundleMajorVersion) \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPad_safari_34_Brave_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios34, useAppIdentifierEnabled: true).build(desktopMode: false),
      "User agent for iOS 34.0 iPad doesn't match."
    )
  }

  // Test BraveUserAgentBuilder for future mobile version with `useAppIdentifierEnabled` disabled or masked
  func testFutureProofMobileUA() {
    // MARK: - iPhone iOS 34
    let ios34 = OperatingSystemVersion(majorVersion: 34, minorVersion: 0, patchVersion: 0)
    let iPhone_safari_34_UA = """
      Mozilla/5.0 (iPhone; CPU iPhone OS 18_7 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/34.0 \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPhone_safari_34_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios34, useAppIdentifierEnabled: false).build(desktopMode: false),
      "User agent for iOS 34.0 iPhone doesn't match."
    )
    XCTAssertEqual(
      iPhone_safari_34_UA,
      UserAgentBuilder(device: iPhone, iOSVersion: ios34, useAppIdentifierEnabled: true).build(desktopMode: false, useSafariUA: true),
      "User agent for iOS 34.0 iPhone doesn't match."
    )

    let iPad_safari_34_UA = """
      Mozilla/5.0 (iPad; CPU OS 18_7 like Mac OS X) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      Version/34.0 \
      Mobile/15E148 \
      Safari/604.1
      """

    XCTAssertEqual(
      iPad_safari_34_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios34, useAppIdentifierEnabled: false).build(desktopMode: false),
      "User agent for iOS 34.0 iPad doesn't match."
    )
    XCTAssertEqual(
      iPad_safari_34_UA,
      UserAgentBuilder(device: iPad, iOSVersion: ios34, useAppIdentifierEnabled: true).build(desktopMode: false, useSafariUA: true),
      "User agent for iOS 34.0 iPad doesn't match."
    )
  }
}
