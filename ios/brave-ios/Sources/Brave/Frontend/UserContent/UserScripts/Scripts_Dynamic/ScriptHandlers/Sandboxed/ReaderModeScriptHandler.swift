// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import SwiftyJSON
import WebKit
import os.log

let readerModeProfileKeyStyle = "readermode.style"

enum ReaderModeMessageType: String {
  case stateChange = "ReaderModeStateChange"
  case pageEvent = "ReaderPageEvent"
  case contentParsed = "ReaderContentParsed"
}

enum ReaderPageEvent: String {
  case pageShow = "PageShow"
}

enum ReaderModeState: String {
  case available = "Available"
  case unavailable = "Unavailable"
  case active = "Active"
}

enum ReaderModeTheme: String {
  case light = "light"
  case dark = "dark"
  case sepia = "sepia"
  case black = "black"

  var backgroundColor: UIColor {
    switch self {
    case .light:
      return .white
    case .dark:
      return .darkGray
    case .sepia:
      return .init(rgb: 0xf0e6dc)  // Light Beige
    case .black:
      return .black
    }
  }
}

enum ReaderModeFontType: String {
  case serif = "serif"
  case sansSerif = "sans-serif"
}

enum ReaderModeFontSize: Int {
  case size1 = 1
  case size2 = 2
  case size3 = 3
  case size4 = 4
  case size5 = 5
  case size6 = 6
  case size7 = 7
  case size8 = 8
  case size9 = 9
  case size10 = 10
  case size11 = 11
  case size12 = 12
  case size13 = 13

  func isSmallest() -> Bool {
    return self == ReaderModeFontSize.size1
  }

  func smaller() -> ReaderModeFontSize {
    if isSmallest() {
      return self
    } else {
      return ReaderModeFontSize(rawValue: self.rawValue - 1)!
    }
  }

  func isLargest() -> Bool {
    return self == ReaderModeFontSize.size13
  }

  static var defaultSize: ReaderModeFontSize {
    switch UIApplication.shared.preferredContentSizeCategory {
    case .extraSmall:
      return .size1
    case .small:
      return .size2
    case .medium:
      return .size3
    case .large:
      return .size5
    case .extraLarge:
      return .size7
    case .extraExtraLarge:
      return .size9
    case .extraExtraExtraLarge:
      return .size12
    default:
      return .size5
    }
  }

  func bigger() -> ReaderModeFontSize {
    if isLargest() {
      return self
    } else {
      return ReaderModeFontSize(rawValue: self.rawValue + 1)!
    }
  }
}

struct ReaderModeStyle {
  var theme: ReaderModeTheme
  var fontType: ReaderModeFontType
  var fontSize: ReaderModeFontSize

  /// Encode the style to a JSON dictionary that can be passed to ReaderMode.js
  func encode() -> String {
    return JSON(
      ["theme": theme.rawValue, "fontType": fontType.rawValue, "fontSize": fontSize.rawValue]
        as [String: Any]
    ).stringValue() ?? ""
  }

  /// Encode the style to a dictionary that can be stored in the profile
  func encodeAsDictionary() -> [String: Any] {
    return ["theme": theme.rawValue, "fontType": fontType.rawValue, "fontSize": fontSize.rawValue]
  }

  init(theme: ReaderModeTheme, fontType: ReaderModeFontType, fontSize: ReaderModeFontSize) {
    self.theme = theme
    self.fontType = fontType
    self.fontSize = fontSize
  }

  /// Initialize the style from a dictionary, taken from the profile. Returns nil if the object cannot be decoded.
  init?(dict: [String: Any]) {
    let themeRawValue = dict["theme"] as? String
    let fontTypeRawValue = dict["fontType"] as? String
    let fontSizeRawValue = dict["fontSize"] as? Int
    if themeRawValue == nil || fontTypeRawValue == nil || fontSizeRawValue == nil {
      return nil
    }

    let theme = ReaderModeTheme(rawValue: themeRawValue!)
    let fontType = ReaderModeFontType(rawValue: fontTypeRawValue!)
    let fontSize = ReaderModeFontSize(rawValue: fontSizeRawValue!)
    if theme == nil || fontType == nil || fontSize == nil {
      return nil
    }

    self.theme = theme!
    self.fontType = fontType!
    self.fontSize = fontSize!
  }
}

let defaultReaderModeStyle = ReaderModeStyle(
  theme: .light,
  fontType: .sansSerif,
  fontSize: ReaderModeFontSize.defaultSize
)

/// This struct captures the response from the Readability.js code.
struct ReadabilityResult {
  var domain = ""
  var url = ""
  var content = ""
  var title = ""
  var credits = ""
  var direction = "auto"
  var cspMetaTags = [String]()

  init?(object: AnyObject?) {
    if let dict = object as? NSDictionary {
      guard JSONSerialization.isValidJSONObject(dict) else {
        return nil
      }

      if let uri = dict["uri"] as? NSDictionary {
        if let url = uri["spec"] as? String {
          self.url = url
        }
        if let host = uri["host"] as? String {
          self.domain = host
        }
      }
      if let content = dict["content"] as? String {
        self.content = content
      }
      if let title = dict["title"] as? String {
        self.title = title
      }
      if let credits = dict["byline"] as? String {
        self.credits = credits
      }
      if let direction = dict["dir"] as? String {
        self.direction = direction
      }
      if let cspMetaTags = dict["cspMetaTags"] as? [String] {
        self.cspMetaTags = cspMetaTags
      }
    } else {
      return nil
    }
  }

  /// Initialize from a JSON encoded string
  init?(string: String) {
    let object = JSON(parseJSON: string)
    let domain = object["domain"].string
    let url = object["url"].string
    let content = object["content"].string
    let title = object["title"].string
    let credits = object["credits"].string
    let direction = object["dir"].string
    let cspMetaTags = object["cspMetaTags"].arrayObject as? [String]

    if domain == nil || url == nil || content == nil || title == nil || credits == nil {
      return nil
    }

    self.domain = domain!
    self.url = url!
    self.content = content!
    self.title = title!
    self.credits = credits!
    self.direction = direction ?? "auto"
    self.cspMetaTags = cspMetaTags ?? []
  }

  /// Encode to a dictionary, which can then for example be json encoded
  func encode() -> [String: Any] {
    return [
      "domain": domain, "url": url, "content": content, "title": title, "credits": credits,
      "dir": direction, "cspMetaTags": cspMetaTags,
    ]
  }

  /// Encode to a JSON encoded string
  func encode() -> String {
    let dict: [String: Any] = self.encode()
    return JSON(dict).stringValue()!
  }
}

/// Delegate that contains callbacks that we have added on top of the built-in WKWebViewDelegate
protocol ReaderModeScriptHandlerDelegate: AnyObject {
  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didChangeReaderModeState state: ReaderModeState,
    forTab tab: Tab
  )
  func readerMode(_ readerMode: ReaderModeScriptHandler, didDisplayReaderizedContentForTab tab: Tab)
  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didParseReadabilityResult readabilityResult: ReadabilityResult,
    forTab tab: Tab
  )
}

let readerModeNamespace = "window.__firefox__.reader"

class ReaderModeScriptHandler: TabContentScript {
  weak var delegate: ReaderModeScriptHandlerDelegate?

  var state: ReaderModeState = ReaderModeState.unavailable
  fileprivate var originalURL: URL?

  static let scriptName = "ReaderModeScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "readerModeMessageHandler"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

  fileprivate func handleReaderPageEvent(_ readerPageEvent: ReaderPageEvent, tab: Tab) {
    switch readerPageEvent {
    case .pageShow:
      delegate?.readerMode(self, didDisplayReaderizedContentForTab: tab)
    }
  }

  fileprivate func handleReaderModeStateChange(_ state: ReaderModeState, tab: Tab) {
    self.state = state
    delegate?.readerMode(self, didChangeReaderModeState: state, forTab: tab)
  }

  fileprivate func handleReaderContentParsed(_ readabilityResult: ReadabilityResult, tab: Tab) {
    delegate?.readerMode(self, didParseReadabilityResult: readabilityResult, forTab: tab)
  }

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    guard let body = message.body as? [String: AnyObject] else {
      return
    }

    if let msg = body["data"] as? [String: Any] {
      if let messageType = ReaderModeMessageType(rawValue: msg["Type"] as? String ?? "") {
        switch messageType {
        case .pageEvent:
          if let readerPageEvent = ReaderPageEvent(rawValue: msg["Value"] as? String ?? "Invalid") {
            handleReaderPageEvent(readerPageEvent, tab: tab)
          }
        case .stateChange:
          if let readerModeState = ReaderModeState(rawValue: msg["Value"] as? String ?? "Invalid") {
            handleReaderModeStateChange(readerModeState, tab: tab)
          }
        case .contentParsed:
          if let readabilityResult = ReadabilityResult(object: msg["Value"] as AnyObject?) {
            handleReaderContentParsed(readabilityResult, tab: tab)
          } else {
            handleReaderModeStateChange(.unavailable, tab: tab)
          }
        }
      }
    }
  }

  func setStyle(_ style: ReaderModeStyle, in tab: Tab) {
    if state == ReaderModeState.active {
      tab.webView?.evaluateSafeJavaScript(
        functionName: "\(readerModeNamespace).setStyle",
        args: [style.encode()],
        contentWorld: Self.scriptSandbox,
        escapeArgs: false
      ) { (object, error) -> Void in
        return
      }
    }
  }

  static func cache(for tab: Tab?) -> ReaderModeCache {
    switch TabType.of(tab) {
    case .regular:
      return DiskReaderModeCache.sharedInstance
    case .private:
      return MemoryReaderModeCache.sharedInstance
    }
  }

}
