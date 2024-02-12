// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Shared
import BraveShared
import Storage
import Data
import WebKit
import ObjectiveC.runtime
@testable import Brave

private extension WKWebView {
  class func swizzleMe() {
    let originalSelector = #selector(WKWebView.init(frame:configuration:))
    let swizzledSelector = #selector(WKWebView.reInit(frame:configuration:))

    let originalMethod = class_getInstanceMethod(self, originalSelector)!
    let swizzledMethod = class_getInstanceMethod(self, swizzledSelector)!

    let didAddMethod = class_addMethod(self, originalSelector, method_getImplementation(swizzledMethod), method_getTypeEncoding(swizzledMethod))

    if didAddMethod {
      class_replaceMethod(self, swizzledSelector, method_getImplementation(originalMethod), method_getTypeEncoding(originalMethod))
    } else {
      method_exchangeImplementations(originalMethod, swizzledMethod);
    }
  }

  @objc
  func reInit(frame: CGRect, configuration: WKWebViewConfiguration) -> WKWebView {
    configuration.setValue(true, forKey: "alwaysRunsAtForegroundPriority")
    return reInit(frame: frame, configuration: configuration)
  }
}

private extension HTTPCookie {
  class func filter(cookies: [HTTPCookie], for url: URL) -> [HTTPCookie]? {
    guard let host = url.host?.lowercased() else { return nil }
    return cookies.filter({ $0.validFor(host: host) })
  }

  private func validFor(host: String) -> Bool {
    guard domain.hasPrefix(".") else { return host == domain }
    return host == domain.dropFirst() || host.hasSuffix(domain)
  }
}

private class WebViewNavigationAdapter: NSObject, WKNavigationDelegate {
  private let didFailListener: ((Error) -> Void)?
  private let didFinishListener: (() -> Void)?

  init(didFailListener: ((Error) -> Void)? = nil, didFinishListener: (() -> Void)? = nil) {
    self.didFailListener = didFailListener
    self.didFinishListener = didFinishListener
    super.init()
  }

  func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
    decisionHandler(.allow)
  }

  func webView(_ webView: WKWebView, decidePolicyFor navigationResponse: WKNavigationResponse, decisionHandler: @escaping (WKNavigationResponsePolicy) -> Void) {
    decisionHandler(.allow)
  }

  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    didFinishListener?()
  }

  func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    didFailListener?(error)
  }
}

@MainActor class TabSessionTests: XCTestCase {
  private var tabManager: TabManager!
  private let maxTimeout = 60.0
  private let privateBrowsingManager = PrivateBrowsingManager()

  override class func setUp() {
    super.setUp()
    //        WKWebView.swizzleMe()
  }

  override func setUp() {
    super.setUp()

    DataController.shared.initializeOnce()
    tabManager = { () -> TabManager in
      let profile = BrowserProfile(localName: "profile")
      return TabManager(windowId: UUID(), prefs: profile.prefs, rewards: nil, tabGeneratorAPI: nil, privateBrowsingManager: privateBrowsingManager)
    }()
  }

  override func tearDown() {
    super.tearDown()

    destroyData {}
    tabManager = nil
  }

  private func destroyData(_ completion: @escaping () -> Void) {
    tabManager.removeAll()
    tabManager.reset()

    HTTPCookieStorage.shared.removeCookies(since: .distantPast)

    WKWebsiteDataStore.default().removeData(
      ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
      modifiedSince: .distantPast,
      completionHandler: {
        completion()
      })
  }

  func testPrivateTabSessionSharing() {
    let dataStoreExpectation = XCTestExpectation(description: "dataStorePersistence")
    var webViewNavigationAdapter = WebViewNavigationAdapter()

    destroyData {
      let urls = [
        "https://bing.com",
        "https://google.com",
        "https://yandex.ru",
        "https://yahoo.com",
      ]

      self.tabManager.addTabsForURLs(urls.compactMap({ URL(string: $0) }), zombie: false, isPrivate: true)
      if self.tabManager.allTabs.count != 4 {
        XCTFail("Error: Not all Tabs are created equally")
        return dataStoreExpectation.fulfill()
      }

      let group = DispatchGroup()
      for tab in self.tabManager.allTabs {
        guard let webView = tab.webView else {
          XCTFail("WebView is not created yet")
          return dataStoreExpectation.fulfill()
        }

        if webView.configuration.websiteDataStore.isPersistent {
          XCTFail("Private Tab is storing data persistently!")
          return dataStoreExpectation.fulfill()
        }

        group.enter()
      }

      webViewNavigationAdapter = WebViewNavigationAdapter(
        didFailListener: { _ in
          group.leave()
        },
        didFinishListener: {
          group.leave()
        })
      self.tabManager.addNavigationDelegate(webViewNavigationAdapter)

      group.notify(queue: .main) {
        if self.tabManager.allTabs.count != 4 {
          XCTFail("Error: Not all Tabs are alive equally")
          return dataStoreExpectation.fulfill()
        }

        // All requests finished loading.. time to check the cookies..
        let group = DispatchGroup()
        for tab in self.tabManager.allTabs {
          guard let webView = tab.webView else {
            XCTFail("WebView died unexpectedly")
            return dataStoreExpectation.fulfill()
          }

          group.enter()
          webView.configuration.websiteDataStore.fetchDataRecords(
            ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
            completionHandler: { records in
              XCTAssertFalse(records.isEmpty, "Error: Data Store not shared amongst private tabs!")

              let recordNames = Set<String>(records.compactMap({ URL(string: "http://\($0.displayName)")?.host }))
              let urlNames = Set<String>(urls.compactMap({ URL(string: $0)?.host }))

              XCTAssertTrue(urlNames.isSubset(of: recordNames), "Data Store records do not match!")
              group.leave()
            })
        }

        group.notify(queue: .main) {
          dataStoreExpectation.fulfill()
        }
      }
    }
    wait(for: [dataStoreExpectation], timeout: maxTimeout)
  }

  func testNormalTabSessionSharing() {
    let dataStoreExpectation = XCTestExpectation(description: "dataStorePersistence")
    var webViewNavigationAdapter = WebViewNavigationAdapter()

    destroyData {
      let urls = [
        "https://stackoverflow.com",
        "https://discordapp.com",
        "https://apple.com",
        "https://slack.com",
      ]

      self.tabManager.addTabsForURLs(urls.compactMap({ URL(string: $0) }), zombie: false, isPrivate: false)
      self.tabManager.removeTabs(self.tabManager.allTabs.filter({ $0.url?.absoluteString.contains("localhost") ?? false }))
      if self.tabManager.allTabs.count != 4 {
        XCTFail("Error: Not all Tabs are created equally")
        return dataStoreExpectation.fulfill()
      }

      let group = DispatchGroup()
      for tab in self.tabManager.allTabs {
        guard let webView = tab.webView else {
          XCTFail("WebView is not created yet")
          return dataStoreExpectation.fulfill()
        }

        if !webView.configuration.websiteDataStore.isPersistent {
          XCTFail("Normal Tab is not storing data persistently!")
          return dataStoreExpectation.fulfill()
        }

        group.enter()
      }

      webViewNavigationAdapter = WebViewNavigationAdapter(
        didFailListener: { _ in
          group.leave()
        },
        didFinishListener: {
          group.leave()
        })
      self.tabManager.addNavigationDelegate(webViewNavigationAdapter)

      group.notify(queue: .main) {
        if self.tabManager.allTabs.count != 4 {
          XCTFail("Error: Not all Tabs are alive equally")
          return dataStoreExpectation.fulfill()
        }

        // All requests finished loading.. time to check the cookies..
        let group = DispatchGroup()
        if self.tabManager.allTabs.isEmpty {
          XCTFail("Tabs somehow destroyed")
          return dataStoreExpectation.fulfill()
        }

        for tab in self.tabManager.allTabs {
          guard let webView = tab.webView else {
            XCTFail("WebView died unexpectedly")
            return dataStoreExpectation.fulfill()
          }

          group.enter()
          webView.configuration.websiteDataStore.fetchDataRecords(
            ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
            completionHandler: { records in
              XCTAssertFalse(records.isEmpty, "Error: Data Store not shared amongst normal tabs!")

              let recordNames = Set<String>(records.compactMap({ URL(string: "http://\($0.displayName)")?.host }))
              let urlNames = Set<String>(urls.compactMap({ URL(string: $0)?.host }))

              XCTAssertTrue(urlNames.isSubset(of: recordNames), "Data Store records do not match!")
              group.leave()
            })
        }

        group.notify(queue: .main) {
          dataStoreExpectation.fulfill()
        }
      }
    }
    wait(for: [dataStoreExpectation], timeout: maxTimeout)
  }

  func testPrivateTabNonPersistence() {
    let dataStoreExpectation = XCTestExpectation(description: "dataStorePersistence")
    var webViewNavigationAdapter = WebViewNavigationAdapter()

    destroyData {
      let urls = [
        "https://github.com",
        "https://bitbucket.com",
        "https://youtube.com",
        "https://msn.com",
      ]

      self.tabManager.addTabsForURLs(urls.compactMap({ URL(string: $0) }), zombie: false, isPrivate: true)
      if self.tabManager.allTabs.count != 4 {
        XCTFail("Error: Not all Tabs are created equally")
        dataStoreExpectation.fulfill()
      }

      let group = DispatchGroup()
      for tab in self.tabManager.allTabs {
        guard let webView = tab.webView else {
          XCTFail("WebView is not created yet")
          return dataStoreExpectation.fulfill()
        }

        if webView.configuration.websiteDataStore.isPersistent {
          XCTFail("Private Tab is storing data persistently!")
          return dataStoreExpectation.fulfill()
        }

        group.enter()
      }

      webViewNavigationAdapter = WebViewNavigationAdapter(
        didFailListener: { _ in
          group.leave()
        },
        didFinishListener: {
          group.leave()
        })
      self.tabManager.addNavigationDelegate(webViewNavigationAdapter)

      group.notify(queue: .main) {
        // All requests finished loading.. kill all tabs.. check the cookies..
        self.tabManager.removeAll()

        // When the tab manager destroys tabs, if there are no tabs left, it adds a new zombie tab.
        // Hence the count of 1.
        if self.tabManager.allTabs.count != 1 {
          XCTFail("Error: Not all Tabs are destroyed equally")
          return dataStoreExpectation.fulfill()
        }

        // Now that all private tabs have been destroyed, we need to create a new one and check the storage.
        // We'd verify if the new tab has the old data after all private tabs were destroyed.
        let group = DispatchGroup()
        webViewNavigationAdapter = WebViewNavigationAdapter(
          didFailListener: { _ in
            group.leave()
          },
          didFinishListener: {
            group.leave()
          })

        self.tabManager.addNavigationDelegate(webViewNavigationAdapter)
        self.tabManager.addTabsForURLs([URL(string: "https://brave.com")!], zombie: false, isPrivate: true)
        if self.tabManager.tabs(withType: .private).isEmpty {
          XCTFail("Error: Private tab not created")
          return dataStoreExpectation.fulfill()
        }

        for tab in self.tabManager.allTabs {
          guard let webView = tab.webView else {
            XCTFail("WebView is not created yet")
            return dataStoreExpectation.fulfill()
          }

          if webView.configuration.websiteDataStore.isPersistent {
            XCTFail("Private Tab is storing data persistently!")
            return dataStoreExpectation.fulfill()
          }

          group.enter()
        }

        group.notify(queue: .main) {
          let group = DispatchGroup()
          for tab in self.tabManager.allTabs {
            guard let webView = tab.webView else {
              XCTFail("WebView died unexpectedly")
              return dataStoreExpectation.fulfill()
            }

            group.enter()
            webView.configuration.websiteDataStore.fetchDataRecords(
              ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
              completionHandler: { records in
                XCTAssertFalse(records.isEmpty, "Error: Data Store does not contain the latest webpage's data!")

                let recordNames = records.compactMap({ URL(string: "http://\($0.displayName)")?.host })

                for url in urls.compactMap({ URL(string: $0)?.host }) {
                  if recordNames.contains(url) {
                    XCTFail("Error: Data Store is NOT ephemeral!")
                  }
                }

                group.leave()
              })
          }

          group.notify(queue: .main) {
            dataStoreExpectation.fulfill()
          }
        }
      }
    }

    wait(for: [dataStoreExpectation], timeout: maxTimeout)
  }

  func testTabsPrivateToNormal() {
    let dataStoreExpectation = XCTestExpectation(description: "dataStorePersistence")
    var webViewNavigationAdapter = WebViewNavigationAdapter()

    destroyData {
      let url = URL(string: "https://www.insanelymac.com")!
      let otherURL = URL(string: "https://www.macrumors.com")!

      self.tabManager.addTabsForURLs([url], zombie: false, isPrivate: true)
      if self.tabManager.allTabs.count != 1 {
        XCTFail("Error: Not all Tabs are created equally")
        return dataStoreExpectation.fulfill()
      }

      let group = DispatchGroup()
      for tab in self.tabManager.allTabs {
        guard let webView = tab.webView else {
          XCTFail("WebView is not created yet")
          return dataStoreExpectation.fulfill()
        }

        if webView.configuration.websiteDataStore.isPersistent {
          XCTFail("Private Tab is storing data persistently!")
          return dataStoreExpectation.fulfill()
        }

        group.enter()
      }

      webViewNavigationAdapter = WebViewNavigationAdapter(
        didFailListener: { _ in
          group.leave()
        },
        didFinishListener: {
          group.leave()
        })
      self.tabManager.addNavigationDelegate(webViewNavigationAdapter)

      // All requests finished loading.. switch to normal mode.. check the cookies..
      group.notify(queue: .main) {
        let group = DispatchGroup()
        webViewNavigationAdapter = WebViewNavigationAdapter(
          didFailListener: { _ in
            group.leave()
          },
          didFinishListener: {
            group.leave()
          })

        self.tabManager.addNavigationDelegate(webViewNavigationAdapter)
        self.tabManager.addTabsForURLs([otherURL], zombie: false, isPrivate: false)

        // When the tab manager switches to normal mode from private
        // It should kill all private tabs..
        if !self.tabManager.tabs(withType: .private).isEmpty {
          XCTFail("Error: Private tabs not destroyed when switching to normal mode!")
          return dataStoreExpectation.fulfill()
        }

        if self.tabManager.tabs(withType: .regular).isEmpty {
          XCTFail("Error: Normal tab not created")
          return dataStoreExpectation.fulfill()
        }

        for tab in self.tabManager.allTabs {
          guard let webView = tab.webView else {
            XCTFail("WebView is not created yet")
            return dataStoreExpectation.fulfill()
          }

          if webView.configuration.websiteDataStore.isPersistent == false {
            XCTFail("Normal Tab is not storing data persistently!")
            return dataStoreExpectation.fulfill()
          }

          group.enter()
        }

        group.notify(queue: .main) {
          let group = DispatchGroup()
          for tab in self.tabManager.tabs(withType: .regular) {
            guard let webView = tab.webView else {
              XCTFail("Webview died unexpectedly")
              return dataStoreExpectation.fulfill()
            }

            group.enter()
            webView.configuration.websiteDataStore.fetchDataRecords(
              ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
              completionHandler: { records in
                XCTAssertFalse(records.isEmpty, "Error: Data Store not persistent in normal mode!")

                let recordNames = Set<String>(records.compactMap({ URL(string: "http://\($0.displayName)")?.host }))
                let urlNames = Set<String>([url.host ?? "FailedTestDomain"])

                XCTAssertFalse(urlNames.isSubset(of: recordNames), "Data Store leaking from private tab to normal tab!")

                group.leave()
              })
          }

          group.notify(queue: .main) {
            dataStoreExpectation.fulfill()
          }
        }
      }
    }

    wait(for: [dataStoreExpectation], timeout: maxTimeout)
  }

  func testIsSecretTokenAvailable() {
    let url = URL(string: "https://www.brave.com")!
    let expectation = self.expectation(description: "No secret tokens found")

    var webViewNavigationAdapter = WebViewNavigationAdapter()
    self.tabManager.addTabsForURLs([url], zombie: false, isPrivate: false)

    let javascript =
      """
      (function() {
         var found = ''
         var parsed = []

         // search all user known functions for tokens
         var functions = []
         if (typeof navigator.credentials !== 'undefined') {
           if (typeof navigator.credentials.create !== 'undefined') {
             functions.push(navigator.credentials.create)
           }

           if (typeof navigator.credentials.get !== 'undefined') {
             functions.push(navigator.credentials.get)
           }
         }

         // search all enumerable objects for tokens
         var objects = [
           document,
           window
         ]

         if (typeof registerResponse !== 'undefined') {
           objects.push(registerResponse)
         }

         if (typeof signResponse !== 'undefined') {
           objects.push(signResponse)
         }

         if (typeof navigator.credentials !== 'undefined') {
           objects.push(navigator.credentials)
         }

         function isTokenPresent(value) {
           if (typeof value == "undefined" || typeof value.indexOf == "undefined") {
             return false;
           }
           if (value.indexOf("\(UserScriptManager.securityToken)") >= 0 || value.indexOf("\(UserScriptManager.securityToken)") >= 0) {
             return true;
           }
           return false;
         }

         function secretTokensInObject(obj) {
           if (obj == undefined) {
             return ''
           }

           if (isTokenPresent(obj.name) || isTokenPresent(obj.toString())) {
             return '' + obj
           }
           for (const [key, value] of Object.entries(obj)) {
             if (isTokenPresent(key + '') || isTokenPresent(value + '')) {
               return key + ': ' + value;
             }

             if (typeof value == 'function' && isTokenPresent(value.toString())) {
               return 'function: ' + value
             }

             if (typeof value == 'object' && value != null) {
               if (isTokenPresent(value.constructor.name)) {
                 return 'Object: ' + value.constructor.name
               } else if (value != obj && parsed.indexOf(value) == -1 && objects.indexOf(value) == -1) {
                 objects.push(value)
               }
             }
           }
           parsed.push(obj)
           return '';
         }

         function secretTokensInFunctions(func) {
           if (func == undefined) {
             return '';
           }

           if (isTokenPresent(func.toString())) {
             return func.toString();
           }
           return ''
         }

         while (objects.length > 0 && found.length == 0) {
           found = secretTokensInObject(objects.shift())
         }

         while (functions.length > 0 && found.length == 0) {
           found = secretTokensInFunctions(functions.shift())
         }
         return found
       })();
      """
    
    let scripts = Set<UserScriptManager.ScriptType>(UserScriptManager.ScriptType.allCases)
    let customScripts = Set<UserScriptType>([.farblingProtection(etld: "fake.com")])

    let group = DispatchGroup()
    for tab in self.tabManager.allTabs {
      // include all scripts
      UserScriptManager.shared.loadCustomScripts(into: tab, userScripts: scripts, customScripts: customScripts)

      group.enter()
    }

    webViewNavigationAdapter = WebViewNavigationAdapter(
      didFailListener: { _ in
        group.leave()
      },
      didFinishListener: {
        group.leave()
      })

    self.tabManager.addNavigationDelegate(webViewNavigationAdapter)

    group.notify(queue: .main) {
      let tab = self.tabManager.selectedTab
      guard let webView = tab?.webView else {
        XCTFail("WebView is not created yet")
        return
      }
      webView.evaluateSafeJavaScript(functionName: javascript, contentWorld: .page, asFunction: false) { result, error in
        guard let keys = result as? String else {
          XCTFail("Javascript error while finding secret tokens")
          expectation.fulfill()
          return
        }
        if !keys.isEmpty {
          XCTFail("Secret tokens found!" + keys)
        }
        expectation.fulfill()
      }
    }

    waitForExpectations(timeout: 60, handler: nil)
  }

  func testTabsNormalToPrivate() {
    let dataStoreExpectation = XCTestExpectation(description: "dataStorePersistence")
    var webViewNavigationAdapter = WebViewNavigationAdapter()

    destroyData {
      let url = URL(string: "https://twitter.com")!
      let otherURL = URL(string: "https://instagram.com")!

      self.tabManager.addTabsForURLs([url], zombie: false, isPrivate: false)
      if self.tabManager.allTabs.count != 1 {
        XCTFail("Error: Not all Tabs are created equally")
        return dataStoreExpectation.fulfill()
      }

      let group = DispatchGroup()
      for tab in self.tabManager.allTabs {
        guard let webView = tab.webView else {
          XCTFail("WebView is not created yet")
          return dataStoreExpectation.fulfill()
        }

        if !webView.configuration.websiteDataStore.isPersistent {
          XCTFail("Normal Tab is not storing data persistently!")
          return dataStoreExpectation.fulfill()
        }

        group.enter()
      }

      webViewNavigationAdapter = WebViewNavigationAdapter(
        didFailListener: { _ in
          group.leave()
        },
        didFinishListener: {
          group.leave()
        })
      self.tabManager.addNavigationDelegate(webViewNavigationAdapter)

      // All requests finished loading.. switch to private mode.. check the cookies..
      group.notify(queue: .main) {
        let group = DispatchGroup()
        webViewNavigationAdapter = WebViewNavigationAdapter(
          didFailListener: { _ in
            group.leave()
          },
          didFinishListener: {
            group.leave()
          })

        self.tabManager.addNavigationDelegate(webViewNavigationAdapter)
        self.tabManager.addTabsForURLs([otherURL], zombie: false, isPrivate: true)

        // When the tab manager switches to private mode from normal
        // It should keep both private and normal tabs
        if self.tabManager.tabs(withType: .private).isEmpty {
          XCTFail("Error: Private tabs not created")
          return dataStoreExpectation.fulfill()
        }

        if self.tabManager.tabs(withType: .regular).isEmpty {
          XCTFail("Error: Normal tab not created")
          return dataStoreExpectation.fulfill()
        }

        for tab in self.tabManager.tabs(withType: .private) {
          guard let webView = tab.webView else {
            XCTFail("WebView is not created yet")
            return dataStoreExpectation.fulfill()
          }

          if webView.configuration.websiteDataStore.isPersistent {
            XCTFail("Private Tab is storing data persistently!")
            return dataStoreExpectation.fulfill()
          }

          group.enter()
        }

        group.notify(queue: .main) {
          let group = DispatchGroup()

          for tab in self.tabManager.tabs(withType: .private) {
            guard let webView = tab.webView else {
              XCTFail("WebView unexpectedly died")
              return dataStoreExpectation.fulfill()
            }

            group.enter()
            webView.configuration.websiteDataStore.fetchDataRecords(
              ofTypes: WKWebsiteDataStore.allWebsiteDataTypes(),
              completionHandler: { records in
                let recordNames = Set<String>(records.compactMap({ URL(string: "http://\($0.displayName)")?.host }))
                let urlNames = Set<String>([url.host ?? "FailedTestDomain"])

                XCTAssertFalse(urlNames.isSubset(of: recordNames), "Data Store leaking from normal tab to private tab!")

                group.leave()
              })
          }

          group.notify(queue: .main) {
            dataStoreExpectation.fulfill()
          }
        }
      }
    }

    wait(for: [dataStoreExpectation], timeout: maxTimeout)
  }
}
