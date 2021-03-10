// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Client

class NTPDownloaderTests: XCTestCase {

    func testIsCampaignEnded() throws {
        XCTAssert(NTPDownloader.isCampaignEnded(data: emptyJson.asData))
        XCTAssert(NTPDownloader.isCampaignEnded(data: schemaKeyJson.asData))
        XCTAssert(NTPDownloader.isCampaignEnded(data: noWallpapersJson.asData))
        
        XCTAssertFalse(NTPDownloader.isCampaignEnded(data: validJson.asData))
    }

    // MARK: - Json input
    
    private let emptyJson =
        """
        {
        }
        """
    
    // Regression test:
    // Adding schemakey to empty json caused regression on campaign invalidation logic.
    private let schemaKeyJson =
        """
        {
            "schemaVersion": 1
        }
        """
    
    private let noWallpapersJson =
        """
        {
            "schemaVersion": 1,
            "logo": {
                "imageUrl": "logo.png",
                "alt": "Visit Brave.com",
                "companyName": "Brave",
                "destinationUrl": "https://brave.com"
            },
            "wallpapers": []
        }
        """
    
    // Taken from an old campaign, brand replaced with Brave.
    private let validJson =
        """
        {
            "schemaVersion": 1,
            "logo": {
                "imageUrl": "logo.png",
                "alt": "Visit Brave.com",
                "companyName": "Brave",
                "destinationUrl": "https://brave.com"
            },
            "wallpapers": [
                {
                    "imageUrl": "background-1.jpg",
                    "focalPoint": {
                        "x": 1187,
                        "y": 720
                    },
                    "creativeInstanceId": "749f67dd-f755-48b2-8699-6d214051de20"
                },
                {
                    "imageUrl": "background-2.jpg",
                    "focalPoint": {
                        "x": 1436,
                        "y": 720
                    },
                    "creativeInstanceId": "266b1ade-51fb-4311-9468-b0406faa0288"
                },
                {
                    "imageUrl": "background-3.jpg",
                    "focalPoint": {
                        "x": 905,
                        "y": 720
                    },
                    "creativeInstanceId": "07edaf61-0f9c-47a9-8c0b-26284f92596b"
                }
            ]
        }
        """
}

private extension String {
    var asData: Data {
        Data(self.utf8)
    }
}
