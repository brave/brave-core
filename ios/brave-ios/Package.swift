// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import Foundation
import PackageDescription

// News, Playlist (+JS), Onboarding, Browser (Favicons, Bookmarks, History, Passwords, Reader Mode, Settings, Sync),
// VPN, Rewards, Shields (Privacy, De-Amp, Downloaders, Content Blockers, ...), NTP, Networking,

var package = Package(
  name: "Brave",
  defaultLocalization: "en",
  platforms: [.iOS(.v16), .macOS(.v13)],
  products: [
    .library(name: "Brave", targets: ["Brave"]),
    .library(name: "Shared", targets: ["Shared"]),
    .library(name: "BraveCore", targets: ["BraveCore", "MaterialComponents"]),
    .library(name: "BraveShared", targets: ["BraveShared"]),
    .library(name: "BraveShields", targets: ["BraveShields"]),
    .library(name: "BraveUI", targets: ["BraveUI"]),
    .library(name: "DesignSystem", targets: ["DesignSystem", "NalaAssets"]),
    .library(name: "BraveWallet", targets: ["BraveWallet"]),
    .library(name: "Data", targets: ["Data"]),
    .library(name: "Storage", targets: ["Storage"]),
    .library(name: "BrowserIntentsModels", targets: ["BrowserIntentsModels"]),
    .library(name: "BraveWidgetsModels", targets: ["BraveWidgetsModels"]),
    .library(name: "Strings", targets: ["Strings"]),
    .library(name: "BraveStrings", targets: ["BraveStrings"]),
    .library(name: "BraveVPN", targets: ["BraveVPN"]),
    .library(name: "BraveNews", targets: ["BraveNews"]),
    .library(name: "AIChat", targets: ["AIChat"]),
    .library(name: "BraveStore", targets: ["BraveStore"]),
    .library(name: "Favicon", targets: ["Favicon"]),
    .library(name: "FaviconModels", targets: ["FaviconModels"]),
    .library(name: "SpeechRecognition", targets: ["SpeechRecognition"]),
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
    .library(name: "CredentialProviderUI", targets: ["CredentialProviderUI"]),
    .library(name: "PlaylistUI", targets: ["PlaylistUI"]),
    .executable(name: "LeoAssetCatalogGenerator", targets: ["LeoAssetCatalogGenerator"]),
    .plugin(name: "IntentBuilderPlugin", targets: ["IntentBuilderPlugin"]),
    .plugin(name: "LoggerPlugin", targets: ["LoggerPlugin"]),
    .plugin(name: "LeoAssetsPlugin", targets: ["LeoAssetsPlugin"]),
  ],
  dependencies: [
    .package(url: "https://github.com/SnapKit/SnapKit", from: "5.0.1"),
    .package(url: "https://github.com/cezheng/Fuzi", from: "3.1.3"),
    .package(url: "https://github.com/SwiftyJSON/SwiftyJSON", from: "5.0.0"),
    .package(url: "https://github.com/airbnb/lottie-spm", from: "4.4.3"),
    .package(url: "https://github.com/SDWebImage/SDWebImage", exact: "5.10.3"),
    .package(url: "https://github.com/SDWebImage/SDWebImageSwiftUI", from: "2.2.0"),
    .package(url: "https://github.com/nmdias/FeedKit", from: "9.1.2"),
    .package(
      url: "https://github.com/brave/PanModal",
      revision: "e67e9eff53c05f19b41bbb2ca7d27ff5859a586c"
    ),
    .package(url: "https://github.com/apple/swift-collections", from: "1.0.0"),
    .package(url: "https://github.com/siteline/SwiftUI-Introspect", from: "0.1.3"),
    .package(url: "https://github.com/apple/swift-algorithms", from: "1.0.0"),
    .package(url: "https://github.com/devxoul/Then", from: "2.7.0"),
    .package(url: "https://github.com/mkrd/Swift-BigInt", from: "2.3.0"),
    .package(url: "https://github.com/GuardianFirewall/GuardianConnect", exact: "1.9.3"),
    .package(url: "https://github.com/pointfreeco/swift-custom-dump", from: "0.6.0"),
    .package(
      url: "https://github.com/venmo/Static",
      revision: "622a6804d39515600ead16e6259cb5d5e50f40df"
    ),
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
      dependencies: ["BraveCore", "Shared", "Preferences"],
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
      exclude: ["Certificates/self-signed.conf"],
      resources: [
        .copy("Certificates/certviewer/brave.com.cer"),
        .copy("Certificates/certviewer/github.com.cer"),
      ]
    ),
    .target(name: "BraveStrings", dependencies: ["Strings", "Preferences"]),
    .target(
      name: "Growth",
      dependencies: [
        "BraveVPN", "Shared", "BraveShared", "Strings", "SnapKit", "CertificateUtilities",
        .product(name: "OrderedCollections", package: "swift-collections"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "SpeechRecognition",
      dependencies: ["BraveUI", "Shared", "BraveShared", "Preferences", "Data"],
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
        .product(name: "Lottie", package: "lottie-spm"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "BraveShields",
      dependencies: ["Strings", "Preferences", "BraveCore"],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "DesignSystem",
      dependencies: ["Then", "NalaAssets"],
      plugins: ["LeoAssetsPlugin"]
    ),
    .binaryTarget(name: "NalaAssets", path: "../../../out/ios_current_link/NalaAssets.xcframework"),
    .binaryTarget(name: "BraveCore", path: "../../../out/ios_current_link/BraveCore.xcframework"),
    .binaryTarget(
      name: "MaterialComponents",
      path: "../../../out/ios_current_link/MaterialComponents.xcframework"
    ),
    .binaryTarget(
      name: "GRDWireGuardKit",
      path: "../../third_party/ios_deps/GRDWireGuardKit/GRDWireGuardKit.xcframework"
    ),
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
      dependencies: ["Shared"],
      sources: ["BrowserIntents.intentdefinition", "CustomIntentHandler.swift"],
      plugins: ["IntentBuilderPlugin"]
    ),
    .target(
      name: "BraveWidgetsModels",
      dependencies: ["FaviconModels"],
      sources: [
        "BraveWidgets.intentdefinition", "LockScreenFavoriteIntentHandler.swift",
        "FavoritesWidgetData.swift",
      ],
      plugins: ["IntentBuilderPlugin", "LoggerPlugin"]
    ),
    .target(name: "TestHelpers", dependencies: ["Data", "BraveShared"]),
    .target(
      name: "BraveVPN",
      dependencies: [
        "BraveStore",
        "BraveStrings",
        "SnapKit",
        "Then",
        "Data",
        "GuardianConnect",
        "BraveUI",
        .product(name: "Lottie", package: "lottie-spm"),
      ],
      resources: [.copy("Resources/vpncheckmark.json")],
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
        .product(name: "Lottie", package: "lottie-spm"),
      ],
      resources: [
        .copy("Lottie Assets/brave-today-welcome-graphic.json")
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "AIChat",
      dependencies: [
        "BraveCore",
        "BraveShared",
        "BraveStore",
        "BraveStrings",
        "BraveUI",
        "DesignSystem",
        "Favicon",
        "Fuzi",
        "Preferences",
        "Strings",
        "SpeechRecognition",
        .product(name: "Collections", package: "swift-collections"),
        .product(name: "Introspect", package: "SwiftUI-Introspect"),
        .product(name: "Lottie", package: "lottie-spm"),
      ],
      resources: [
        .copy("Components/Markdown/Code Highlight/Themes/atom-one-dark.min.css"),
        .copy("Components/Markdown/Code Highlight/Themes/atom-one-light.min.css"),
        .copy("Components/Markdown/Code Highlight/Scripts/highlight.min.js"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(
      name: "AIChatTests",
      dependencies: ["AIChat"],
      resources: [
        .copy("Components/Markdown/Code Highlight/Themes/atom-one-dark.min.css"),
        .copy("Components/Markdown/Code Highlight/Themes/atom-one-light.min.css"),
        .copy("Components/Markdown/Code Highlight/Scripts/highlight.min.js"),
      ]
    ),
    .target(
      name: "BraveStore",
      dependencies: [
        "BraveCore",
        "BraveShared",
        "BraveUI",
        "DesignSystem",
        "Preferences",
        .product(name: "Collections", package: "swift-collections"),
        .product(name: "Introspect", package: "SwiftUI-Introspect"),
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
        .product(name: "Lottie", package: "lottie-spm"),
        "Preferences",
        "Shared",
        "SnapKit",
        "Storage",
      ],
      resources: [
        .copy("LottieAssets/onboarding-rewards.json"),
        .copy("LottieAssets/playlist-confetti.json"),
        .copy("WelcomeFocus/Resources/LottieAssets"),
        .copy("WelcomeFocus/Resources/Fonts/Poppins-SemiBold.ttf"),
        .copy("WelcomeFocus/Resources/Fonts/Poppins-Medium.ttf"),
        .copy("WelcomeFocus/Resources/Fonts/Poppins-Regular.ttf"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(
      name: "BraveNewsTests",
      dependencies: ["BraveNews"],
      resources: [
        .copy("opml-test-files/subscriptionList.opml"),
        .copy("opml-test-files/states.opml"),
      ]
    ),
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
        .copy("Assets/TopSites"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(name: "UserAgent", dependencies: ["Preferences"]),
    .target(
      name: "CredentialProviderUI",
      dependencies: ["BraveCore", "DesignSystem", "BraveShared", "Strings", "BraveUI"]
    ),
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
        .product(name: "CustomDump", package: "swift-custom-dump"),
      ]
    ),
    .testTarget(
      name: "StorageTests",
      dependencies: ["Storage", "TestHelpers"],
      resources: [.copy("fixtures/v33.db"), .copy("testcert1.pem"), .copy("testcert2.pem")]
    ),
    .testTarget(name: "DataTests", dependencies: ["Data", "TestHelpers"]),
    .testTarget(
      name: "ClientTests",
      dependencies: ["Brave", "BraveStrings", "TestHelpers"],
      resources: [
        .copy("Resources/debouncing.json"),
        .copy("Resources/content-blocking.json"),
        .copy("Resources/filter-lists.json"),
        .copy("Resources/google-search-plugin.xml"),
        .copy("Resources/duckduckgo-search-plugin.xml"),
        .copy("Resources/ad-block-resources/resources.json"),
        .copy("Resources/filter-rules/iodkpdagapdfkphljnddpjlldadblomo.txt"),
        .copy("Resources/html/index.html"),
        .copy("Resources/scripts/farbling-tests.js"),
        .copy("Resources/scripts/request-blocking-tests.js"),
        .copy("Resources/scripts/cosmetic-filter-tests.js"),
      ]
    ),
    .target(name: "Strings"),
    .target(name: "RuntimeWarnings"),
    .target(name: "PrivateCDN", dependencies: ["SDWebImage"]),
    .target(
      name: "Playlist",
      dependencies: [
        "Data", "BraveShared", "Shared", "Storage", "Preferences", "Strings", "CodableHelpers",
        "UserAgent", "Then",
      ],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(name: "PrivateCDNTests", dependencies: ["PrivateCDN"]),
    .testTarget(
      name: "GrowthTests",
      dependencies: ["Growth", "Shared", "BraveShared", "BraveVPN"]
    ),
    .target(
      name: "PlaylistUI",
      dependencies: [
        "Favicon", "Data", "DesignSystem", "Playlist", "SDWebImage", "SnapKit", "Strings",
        "CodableHelpers", .product(name: "Algorithms", package: "swift-algorithms"), "BraveStrings",
        .product(name: "OrderedCollections", package: "swift-collections"), "BraveUI",
      ],
      resources: [.copy("Resources/oembed_providers.json")]
    ),
    .testTarget(
      name: "PlaylistUITests",
      dependencies: ["PlaylistUI", "Playlist", "Preferences", "Data", "TestHelpers"],
      resources: [.copy("Resources/Big_Buck_Bunny_360_10s_1MB.mp4")]
    ),
    .plugin(name: "IntentBuilderPlugin", capability: .buildTool()),
    .plugin(name: "LoggerPlugin", capability: .buildTool()),
    .plugin(
      name: "LeoAssetsPlugin",
      capability: .buildTool()
    ),
    .executableTarget(name: "LeoAssetCatalogGenerator"),
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
    "AIChat",
    "BraveStore",
    "Onboarding",
    "Growth",
    "SpeechRecognition",
    "CodableHelpers",
    "Preferences",
    "Favicon",
    "CertificateUtilities",
    "Playlist",
    "UserAgent",
    .product(name: "Lottie", package: "lottie-spm"),
    .product(name: "Collections", package: "swift-collections"),
    "PlaylistUI",
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
    .copy("Assets/Interstitial Pages/Pages/BlockedDomain.html"),
    .copy("Assets/Interstitial Pages/Pages/HTTPBlocked.html"),
    .copy("Assets/Interstitial Pages/Pages/CertificateError.html"),
    .copy("Assets/Interstitial Pages/Pages/GenericError.html"),
    .copy("Assets/Interstitial Pages/Pages/NetworkError.html"),
    .copy("Assets/Interstitial Pages/Pages/Web3Domain.html"),
    .copy("Assets/Interstitial Pages/Images/Carret.png"),
    .copy("Assets/Interstitial Pages/Images/Clock.svg"),
    .copy("Assets/Interstitial Pages/Images/Cloud.svg"),
    .copy("Assets/Interstitial Pages/Images/DarkWarning.svg"),
    .copy("Assets/Interstitial Pages/Images/Generic.svg"),
    .copy("Assets/Interstitial Pages/Images/Globe.svg"),
    .copy("Assets/Interstitial Pages/Images/Info.svg"),
    .copy("Assets/Interstitial Pages/Images/Warning.svg"),
    .copy("Assets/Interstitial Pages/Images/warning-triangle-outline.svg"),
    .copy("Assets/Interstitial Pages/Styles/BlockedDomain.css"),
    .copy("Assets/Interstitial Pages/Styles/CertificateError.css"),
    .copy("Assets/Interstitial Pages/Styles/InterstitialStyles.css"),
    .copy("Assets/Interstitial Pages/Styles/NetworkError.css"),
    .copy("Assets/Interstitial Pages/Styles/Web3Domain.css"),
    .copy("Assets/Lottie/shred.json"),
    .copy("Assets/SearchPlugins"),
    .copy("Frontend/Reader/Reader.css"),
    .copy("Frontend/Reader/Reader.html"),
    .copy("Frontend/Reader/ReaderViewLoading.html"),
    .copy("Frontend/Browser/New Tab Page/Backgrounds/Assets/NTP_Images/corwin-prescott-3.jpg"),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveSearchResultAdScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveSearchScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveSkusScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/nacl.min.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/PlaylistFolderSharingScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/FrameCheckWrapper.js"
    ),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/CookieControlScript.js"),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/FarblingProtectionScript.js"
    ),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/gpc.js"),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/MediaBackgroundingScript.js"
    ),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/PlaylistScript.js"),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/PlaylistSwizzlerScript.js"
    ),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/ReadyStateScript.js"),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/RequestBlockingScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/TrackingProtectionStats.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/RewardsReportingScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/WalletEthereumProviderScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/WalletSolanaProviderScript.js"
    ),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/YoutubeQualityScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/BraveLeoScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/DarkReaderScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/DeAmpScript.js"),
    .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/FaviconScript.js"),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/ResourceDownloaderScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/SelectorsPollerScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/ProceduralFilters.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/SiteStateListenerScript.js"
    ),
    .copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/WindowRenderScript.js"
    ),
    .copy("WebFilters/ContentBlocker/Lists/block-ads.json"),
    .copy("WebFilters/ContentBlocker/Lists/block-cookies.json"),
    .copy("WebFilters/ContentBlocker/Lists/block-trackers.json"),
    .copy("WebFilters/ContentBlocker/Lists/mixed-content-upgrade.json"),
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
    PackageDescription.Resource.copy(
      "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveTalkScript.js"
    )
  )
  package.dependencies.append(
    .package(name: "JitsiMeet", path: "../../third_party/ios_deps/JitsiMeet")
  )
  package.products.append(.library(name: "BraveTalk", targets: ["BraveTalk"]))
  package.targets.append(contentsOf: [
    .target(
      name: "BraveTalk",
      dependencies: ["Shared", "Preferences", "JitsiMeet"],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(
      name: "BraveTalkTests",
      dependencies: [
        "BraveTalk", "Shared", "TestHelpers",
        .product(name: "Collections", package: "swift-collections"),
      ]
    ),
  ])
}

package.targets.append(braveTarget)

let iosRootDirectory = URL(string: #file)!.deletingLastPathComponent().absoluteString.dropLast()
let isStripAbsolutePathsFromDebugSymbolsEnabled = {
  do {
    let env = try String(contentsOfFile: "\(iosRootDirectory)/../../.env")
      .split(separator: "\n")
      .map { $0.split(separator: "=").map { $0.trimmingCharacters(in: .whitespacesAndNewlines) } }
    return env.contains(where: { $0.first == "use_remoteexec" && $0.last == "true" })
  } catch {
    return false
  }
}()

if isStripAbsolutePathsFromDebugSymbolsEnabled {
  for target in package.targets where target.type == .regular {
    var settings = target.swiftSettings ?? []
    settings.append(
      .unsafeFlags(
        [
          "-debug-prefix-map", "\(iosRootDirectory)=../../brave/ios/brave-ios",
        ],
        .when(configuration: .debug)
      )
    )
    target.swiftSettings = settings
  }
}
