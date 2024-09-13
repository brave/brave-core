// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveWidgetsModels
import Foundation
import Intents
import Preferences
import Shared

// Used by the App to navigate to different views.
// To open a URL use /open-url or to open a blank tab use /open-url with no params
public enum DeepLink: String {
  case vpnCrossPlatformPromo = "vpn_promo"
  case braveLeo = "brave_leo"
  case playlist
}

// The root navigation for the Router. Look at the tests to see a complete URL
public enum NavigationPath: Equatable {
  case url(webURL: URL?, isPrivate: Bool)
  case deepLink(DeepLink)
  case text(String)
  case widgetShortcutURL(WidgetShortcut)

  public init?(url: URL, isPrivateBrowsing: Bool) {
    let urlString = url.absoluteString
    if url.scheme?.lowercased() == "http" || url.scheme?.lowercased() == "https" {
      self = .url(webURL: url, isPrivate: isPrivateBrowsing)
      return
    }

    guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false) else {
      return nil
    }

    guard
      let urlTypes = Bundle.main.object(forInfoDictionaryKey: "CFBundleURLTypes") as? [AnyObject],
      let urlSchemes = urlTypes.first?["CFBundleURLSchemes"] as? [String]
    else {
      assertionFailure()
      return nil
    }

    guard let scheme = components.scheme, urlSchemes.contains(scheme) else {
      return nil
    }

    if urlString.starts(with: "\(scheme)://deep-link"),
      let deepURL = components.valueForQuery("path"), let link = DeepLink(rawValue: deepURL)
    {
      self = .deepLink(link)
    } else if urlString.starts(with: "\(scheme)://open-url") {
      let urlText = components.valueForQuery("url")
      let url = URIFixup.getURL(urlText ?? "") ?? urlText?.asURL
      let forcedPrivate = Preferences.Privacy.privateBrowsingOnly.value || isPrivateBrowsing
      let isPrivate = Bool(components.valueForQuery("private") ?? "") ?? forcedPrivate
      self = .url(webURL: url, isPrivate: isPrivate)
    } else if urlString.starts(with: "\(scheme)://open-text") {
      let text = components.valueForQuery("text")
      self = .text(text ?? "")
    } else if urlString.starts(with: "\(scheme)://search") {
      let text = components.valueForQuery("q")
      self = .text(text ?? "")
    } else if urlString.starts(with: "\(scheme)://shortcut"),
      let valueString = components.valueForQuery("path"),
      let value = WidgetShortcut.RawValue(valueString),
      let path = WidgetShortcut(rawValue: value)
    {
      self = .widgetShortcutURL(path)
    } else {
      return nil
    }
  }

  static func handle(nav: NavigationPath, with bvc: BrowserViewController) {
    switch nav {
    case .deepLink(let link): NavigationPath.handleDeepLink(link, with: bvc)
    case .url(let url, let isPrivate):
      NavigationPath.handleURL(url: url, isPrivate: isPrivate, with: bvc)
    case .text(let text): NavigationPath.handleText(text: text, with: bvc)
    case .widgetShortcutURL(let path): NavigationPath.handleWidgetShortcut(path, with: bvc)
    }
  }

  private static func handleDeepLink(_ link: DeepLink, with bvc: BrowserViewController) {
    switch link {
    case .vpnCrossPlatformPromo:
      bvc.presentVPNInAppEventCallout()
    case .braveLeo:
      bvc.presentBraveLeoDeepLink()
    case .playlist:
      let helper = BrowserNavigationHelper(bvc)
      helper.openPlaylist()
    }
  }

  private static func handleURL(url: URL?, isPrivate: Bool, with bvc: BrowserViewController) {
    if let newURL = url {
      bvc.switchToTabForURLOrOpen(newURL, isPrivate: isPrivate, isPrivileged: false)
    } else {
      bvc.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: isPrivate)
    }
  }

  private static func handleText(text: String, with bvc: BrowserViewController) {
    bvc.openBlankNewTab(
      attemptLocationFieldFocus: true,
      isPrivate: bvc.privateBrowsingManager.isPrivateBrowsing,
      searchFor: text
    )
  }

  static func handleWidgetShortcut(_ path: WidgetShortcut, with bvc: BrowserViewController) {
    switch path {
    case .unknown, .search:
      // Search
      if let url = bvc.tabManager.selectedTab?.url, InternalURL(url)?.isAboutHomeURL == true {
        bvc.focusURLBar()
      } else {
        bvc.openBlankNewTab(
          attemptLocationFieldFocus: true,
          isPrivate: bvc.privateBrowsingManager.isPrivateBrowsing
        )
      }
    case .newTab:
      bvc.openBlankNewTab(
        attemptLocationFieldFocus: false,
        isPrivate: bvc.privateBrowsingManager.isPrivateBrowsing
      )
    case .newPrivateTab:
      if Preferences.Privacy.lockWithPasscode.value {
        bvc.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: true)
      } else {
        if Preferences.Privacy.privateBrowsingLock.value {
          bvc.askForLocalAuthentication(viewType: .external) { [weak bvc] success, _ in
            if success {
              bvc?.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: true)
            }
          }
        } else {
          bvc.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: true)
        }
      }
    case .bookmarks:
      bvc.navigationHelper.openBookmarks()
    case .history:
      bvc.navigationHelper.openHistory()
    case .downloads:
      bvc.navigationHelper.openDownloads { success in
        if !success {
          bvc.displayOpenDownloadsError()
        }
      }
    case .playlist:
      bvc.navigationHelper.openPlaylist()
    case .wallet:
      bvc.navigationHelper.openWallet()
    case .scanQRCode:
      bvc.scanQRCode()
    case .braveNews:
      bvc.openBlankNewTab(attemptLocationFieldFocus: false, isPrivate: false, isExternal: true)
      bvc.popToBVC()
      guard let newTabPageController = bvc.tabManager.selectedTab?.newTabPageViewController else {
        return
      }
      newTabPageController.scrollToBraveNews()
    case .braveLeo:
      bvc.popToBVC()
      bvc.openBraveLeo()
    @unknown default:
      assertionFailure()
      break
    }
  }
}
