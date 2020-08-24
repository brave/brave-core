# Changelog

## [1.19.2](https://github.com/brave/brave-ios/releases/tag/v1.19.2)

 - Implemented URL scheme handlers for both "HTTP" and "HTTPS". ([#2784](https://github.com/brave/brave-ios/issues/2784))
 - Removed Sync UI from settings. ([#2718](https://github.com/brave/brave-ios/issues/2718))
 
## [1.19](https://github.com/brave/brave-ios/releases/tag/v1.19)

 - Implemented "Brave Firewall + VPN". ([#2739](https://github.com/brave/brave-ios/issues/2739))
 - Updated Brave Rewards to display BAT values to three decimal places. ([#2596](https://github.com/brave/brave-ios/issues/2596))
 - Fixed favorite list not updating on New Tab Page when an existing favorite is deleted. ([#2685](https://github.com/brave/brave-ios/issues/2685))

## [1.18.1](https://github.com/brave/brave-ios/releases/tag/v1.18.1)

 - Fixed pre-roll ads being shown on YouTube videos. ([#511](https://github.com/brave/brave-ios/issues/511))
 - Fixed crash when loading favicons for bookmarks in certain cases. ([#2697](https://github.com/brave/brave-ios/issues/2697))
 - Fixed crash in certain cases when Ad-block list is updated. ([#2699](https://github.com/brave/brave-ios/issues/2699))

## [1.18](https://github.com/brave/brave-ios/releases/tag/v1.18)

 - Implemented enhancements to New Tab Page and favorites overlay. ([#2578](https://github.com/brave/brave-ios/issues/2578))
 - Implemented enhancements to favorites to use fallback monogram letters instead of low-resolution icons. ([#2579](https://github.com/brave/brave-ios/issues/2579))
 - Fixed an issue where favorites icon mismatch when they are reordered. ([#2099](https://github.com/brave/brave-ios/issues/2099))
 - Fixed New Tab Page from loading new images when switching between tabs. ([#2071](https://github.com/brave/brave-ios/issues/2071))
 - Fixed fuzzy/low-resolution favicons for favorites. ([#528](https://github.com/brave/brave-ios/issues/528)) 

## [1.17](https://github.com/brave/brave-ios/releases/tag/v1.17)

 - Implemented super referral improvements. ([#2539](https://github.com/brave/brave-ios/issues/2539))
 - Fixed an issue where grants were not claimed in first attempt. ([#2146](https://github.com/brave/brave-ios/issues/2146))
 - Fixed an issue where Javascript URLs should only work for bookmarklets. ([#2463](https://github.com/brave/brave-ios/issues/2463))


## [1.16.2](https://github.com/brave/brave-ios/releases/tag/v1.16.2)

 - Fixed an issue where total balance did not include old promotions in certain scenarios. ([#2556](https://github.com/brave/brave-ios/issues/2556)

## [1.16.1](https://github.com/brave/brave-ios/releases/tag/v1.16.1)

 - Implemented pagination for publisher list. ([#2529](https://github.com/brave/brave-ios/issues/2529))
 - Fixed users not receiving ad promotion due to empty public key in certain cases. ([#2536](https://github.com/brave/brave-ios/issues/2536))
 - Fixed missing estimated ads payout details. ([#2531](https://github.com/brave/brave-ios/issues/2531))
 - Fixed ads initialization between app relaunch. ([#2541](https://github.com/brave/brave-ios/issues/2541))


## [1.16](https://github.com/brave/brave-ios/releases/tag/v1.16)

 - Added support for referral background images and top sites on the New Tab Page. ([#2324](https://github.com/brave/brave-ios/issues/2324))
 - Fixed missing urlbar icons for certificate errors. ([#1963](https://github.com/brave/brave-ios/issues/1963))
 - Fixed certain websites not being classified correctly for Brave ads. ([#2444](https://github.com/brave/brave-ios/issues/2444))
 - Fixed monthly contributions panel still editable when Brave Rewards is disabled. [#2401](https://github.com/brave/brave-ios/issues/2401))
 - Fixed all tabs being destroyed when partially clearing data using "Clear Private Data". ([#2155](https://github.com/brave/brave-ios/issues/2155))
 - Fixed alignment issue in urlbar. ([#1964](https://github.com/brave/brave-ios/issues/1964))
 - Updated text from "Allow contribution to non-verified sites" to "Show non-verified sites in list" under auto-contribution settings. ([#2402](https://github.com/brave/brave-ios/issues/2402))

## [1.15.2](https://github.com/brave/brave-ios/releases/tag/v1.15.2)

 - Fixed crash on certain devices when downloading publisher list by improving load times. ([#2378](https://github.com/brave/brave-ios/issues/2477))

## [1.15.1](https://github.com/brave/brave-ios/releases/tag/v1.15.1)

 - Migrated rewards database to new code base to improve stability and performance. ([#2378](https://github.com/brave/brave-ios/issues/2378))
 - [Security] Updated Minimist package. ([#2423](https://github.com/brave/brave-ios/issues/2423))
 - Fixed claiming grant from the rewards panel not always working on the first attempt. ([#2329](https://github.com/brave/brave-ios/issues/2329))

## [1.15](https://github.com/brave/brave-ios/releases/tag/v1.15)
 
 - Implemented Safari / iOS User Agent. ([#2210](https://github.com/brave/brave-ios/issues/2210))
 - Added haptic feedback. ([#2283](https://github.com/brave/brave-ios/issues/2283))
 - Added detailed view of pending contributions for rewards. ([#1670](https://github.com/brave/brave-ios/issues/1670))
 - Added ability to restore individual publishers from the "Sites Excluded" list. ([#1674](https://github.com/brave/brave-ios/issues/1674))
 - Added "Open Brave Rewards Settings" under rewards settings. ([#1966](https://github.com/brave/brave-ios/issues/1966))
 - Improved New Tab Page Sponsored Images modals. ([#2363](https://github.com/brave/brave-ios/issues/2363))
 - Changed settings header from "BRAVE SHIELD DEFAULTS" to "SHIELDS DEFAULTS". ([#1196](https://github.com/brave/brave-ios/issues/1196))
 - Fixed crash when downloading New Tab Page Sponsored Images in certain cases. ([#2375](https://github.com/brave/brave-ios/issues/2375))
 - Fixed tabs opened using "window.open" not being restored when restarting Brave. ([#1397](https://github.com/brave/brave-ios/issues/1397))
 - Fixed "Claim my rewards" button under the New Tab Page Sponsored Images modal not always working on the first attempt. ([#2280](https://github.com/brave/brave-ios/issues/2280))
 - Fixed custom images not being displayed under the publisher tipping banner. ([#2164](https://github.com/brave/brave-ios/issues/2164))
 - Fixed on-screen keyboard being dismissed while typing. ([#2016](https://github.com/brave/brave-ios/issues/2016))
 - Fixed rendering glitch affecting the tab bar on iPadOS. ([#2124](https://github.com/brave/brave-ios/issues/2124))
 - Fixed spacing issues between sync text and warning message under "Enter the sync code" screen. ([#2006](https://github.com/brave/brave-ios/issues/2006))

## [1.14.3](https://github.com/brave/brave-ios/releases/tag/v1.14.3)
 
  - Added New Tab Page Sponsored Images. ([#2151](https://github.com/brave/brave-ios/issues/2151))
  - Disable bookmarklets in URL bar. ([#2297](https://github.com/brave/brave-ios/issues/2297))
  - Fix ads region support when calendar is set to non-default. ([#2022](https://github.com/brave/brave-ios/issues/2022))
  - Removed unverified info message for Brave verified publisher. ([#2202](https://github.com/brave/brave-ios/issues/2202))

## [1.14.2](https://github.com/brave/brave-ios/releases/tag/v1.14.2)
 
  - Added support for NFC authentication. ([#1609](https://github.com/brave/brave-ios/issues/1609))
  - Restore lost bookmarks and favorites during 1.12 migration experienced by a subset of users. ([#2015](https://github.com/brave/brave-ios/issues/2015))

## [1.14.1](https://github.com/brave/brave-ios/releases/tag/v1.14.1)

 - Added images to new tab page. ([#1782](https://github.com/brave/brave-ios/issues/1782))
 - Fixed wallet details not being displayed for empty wallets. ([#1852](https://github.com/brave/brave-ios/issues/1852))
 - Fixed top bar visibility when settings page is opened on iPhones with iOS 13. ([#1714](https://github.com/brave/brave-ios/issues/1714))
 - Fixed ad notification from being stacked underneath the shield and reward panels. ([#1904](https://github.com/brave/brave-ios/issues/1904))

## [1.14](https://github.com/brave/brave-ios/releases/tag/v1.14)

 - Added download preview option. ([#1467](https://github.com/brave/brave-ios/issues/1467))
 - Added user confirmation when external links try to switch apps. ([#551](https://github.com/brave/brave-ios/issues/551))
 - Added time out details for incorrect pin-code lockout. ([#443](https://github.com/brave/brave-ios/issues/443))
 - Switching to "Private Browsing Only" closes normal tabs and delete sessions. ([#580](https://github.com/brave/brave-ios/issues/580))
 - Fixed rewards button being displayed even when disabled in settings. ([#1954](https://github.com/brave/brave-ios/issues/1954))
 - Fixed browser crash when attempting to clear private data on iPadOS 13.2. ([#1951](https://github.com/brave/brave-ios/issues/1951))
 - Fixed AppStore button functionality when AppStore links are opened in the browser. ([#1941](https://github.com/brave/brave-ios/issues/1941))
 - Fixed several keyboard shortcuts when using a Bluetooth keyboard. ([#1146](https://github.com/brave/brave-ios/issues/1146))
 - Fixed force 3D Touch cursor movement in the URL bar when URL is highlighted. ([#1081](https://github.com/brave/brave-ios/issues/1081))

## [1.13](https://github.com/brave/brave-ios/releases/tag/v1.13)
 
 - Added Brave Rewards and Ads. ([#1920](https://github.com/brave/brave-ios/issues/1920))
 - Added new preview menu on iOS13. ([#1499](https://github.com/brave/brave-ios/issues/1499))
 - Added bookmarklet support. ([#1119](https://github.com/brave/brave-ios/issues/1119))
 - Updated text size and colors on new tab stats to match desktop. ([#1641](https://github.com/brave/brave-ios/issues/1641))
 - Updated StartPage icon. ([#1598](https://github.com/brave/brave-ios/issues/1598))
 - Fixed wrong device name being shown when tapping on sync devices. ([#1587](https://github.com/brave/brave-ios/issues/1587))
 - Fixed readability of website/tab names in dark mode. ([#1551](https://github.com/brave/brave-ios/issues/1551))
 - Fixed button border visibility under Brave Shields when using the light theme. ([#1548](https://github.com/brave/brave-ios/issues/1548))
 - Fixed missing long press context menu on image search results. ([#1547](https://github.com/brave/brave-ios/issues/1547))
 - Fixed "Download Cancelled" not visible after cancelling a download under iOS 12. ([#1516](https://github.com/brave/brave-ios/issues/1516))
 - Removed "www" from the URL bar. ([#1533](https://github.com/brave/brave-ios/issues/1533))
 - Removed Alexa top sites recommendations from showing up as search suggestions. ([#868](https://github.com/brave/brave-ios/issues/868))
 - Removed URL autocomplete while editing an existing URL. ([#807](https://github.com/brave/brave-ios/issues/807))

## [1.12.1](https://github.com/brave/brave-ios/releases/tag/v1.12.1)

 - Fixed data loss on upgrade to 1.12 under certain scenarios. ([#1554](https://github.com/brave/brave-ios/issues/1554))

## [1.12](https://github.com/brave/brave-ios/releases/tag/v1.12)

 - Added dark mode support for iOS 12 and iOS 13. ([#1493](https://github.com/brave/brave-ios/issues/1493))
 - Added onboarding experience for new users. ([#1416](https://github.com/brave/brave-ios/issues/1416))
 - Added file downloading into the Downloads folder. ([#206](https://github.com/brave/brave-ios/issues/206))
 - Added "DuckDuckGo" as default search engine for Australia/Germany/Ireland and New Zealand. ([#1451](https://github.com/brave/brave-ios/issues/1451))
 - Added URLs to 2FA modals. ([#1359](https://github.com/brave/brave-ios/issues/1359))
 - Added option in settings to enable bookmarks button beside the URL bar. ([#1391](https://github.com/brave/brave-ios/issues/1391))
 - Improved handling of truncated domains. ([#552](https://github.com/brave/brave-ios/issues/552))
 - Updated default iPad user agent to desktop from mobile. ([#1290](https://github.com/brave/brave-ios/issues/1290))
 - Updated several text strings under settings to have proper capitalization. ([#1431](https://github.com/brave/brave-ios/issues/1431))
 - Updated "Add to.." to "Add Bookmark" and "Always request desktop site" to "Request Desktop Site". ([#1453](https://github.com/brave/brave-ios/issues/1453))
 - Disabled edit mode when there are no added bookmarks. ([#882](https://github.com/brave/brave-ios/issues/882))
 - Removed empty brackets from the download popup when files are smaller than a certain size. ([#1433](https://github.com/brave/brave-ios/issues/1433))
 - Fixed denial of service vulnerability using JS. ([#87](https://github.com/brave/brave-ios/issues/87))
 - Fixed ability to access "Brave.sqlite" database from Documents folder. ([#195](https://github.com/brave/brave-ios/issues/195))
 - Fixed inability to share content with Brave using Feedly. ([#661](https://github.com/brave/brave-ios/issues/661))
 - Fixed status bar position when device is oriented to landscape mode. ([#867](https://github.com/brave/brave-ios/issues/867))
 - Fixed an issue when exiting private browsing mode causes normal tab to show blank page with an incorrect URL. ([#888](https://github.com/brave/brave-ios/issues/888))
 - Fixed PIN window not being shown in certain cases where app is minimized and brought into focus. ([#980](https://github.com/brave/brave-ios/issues/980))
 - Fixed on-screen keyboard not being displayed in certain cases when viewing PIN window. ([#1122](https://github.com/brave/brave-ios/issues/1122))
 - Fixed incorrect URL being shared with Brave on certain websites. ([#1032](https://github.com/brave/brave-ios/issues/1032))
 - Fixed new tab page not being shown in certain cases. ([#1244](https://github.com/brave/brave-ios/issues/1244))
 - Fixed source link on long press context menu on an image. ([#1266](https://github.com/brave/brave-ios/issues/1266))
 - Fixed text string capitalization on sync screen. ([#1274](https://github.com/brave/brave-ios/issues/1274))
 - Fixed tap area near the edge of Brave Shields which selected the URL. ([#1280](https://github.com/brave/brave-ios/issues/1280))
 - Fixed issue where websites can't be shared/added to bookmarks in reader mode. ([#1340](https://github.com/brave/brave-ios/issues/1340))
 - Fixed report a bug in settings not working. ([#1354](https://github.com/brave/brave-ios/issues/1354))
 - Fixed crash when using YubiKey in certain cases. ([#1388](https://github.com/brave/brave-ios/issues/1388))
 - Fixed search data being retained in private browsing in certain cases. ([#1395](https://github.com/brave/brave-ios/issues/1395))
 - Fixed modals not using the entire screen on mobile on iOS 13. ([#1414](https://github.com/brave/brave-ios/issues/1414))
 - Fixed full screen issue for video sites when viewed in desktop mode. ([#1419](https://github.com/brave/brave-ios/issues/1419))
 - Fixed crash when transitioning from portrait to landscape while context menu opened. ([#1513](https://github.com/brave/brave-ios/issues/1513))
 - Fixed "DuckDuckGo" modal not being shown on private tab when a different search engine is set during onboarding. ([#1515](https://github.com/brave/brave-ios/issues/1515))
 - Fixed an issue where tab highlight is lost when the device is locked and unlocked. ([#1526](https://github.com/brave/brave-ios/issues/1526))

## [1.11.4](https://github.com/brave/brave-ios/releases/tag/v1.11.4)

 - Added support for Adblock Rust library. ([#1442](https://github.com/brave/brave-ios/issues/1442))

## [1.11.3](https://github.com/brave/brave-ios/releases/tag/v1.11.3)

 - Added support for U2F/WebAuthn hardware security keys. ([#1406](https://github.com/brave/brave-ios/issues/1406))
 - Fixed app crash when switching from Private mode to normal mode. ([#1309](https://github.com/brave/brave-ios/issues/1309))

## [1.11](https://github.com/brave/brave-ios/releases/tag/v1.11)
 
 - Added quick action for New Private Tab from home screen. ([#1258](https://github.com/brave/brave-ios/issues/1258))  
 - Improved video orientation on Youtube. ([#1189](https://github.com/brave/brave-ios/issues/1189))
 - Fixed passcode not being reset when resuming from background. ([#1203](https://github.com/brave/brave-ios/issues/1203))
 - Fixed browser becoming unresponsive when using print preview in certain conditions. ([#1144](https://github.com/brave/brave-ios/issues/1144))

## [1.10](https://github.com/brave/brave-ios/releases/tag/v1.10)

 - Implemented new address bar layout. ([#1134](https://github.com/brave/brave-ios/issues/1134))
 - Implemented new main menu layout. ([#1130](https://github.com/brave/brave-ios/issues/1130))
 - Added ability to "Find in page" using the URL bar. ([#1019](https://github.com/brave/brave-ios/issues/1019))
 - Added 1Password activity in share sheet. ([#948](https://github.com/brave/brave-ios/issues/948))
 - Added image titles when using long-press on images. ([#851](https://github.com/brave/brave-ios/issues/851))
 - Added privacy warning on Brave Sync code page. ([#1225](https://github.com/brave/brave-ios/issues/1225))
 - Improved swipe gesture on bookmark hanger. ([#953](https://github.com/brave/brave-ios/issues/953)) 
 - Improved scrolling issue on websites with sticky headers and footers. ([#631](https://github.com/brave/brave-ios/issues/631))
 - Improved icon resolution for favourites. ([#463](https://github.com/brave/brave-ios/issues/463))
 - Fixed several random crashes in certain conditions. ([#1120](https://github.com/brave/brave-ios/issues/1120))
 - Fixed "Block-all-cookies" toggle behaviour. ([#897](https://github.com/brave/brave-ios/issues/897))
 - Fixed not being able to enter passcode after lockout timer. ([#938](https://github.com/brave/brave-ios/issues/938))
 - Fixed browser failing to start on iPad Air2 in certain conditions. ([#1040](https://github.com/brave/brave-ios/issues/1040)) 
 - Fixed browser failing to launch when device uses Swedish as default language. ([#1111](https://github.com/brave/brave-ios/issues/1111))
 - Fixed auto-focus of a tab when a link is opened using the share sheet. ([#698](https://github.com/brave/brave-ios/issues/698))
 - Fixed missing translations when viewing introduction summary under private tab. ([#1239](https://github.com/brave/brave-ios/issues/1239))
 - Fixed webcompat issues with https://borsen.dk/ due to Brave shields. ([#1061](https://github.com/brave/brave-ios/issues/1061))
 
