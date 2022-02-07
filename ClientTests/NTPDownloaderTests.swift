// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Client

class NTPDownloaderTests: XCTestCase {

    func testIsSuperReferralCampaignEnded() throws {
        XCTAssert(NTPDownloader.isSuperReferralCampaignEnded(data: emptyJson.asData))
        XCTAssert(NTPDownloader.isSuperReferralCampaignEnded(data: schemaKeyJson.asData))
        XCTAssert(NTPDownloader.isSuperReferralCampaignEnded(data: noWallpapersJson.asData))

        XCTAssertFalse(NTPDownloader.isSuperReferralCampaignEnded(data: validSuperReferralJson.asData))
    }

    func testIsSponsorCampaignEnded() throws {
        XCTAssert(NTPDownloader.isSponsorCampaignEnded(data: emptyJson.asData))
        XCTAssert(NTPDownloader.isSponsorCampaignEnded(data: schemaKeyJson.asData))
        XCTAssert(NTPDownloader.isSponsorCampaignEnded(data: noCampaignsJson.asData))

        XCTAssertFalse(NTPDownloader.isSponsorCampaignEnded(data: validWallpapersJson.asData))
        XCTAssertFalse(NTPDownloader.isSponsorCampaignEnded(data: validCampaignsJson.asData))
        XCTAssertFalse(NTPDownloader.isSponsorCampaignEnded(data: validWallpapersAndCampaignsJson.asData))
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

    private let noCampaignsJson =
        """
        {
            "schemaVersion": 1,
            "campaigns": []
        }
        """

    // Taken from an old campaign, brand replaced with Brave.
    private let validSuperReferralJson =
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

    // Taken from an old campaign, brand replaced with Brave.
    private let validWallpapersJson =
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

    // Taken from an old campaign, brand replaced with Brave.
    private let validCampaignsJson =
        """
        {
            "schemaVersion": 1,
            "campaigns": [
                {
                    "logo": {
                        "imageUrl": "logo.png",
                        "alt": "Visit Brave.com",
                        "destinationUrl": "https://brave.com",
                        "companyName": "Brave"
                    },
                    "wallpapers": [
                        {
                            "imageUrl": "background-1.jpg",
                            "focalPoint": {
                                "x": 696,
                                "y": 691
                            },
                            "creativeInstanceId": "18a88702-d137-4327-ab76-5fcace4c870a"
                        },
                        {
                            "imageUrl": "background-2.jpg",
                            "logo": {
                                "imageUrl": "logo-2.png",
                                "alt": "Visit basicattentiontoken.org",
                                "companyName": "BAT",
                                "destinationUrl": "https://basicattentiontoken.org"
                            }
                        },
                        {
                            "imageUrl": "background-3.jpg",
                            "focalPoint": {}
                        }
                    ]
                },
                {
                    "logo": {
                        "imageUrl": "logo.png",
                        "alt": "Visit Brave.com",
                        "destinationUrl": "https://brave.com",
                        "companyName": "Brave"
                    },
                    "wallpapers": [
                        {
                            "imageUrl": "background-4.jpg",
                            "focalPoint": {
                                "x": 696,
                                "y": 691
                            }
                        },
                        {
                            "imageUrl": "background-5.jpg",
                            "logo": {
                                "imageUrl": "logo.png",
                                "alt": "Visit Brave.com",
                                "destinationUrl": "https://brave.com",
                                "companyName": "Brave"
                            },
                            "creativeInstanceId": "54774092-04bf-45fd-86e3-9098ec418f6b"
                        }
                    ]
                }
            ]
        }
        """

    // Taken from an old campaign, brand replaced with Brave.
    private let validWallpapersAndCampaignsJson =
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
            ],
            "campaigns": [
                {
                    "logo": {
                        "imageUrl": "logo.png",
                        "alt": "Visit Brave.com",
                        "destinationUrl": "https://brave.com",
                        "companyName": "Brave"
                    },
                    "wallpapers": [
                        {
                            "imageUrl": "background-1.jpg",
                            "focalPoint": {
                                "x": 696,
                                "y": 691
                            },
                            "creativeInstanceId": "18a88702-d137-4327-ab76-5fcace4c870a"
                        },
                        {
                            "imageUrl": "background-2.jpg",
                            "logo": {
                                "imageUrl": "logo-2.png",
                                "alt": "Visit basicattentiontoken.org",
                                "companyName": "BAT",
                                "destinationUrl": "https://basicattentiontoken.org"
                            }
                        },
                        {
                            "imageUrl": "background-3.jpg",
                            "focalPoint": {}
                        }
                    ]
                },
                {
                    "logo": {
                        "imageUrl": "logo.png",
                        "alt": "Visit Brave.com",
                        "destinationUrl": "https://brave.com",
                        "companyName": "Brave"
                    },
                    "wallpapers": [
                        {
                            "imageUrl": "background-4.jpg",
                            "focalPoint": {
                                "x": 696,
                                "y": 691
                            }
                        },
                        {
                            "imageUrl": "background-5.jpg",
                            "logo": {
                                "imageUrl": "logo.png",
                                "alt": "Visit Brave.com",
                                "destinationUrl": "https://brave.com",
                                "companyName": "Brave"
                            },
                            "creativeInstanceId": "54774092-04bf-45fd-86e3-9098ec418f6b"
                        }
                    ]
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
