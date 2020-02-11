// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import WebKit
@testable import Shared

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
        let iOS12DesktopUA =
        """
        Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_6) \
        AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/12.1.2 \
        Safari/605.1.15
        """
        
        let iOS13DesktopUA =
        """
        Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15) \
        AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/13.1 \
        Safari/605.1.15
        """
        
        let iOS12 = OperatingSystemVersion(majorVersion: 12, minorVersion: 0, patchVersion: 0)
        
        XCTAssertEqual(iOS12DesktopUA,
                       UserAgentBuilder(device: iPhone, iOSVersion: iOS12).build(desktopMode: true),
                       "iOS 12 desktop User Agent on iPhone doesn't match")
        
        XCTAssertEqual(iOS12DesktopUA,
                       UserAgentBuilder(device: iPad, iOSVersion: iOS12).build(desktopMode: true),
                       "iOS 12 desktop User Agent on iPad doesn't match")
        
        let iOS13 = OperatingSystemVersion(majorVersion: 13, minorVersion: 0, patchVersion: 0)
        
        XCTAssertEqual(iOS13DesktopUA,
                       UserAgentBuilder(device: iPhone, iOSVersion: iOS13).build(desktopMode: true),
                       "iOS 13 desktop User Agent on iPhone doesn't match")
        
        XCTAssertEqual(iOS13DesktopUA,
                       UserAgentBuilder(device: iPad, iOSVersion: iOS13).build(desktopMode: true),
                       "iOS 13 desktop User Agent on iPad doesn't match")
    }
    
    func testSpecificMobileUA() {
        // Put specific devices here, look how mobile UA looks like on your device and do hardcoded compare
        // For general UA tests there is another regex based test.
        //
        // For iPads please remember that desktop UA is used by default on 13+,
        // switch to mobile UA in safari before pasting the results here.
        
        // MARK: iPhone 13.3.1
        let ios13_3_1 = OperatingSystemVersion(majorVersion: 13, minorVersion: 3, patchVersion: 1)
        let iPhone_safari_13_3_1_UA = """
        Mozilla/5.0 (iPhone; CPU iPhone OS 13_3_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/13.0.5 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPhone_safari_13_3_1_UA,
                       UserAgentBuilder(device: iPhone, iOSVersion: ios13_3_1).build(desktopMode: false),
                       "User agent for iOS 13.3.1 iPhone doesn't match.")
        
        // MARK: iPad 13.3.1
        let iPad_safari_13_3_1_UA = """
        Mozilla/5.0 (iPad; CPU OS 13_3_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/13.0.5 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPad_safari_13_3_1_UA,
                       UserAgentBuilder(device: iPad, iOSVersion: ios13_3_1).build(desktopMode: false),
                       "User agent for iOS 13.3.1 iPad doesn't match.")
        
        // MARK: iPhone 12.4
        let ios12_4 = OperatingSystemVersion(majorVersion: 12, minorVersion: 4, patchVersion: 0)
        let iPhone_safari_12_4_UA = """
        Mozilla/5.0 (iPhone; CPU iPhone OS 12_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/12.1.2 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPhone_safari_12_4_UA,
                       UserAgentBuilder(device: iPhone, iOSVersion: ios12_4).build(desktopMode: false),
                       "User agent for iOS 12.4 iPhone doesn't match.")
        
        // MARK: iPad 12.4
        let iPad_safari_12_4_UA = """
        Mozilla/5.0 (iPad; CPU OS 12_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/12.1.2 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPad_safari_12_4_UA,
                       UserAgentBuilder(device: iPad, iOSVersion: ios12_4).build(desktopMode: false),
                       "User agent for iOS 12.4 iPad doesn't match.")
    }
    
    func testFutureProofDesktopUA() {
        let iOS13DesktopUA =
        """
        Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15) \
        AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/13.1 \
        Safari/605.1.15
        """
        
        let iOS14 = OperatingSystemVersion(majorVersion: 14, minorVersion: 0, patchVersion: 0)
        
        XCTAssertEqual(iOS13DesktopUA,
                       UserAgentBuilder(device: iPhone, iOSVersion: iOS14).build(desktopMode: true),
                       "iOS 14 fallback desktop User Agent on iPhone doesn't match")
        
        XCTAssertEqual(iOS13DesktopUA,
                       UserAgentBuilder(device: iPad, iOSVersion: iOS14).build(desktopMode: true),
                       "iOS 14 fallback desktop User Agent on iPad doesn't match")
    }
    
    func testFutureProofMobileUA() {
        // MARK: - iPhone iOS 14
        let ios14 = OperatingSystemVersion(majorVersion: 14, minorVersion: 0, patchVersion: 0)
        let iPhone_safari_14_UA = """
        Mozilla/5.0 (iPhone; CPU iPhone OS 14_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/14.0 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPhone_safari_14_UA,
                       UserAgentBuilder(device: iPhone, iOSVersion: ios14).build(desktopMode: false),
                       "User agent for iOS 14.0 iPhone doesn't match.")
        
        // MARK: - iPad iOS 14
        let iPad_safari_14_UA = """
        Mozilla/5.0 (iPad; CPU OS 14_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/14.0 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPad_safari_14_UA,
                       UserAgentBuilder(device: iPad, iOSVersion: ios14).build(desktopMode: false),
                       "User agent for iOS 14.0 iPad doesn't match.")
        
        // MARK: - iPhone iOS 13, non existent version(13.5.1)
        let ios13_5_1 = OperatingSystemVersion(majorVersion: 13, minorVersion: 5, patchVersion: 1)
        let iPhone_safari_13_5_1_UA = """
        Mozilla/5.0 (iPhone; CPU iPhone OS 13_5_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/13.1 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPhone_safari_13_5_1_UA,
                       UserAgentBuilder(device: iPhone, iOSVersion: ios13_5_1).build(desktopMode: false),
                       "User agent for non existent iOS 13.5.1 iPhone doesn't match.")
        
        // MARK: - iPad iOS 13, non existent version(13.5.1)
        let iPad_safari_13_5_1_UA = """
        Mozilla/5.0 (iPad; CPU OS 13_5_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/13.1 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPad_safari_13_5_1_UA,
                       UserAgentBuilder(device: iPad, iOSVersion: ios13_5_1).build(desktopMode: false),
                       "User agent for non existent iOS 13.5.1 iPad doesn't match.")
        
        // MARK: - iPhone iOS 12, non existent version(12.6)
        let ios12_6 = OperatingSystemVersion(majorVersion: 12, minorVersion: 6, patchVersion: 0)
        let iPhone_safari_12_6_UA = """
        Mozilla/5.0 (iPhone; CPU iPhone OS 12_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/12.1.2 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPhone_safari_12_6_UA,
                       UserAgentBuilder(device: iPhone, iOSVersion: ios12_6).build(desktopMode: false),
                       "User agent for non existent iOS 13.5.1 iPhone doesn't match.")
        
        // MARK: - iPad iOS 12, non existent version(12.6)
        let iPad_safari_12_6_UA = """
        Mozilla/5.0 (iPad; CPU OS 12_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/12.1.2 \
        Mobile/15E148 \
        Safari/604.1
        """
        
        XCTAssertEqual(iPad_safari_12_6_UA,
                       UserAgentBuilder(device: iPad, iOSVersion: ios12_6).build(desktopMode: false),
                       "User agent for non existent iOS 13.5.1 iPad doesn't match.")
    }
    
    func testUADifferences() {
        let ios12_1_1 = OperatingSystemVersion(majorVersion: 12, minorVersion: 1, patchVersion: 1)
        let ios12_1_3 = OperatingSystemVersion(majorVersion: 12, minorVersion: 1, patchVersion: 3)
        let ios13_1_1 = OperatingSystemVersion(majorVersion: 13, minorVersion: 1, patchVersion: 1)
        
        // Minor version difference
        XCTAssertNotEqual(UserAgentBuilder(device: iPhone, iOSVersion: ios12_1_1).build(desktopMode: false),
                          UserAgentBuilder(device: iPhone, iOSVersion: ios12_1_3).build(desktopMode: false))
        
        // Major version difference
        XCTAssertNotEqual(UserAgentBuilder(device: iPhone, iOSVersion: ios12_1_1).build(desktopMode: false),
                          UserAgentBuilder(device: iPhone, iOSVersion: ios13_1_1).build(desktopMode: false))
        
        // Device difference
        XCTAssertNotEqual(UserAgentBuilder(device: iPhone, iOSVersion: ios13_1_1).build(desktopMode: false),
                          UserAgentBuilder(device: iPad, iOSVersion: ios13_1_1).build(desktopMode: false))
        
        // Desktop mode difference
        XCTAssertNotEqual(UserAgentBuilder(device: iPhone, iOSVersion: ios13_1_1).build(desktopMode: false),
                          UserAgentBuilder(device: iPhone, iOSVersion: ios13_1_1).build(desktopMode: true))
    }
    
}
