// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Client
import Shared
import BraveShared

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
    
    func testURLSponsoredPath() throws {
        let publicChannels: [AppBuildChannel] = [.release, .beta]
        let privateChannels: [AppBuildChannel] = [.debug, .dev, .enterprise]
        let locales: [Locale] = [.init(identifier: "en_US"), .init(identifier: "pl_PL")]
        
        locales.forEach { locale in
            publicChannels.forEach {
                let type = NTPDownloader.ResourceType.sponsor.resourceBaseURL(for: $0, locale: locale)
                XCTAssertEqual(type, URL(string: "https://mobile-data.s3.brave.com/\(locale.regionCode!)/ios"))
                
                let invalidLocaleType = NTPDownloader.ResourceType.sponsor
                    .resourceBaseURL(for: $0, locale: .init(identifier: "bad locale region code"))
                XCTAssertNil(invalidLocaleType)
            }
            
            privateChannels.forEach {
                let type = NTPDownloader.ResourceType.sponsor.resourceBaseURL(for: $0, locale: locale)
                XCTAssertEqual(type, URL(string: "https://mobile-data-dev.s3.brave.software/\(locale.regionCode!)/ios"))
                
                let invalidLocaleType = NTPDownloader.ResourceType.sponsor
                    .resourceBaseURL(for: $0, locale: .init(identifier: "bad locale region code"))
                XCTAssertNil(invalidLocaleType)
            }
        }
    }
    
    func testURLSuperReferrerPath() throws {
        let publicChannels: [AppBuildChannel] = [.release, .beta]
        let privateChannels: [AppBuildChannel] = [.debug, .dev, .enterprise]
        let locales: [Locale] = [.init(identifier: "en_US"), .init(identifier: "pl_PL")]
        let codes = ["abc", "XXX"]
        
        codes.forEach { code in
            locales.forEach { locale in
                publicChannels.forEach {
                    let type = NTPDownloader.ResourceType.superReferral(code: code).resourceBaseURL(for: $0, locale: locale)
                    XCTAssertEqual(type, URL(string: "https://mobile-data.s3.brave.com/superreferrer/\(code)"))
                }
                
                privateChannels.forEach {
                    let type = NTPDownloader.ResourceType.superReferral(code: code).resourceBaseURL(for: $0, locale: locale)
                    XCTAssertEqual(type, URL(string: "https://mobile-data-dev.s3.brave.software/superreferrer/\(code)"))
                }
            }
        }
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
