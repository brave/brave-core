// swift-tools-version: 5.7
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription
import Foundation

// News, Playlist (+JS), Onboarding, Browser (Favicons, Bookmarks, History, Passwords, Reader Mode, Settings, Sync),
// VPN, Rewards, Shields (Privacy, De-Amp, Downloaders, Content Blockers, ...), NTP, Networking,

var package = Package(
  name: "Brave",
  defaultLocalization: "en",
  platforms: [.iOS(.v15), .macOS(.v12)],
  products: [
    .library(name: "Brave", targets: ["Brave"]),
    .library(name: "Shared", targets: ["Shared"]),
    .library(name: "BraveCore", targets: ["BraveCore", "MaterialComponents"]),
    .library(name: "BraveShared", targets: ["BraveShared"]),
    .library(name: "BraveShields", targets: ["BraveShields"]),
    .library(name: "BraveUI", targets: ["BraveUI"]),
    .library(name: "DesignSystem", targets: ["DesignSystem"]),
    .library(name: "BraveWallet", targets: ["BraveWallet"]),
    .library(name: "Data", targets: ["Data"]),
    .library(name: "Storage", targets: ["Storage"]),
    .library(name: "BrowserIntentsModels", targets: ["BrowserIntentsModels"]),
    .library(name: "BraveWidgetsModels", targets: ["BraveWidgetsModels"]),
    .library(name: "Strings", targets: ["Strings"]),
    .library(name: "BraveStrings", targets: ["BraveStrings"]),
    .library(name: "BraveVPN", targets: ["BraveVPN"]),
    .library(name: "BraveNews", targets: ["BraveNews"]),
    .library(name: "Favicon", targets: ["Favicon"]),
    .library(name: "FaviconModels", targets: ["FaviconModels"]),
    .library(name: "Onboarding", targets: ["Onboarding"]),
    .library(name: "Growth", targets: ["Growth"]),
    .library(name: "RuntimeWarnings", targets: ["RuntimeWarnings"]),
    .library(name: "CodableHelpers", targets: ["CodableHelpers"]),
    .library(name: "GRDWireGuardKit", targets: ["GRDWireGuardKit"]),
    .library(name: "Preferences", targets: ["Preferences"]),
    .library(name: "PrivateCDN", targets: ["PrivateCDN"]),
    .library(name: "CertificateUtilities", targets: ["CertificateUtilities"]),
    .library(name: "Playlist", targets: ["Playlist"]),
    .library(name: "UserAgent", targets: ["UserAgent"]),
    .executable(name: "LeoAssetCatalogGenerator", targets: ["LeoAssetCatalogGenerator"]),
    .plugin(name: "IntentBuilderPlugin", targets: ["IntentBuilderPlugin"]),
    .plugin(name: "LoggerPlugin", targets: ["LoggerPlugin"]),
    .plugin(name: "LeoAssetsPlugin", targets: ["LeoAssetsPlugin"])
  ],
  dependencies: [
    .package(url: "https://github.com/SnapKit/SnapKit", from: "5.0.1"),
    .package(url: "https://github.com/cezheng/Fuzi", from: "3.1.3"),
    .package(url: "https://github.com/SwiftyJSON/SwiftyJSON", from: "5.0.0"),
    .package(url: "https://github.com/airbnb/lottie-ios", from: "3.1.9"),
    .package(url: "https://github.com/SDWebImage/SDWebImage", exact: "5.10.3"),
    .package(url: "https://github.com/SDWebImage/SDWebImageSwiftUI", from: "2.2.0"),
    .package(url: "https://github.com/nmdias/FeedKit", from: "9.1.2"),
    .package(url: "https://github.com/brave/PanModal", revision: "e67e9eff53c05f19b41bbb2ca7d27ff5859a586c"),
    .package(url: "https://github.com/apple/swift-collections", from: "1.0.0"),
    .package(url: "https://github.com/siteline/SwiftUI-Introspect", from: "0.1.3"),
    .package(url: "https://github.com/apple/swift-algorithms", from: "1.0.0"),
    .package(url: "https://github.com/devxoul/Then", from: "2.7.0"),
    .package(url: "https://github.com/mkrd/Swift-BigInt", from: "2.0.0"),
    .package(url: "https://github.com/GuardianFirewall/GuardianConnect", exact: "1.8.5"),
    .package(url: "https://github.com/pointfreeco/swift-custom-dump", from: "0.6.0"),
    .package(name: "Static", path: "ThirdParty/Static"),
  ],
  targets: [
    .target(
      name: "Shared",
      dependencies: [
        "BraveCore",
        "MaterialComponents",
        "Strings",
        "SwiftyJSON",
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "BraveShared",
      dependencies: ["BraveCore", "Shared"],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "CertificateUtilities",
      dependencies: ["Shared"],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(
      name: "CertificateUtilitiesTests",
      dependencies: ["CertificateUtilities", "BraveShared", "BraveCore", "MaterialComponents"],
      exclude: [ "Certificates/self-signed.conf" ],
      resources: [
        .copy("Certificates/certviewer/brave.com.cer"),
        .copy("Certificates/certviewer/github.com.cer"),
      ]
    ),
    .target(name: "BraveStrings", dependencies: ["Strings", "Preferences"]),
    .target(
      name: "Growth",
      dependencies: ["BraveVPN", "Shared", "BraveShared", "Strings", "SnapKit", "CertificateUtilities"],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "BraveUI",
      dependencies: [
        "Strings",
        "DesignSystem",
        "PanModal",
        "SDWebImage",
        "SnapKit",
        .product(name: "Introspect", package: "SwiftUI-Introspect"),
        "Then",
        "Static",
        "Preferences",
        "Shared",
        .product(name: "Lottie", package: "lottie-ios")
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(name: "BraveShields", dependencies: ["Strings", "Preferences"], plugins: ["LoggerPlugin"]),
    .target(name: "DesignSystem", plugins: ["LeoAssetsPlugin"]),
    .binaryTarget(name: "BraveCore", path: "node_modules/brave-core-ios/BraveCore.xcframework"),
    .binaryTarget(name: "MaterialComponents", path: "node_modules/brave-core-ios/MaterialComponents.xcframework"),
    .binaryTarget(name: "GRDWireGuardKit", path: "ThirdParty/GRDWireGuardKit/GRDWireGuardKit.xcframework"),
    .target(
      name: "Storage",
      dependencies: ["Shared"],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "Data",
      dependencies: ["BraveShields", "Storage", "Strings", "Preferences", "Shared"],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "BraveWallet",
      dependencies: [
        "Data",
        "BraveCore",
        "MaterialComponents",
        "BraveShared",
        "BraveUI",
        "DesignSystem",
        "Favicon",
        "Strings",
        "PanModal",
        "SDWebImageSwiftUI",
        "SnapKit",
        "Then",
        "Shared",
        "BraveStrings",
        .product(name: "BigNumber", package: "Swift-BigInt"),
        .product(name: "Algorithms", package: "swift-algorithms"),
        .product(name: "Collections", package: "swift-collections"),
        .product(name: "Introspect", package: "SwiftUI-Introspect"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "BrowserIntentsModels",
      sources: ["BrowserIntents.intentdefinition", "CustomIntentHandler.swift"],
      plugins: ["IntentBuilderPlugin"]
    ),
    .target(
      name: "BraveWidgetsModels",
      dependencies: ["FaviconModels"],
      sources: ["BraveWidgets.intentdefinition", "LockScreenFavoriteIntentHandler.swift", "FavoritesWidgetData.swift"],
      plugins: ["IntentBuilderPlugin", "LoggerPlugin"]
    ),
    .target(name: "TestHelpers", dependencies: ["Data", "BraveShared"]),
    .target(
      name: "BraveVPN",
      dependencies: [
        "BraveStrings",
        "SnapKit",
        "Then",
        "Data",
        "GuardianConnect",
        "BraveUI",
        .product(name: "Lottie", package: "lottie-ios")
      ],
      resources: [.copy("vpncheckmark.json")],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "BraveNews",
      dependencies: [
        "BraveCore",
        "BraveShared",
        "BraveStrings",
        "BraveUI",
        "CodableHelpers",
        "Data",
        "DesignSystem",
        "FeedKit",
        "Fuzi",
        "Growth",
        "Preferences",
        "Shared",
        "SnapKit",
        "Storage",
        "Strings",
        "Then",
        .product(name: "Collections", package: "swift-collections"),
        .product(name: "Introspect", package: "SwiftUI-Introspect"),
        .product(name: "Lottie", package: "lottie-ios"),
      ],
      resources: [
        .copy("Lottie Assets/brave-today-welcome-graphic.json"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(name: "Preferences", dependencies: ["Shared"], plugins: ["LoggerPlugin"]),
    .target(
      name: "Onboarding",
      dependencies: [
        "BraveCore",
        "BraveShared",
        "BraveStrings",
        "BraveUI",
        "DesignSystem",
        "Growth",
        .product(name: "Lottie", package: "lottie-ios"),
        "Preferences",
        "Shared",
        "SnapKit",
        "Storage",
      ],
      resources: [
        .copy("LottieAssets/onboarding-ads.json"),
        .copy("LottieAssets/onboarding-rewards.json"),
        .copy("LottieAssets/onboarding-shields.json"),
        .copy("LottieAssets/playlist-confetti.json"),
        .copy("Welcome/Resources/disconnect-entitylist.json"),
        .copy("ProductNotifications/Resources/blocking-summary.json")
      ],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(name: "BraveNewsTests", dependencies: ["BraveNews"], resources: [
      .copy("opml-test-files/subscriptionList.opml"),
      .copy("opml-test-files/states.opml"),
    ]),
    .target(name: "CodableHelpers"),
    .target(name: "FaviconModels", dependencies: ["Shared"]),
    .target(
      name: "Favicon",
      dependencies: [
        "FaviconModels",
        "BraveCore",
        "BraveShared",
        "Shared",
        "SDWebImage",
      ],
      resources: [
        .copy("Assets/top_sites.json"),
        .copy("Assets/TopSites")
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(name: "UserAgent", dependencies: ["Preferences"]),
    .testTarget(name: "UserAgentTests", dependencies: ["UserAgent", "Brave"]),
    .testTarget(name: "SharedTests", dependencies: ["Shared"]),
    .testTarget(
      name: "BraveSharedTests",
      dependencies: ["BraveShared", "Preferences"]
    ),
    .testTarget(
      name: "BraveVPNTests",
      dependencies: ["BraveVPN", "BraveShared", "GuardianConnect"]
    ),
    .testTarget(
      name: "BraveWalletTests",
      dependencies: [
        "BraveWallet",
        "TestHelpers",
        .product(name: "CustomDump", package: "swift-custom-dump")
      ]
    ),
    .testTarget(name: "StorageTests", dependencies: ["Storage", "TestHelpers"], resources: [.copy("fixtures/v33.db"), .copy("testcert1.pem"), .copy("testcert2.pem")]),
    .testTarget(name: "DataTests", dependencies: ["Data", "TestHelpers"]),
    .testTarget(
      name: "ClientTests",
      dependencies: ["Brave", "BraveStrings"],
      resources: [
        .copy("Resources/debouncing.json"),
        .copy("Resources/content-blocking.json"),
        .copy("Resources/filter-lists.json"),
        .copy("Resources/google-search-plugin.xml"),
        .copy("Resources/duckduckgo-search-plugin.xml"),
        .copy("Resources/ad-block-resources/resources.json"),
        .copy("Resources/filter-rules/cdbbhgbmjhfnhnmgeddbliobbofkgdhe.txt"),
        .copy("Resources/filter-rules/cffkpbalmllkdoenhmdmpbkajipdjfam.dat"),
        .copy("Resources/html/index.html"),
        .copy("Resources/scripts/farbling-tests.js"),
        .copy("Resources/scripts/request-blocking-tests.js"),
        .copy("Resources/scripts/cosmetic-filter-tests.js"),
        .copy("blocking-summary-test.json"),
      ]
    ),
    .target(name: "Strings"),
    .target(name: "RuntimeWarnings"),
    .target(name: "PrivateCDN", dependencies: ["SDWebImage"]),
    .target(
      name: "Playlist",
      dependencies: ["Data", "BraveShared", "Shared", "Storage", "Preferences", "Strings", "CodableHelpers", "UserAgent", "Then"],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(name: "PrivateCDNTests", dependencies: ["PrivateCDN"]),
    .testTarget(name: "GrowthTests", dependencies: ["Growth", "Shared", "BraveShared", "BraveVPN"]),
    .plugin(name: "IntentBuilderPlugin", capability: .buildTool()),
    .plugin(name: "LoggerPlugin", capability: .buildTool()),
    .plugin(name: "LeoAssetsPlugin", capability: .buildTool()/*, dependencies: ["LeoAssetCatalogGenerator"]*/),
    .executableTarget(name: "LeoAssetCatalogGenerator")
  ],
  cxxLanguageStandard: .cxx17
)

var braveTarget: PackageDescription.Target = .target(
  name: "Brave",
  dependencies: [
    "BraveShared",
    "Shared",
    "BraveWallet",
    "BraveCore",
    "MaterialComponents",
    "BraveUI",
    "DesignSystem",
    "Data",
    "Storage",
    "Fuzi",
    "SnapKit",
    "Static",
    "SDWebImage",
    "Then",
    "SwiftyJSON",
    "BrowserIntentsModels",
    "BraveWidgetsModels",
    "BraveVPN",
    "BraveNews",
    "Onboarding",
    "Growth",
    "CodableHelpers",
    "Preferences",
    "Favicon",
    "CertificateUtilities",
    "Playlist",
    "UserAgent",
    .product(name: "Lottie", package: "lottie-ios"),
    .product(name: "Collections", package: "swift-collections"),
  ],
  exclude: [
    "Frontend/UserContent/UserScripts/AllFrames",
    "Frontend/UserContent/UserScripts/MainFrame",
    "Frontend/UserContent/UserScripts/Sandboxed",
  ],
  resources: [
    .copy("Assets/About/Licenses.html"),
    .copy("Assets/__firefox__.js"),
    .copy("Assets/AllFramesAtDocumentEnd.js"),
    .copy("Assets/AllFramesAtDocumentEndSandboxed.js"),
    .copy("Assets/AllFramesAtDocumentStart.js"),
    .copy("Assets/AllFramesAtDocumentStartSandboxed.js"),
    .copy("Assets/MainFrameAtDocumentEnd.js"),
    .copy("Assets/MainFrameAtDocumentEndSandboxed.js"),
    .copy("Assets/MainFrameAtDocumentStart.js"),
    .copy("Assets/MainFrameAtDocumentStartSandboxed.js"),
    .copy("Assets/SessionRestore.html"),
    .copy("Assets/Fonts/FiraSans-Bold.ttf"),
    .copy("Assets/Fonts/FiraSans-BoldItalic.ttf"),
    .copy("Assets/Fonts/FiraSans-Book.ttf"),
    .copy("Assets/Fonts/FiraSans-Italic.ttf"),
    .copy("Assets/Fonts/FiraSans-Light.ttf"),
    .copy("Assets/Fonts/FiraSans-Medium.ttf"),
    .copy("Assets/Fonts/FiraSans-Regular.ttf"),
    .copy("Assets/Fonts/FiraSans-SemiBold.ttf"),
    .copy("Assets/Fonts/FiraSans-UltraLight.ttf"),
    .copy("Assets/Fonts/NewYorkMedium-Bold.otf"),
    .copy("Assets/Fonts/NewYorkMedium-BoldItalic.otf"),
    .copy("Assets/Fonts/NewYorkMedium-Regular.otf"),
    .copy("Assets/Fonts/NewYorkMedium-RegularItalic.otf"),
    .copy("Assets/Interstitial Pages/Pages/CertificateError.html"),
    .copy("Assets/Interstitial Pages/Pages/GenericError.html"),
    .copy("Assets/Interstitial Pages/Pages/NetworkError.html"),
    .copy("Assets/Interstitial Pages/Pages/Web3Domain.html"),
    .copy("Assets/Interstitial Pages/Pages/IPFSPreference.html"),
    .copy("Assets/Interstitial Pages/Images/Carret.png"),
    .copy("Assets/Interstitial Pages/Images/Clock.svg"),
    .copy("Assets/Interstitial Pages/Images/Cloud.svg"),
    .copy("Assets/Interstitial Pages/Images/DarkWarning.svg"),
    .copy("Assets/Interstitial Pages/Images/Generic.svg"),
    .copy("Assets/Interstitial Pages/Images/Globe.svg"),
    .copy("Assets/Interstitial Pages/Images/Info.svg"),
    .copy("Assets/Interstitial Pages/Images/Warning.svg"),
    .copy("Assets/Interstitial Pages/Images/BraveIPFS.svg"),
    .copy("Assets/Interstitial Pages/Images/IPFSBackground.svg"),
    .copy("Assets/Interstitial Pages/Styles/CertificateError.css"),
    .copy("Assets/Interstitial Pages/Styles/InterstitialStyles.css"),
    .copy("Assets/Interstitial Pages/Styles/NetworkError.css"),
    .copy("Assets/Interstitial Pages/Styles/Web3Domain.css"),
    .copy("Assets/Interstitial Pages/Styles/IPFSPreference.css"),
    .copy("Assets/SearchPlugins"),
    .copy("Frontend/Reader/Reader.css"),
    .copy("Frontend/Reader/Reader.html"),
    .copy("Frontend/Reader/ReaderViewLoading.html"),
    .copy("Frontend/Browser/New Tab Page/Backgrounds/Assets/NTP_Images/corwin-prescott-3.jpg"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveSearchScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveSkusScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/nacl.min.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/PlaylistFolderSharingScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/FrameCheckWrapper.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/CookieControlScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/FarblingProtectionScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/gpc.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/MediaBackgroundingScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/PlaylistScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/PlaylistSwizzlerScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/ReadyStateScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/RequestBlockingScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/TrackingProtectionStats.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/RewardsReportingScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/WalletEthereumProviderScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/WalletSolanaProviderScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/YoutubeQualityScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/DeAmpScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/FaviconScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/ResourceDownloaderScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/SelectorsPollerScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/SiteStateListenerScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/WindowRenderScript.js"),
    .copy("WebFilters/ContentBlocker/build-disconnect.py"),
    .copy("WebFilters/ContentBlocker/Lists/block-ads.json"),
    .copy("WebFilters/ContentBlocker/Lists/block-cookies.json"),
    .copy("WebFilters/ContentBlocker/Lists/block-trackers.json"),
    .copy("WebFilters/ShieldStats/Adblock/Resources/ABPFilterParserData.dat"),
  ],
  plugins: ["LoggerPlugin"]
)

// Keeping this in case we need to disable it at any time
// Can use `BRAVE_APPSTORE_BUILD` environment like before to disable only on Release builds as well
let isNativeTalkEnabled = true

if isNativeTalkEnabled {
  // Not a release build, add BraveTalk integrations
  braveTarget.dependencies.append("BraveTalk")
  braveTarget.resources?.append(
    PackageDescription.Resource.copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveTalkScript.js")
  )
  package.dependencies.append(.package(name: "JitsiMeet", path: "ThirdParty/JitsiMeet"))
  package.products.append(.library(name: "BraveTalk", targets: ["BraveTalk"]))
  package.targets.append(contentsOf: [
    .target(name: "BraveTalk", dependencies: ["Shared", "JitsiMeet"], plugins: ["LoggerPlugin"]),
    .testTarget(name: "BraveTalkTests", dependencies: ["BraveTalk", "Shared"]),
  ])
}

package.targets.append(braveTarget)
