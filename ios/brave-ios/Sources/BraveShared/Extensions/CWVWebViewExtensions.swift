// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

// Adds Sequence conformance to the `CWVBackForwardListItemArray` which under the hood is not
// actually an `NSArray` and only conforms to the `NSFastEnumeration` protocol
extension CWVBackForwardListItemArray: Sequence {
  public typealias Element = CWVBackForwardListItem

  // A custom Iterator is used because `NSFastEnumerationIterator.Element` is `Any`, and we can't
  // specialize it.
  public struct Iterator: IteratorProtocol {
    public typealias Element = CWVBackForwardListItem

    private var enumerator: NSFastEnumerationIterator

    fileprivate init(_ enumerable: CWVBackForwardListItemArray) {
      self.enumerator = NSFastEnumerationIterator(enumerable)
    }

    public mutating func next() -> Element? {
      return enumerator.next() as? Element
    }
  }

  public func makeIterator() -> Iterator {
    Iterator(self)
  }
}

// Helper types to properly discern between core types and qualifiers within a PageTransition
// mimics ui/base/page_transition_types.cc
extension CWVNavigationType: CustomDebugStringConvertible {
  // CWVNavigationType is marked with NS_OPTIONS_SET in the Obj-C side which turns this into a
  // struct with static lets for each case on the Swift side, but since `CWVNavigationTypeLink` is
  // equal to `0`, no symbol is generated for it because its implied that 0 should be an empty set.
  // This adds it back since link is a core type which should be treated like an enum case
  public static let link: CWVNavigationType = .init(rawValue: 0)

  /// The core navigation type, which only one value will exist from the list of CWVNavigationType
  /// cases
  public var coreType: CWVNavigationType {
    .init(rawValue: self.rawValue & ~CWVNavigationType.qualifierMask.rawValue)
  }

  /// Qualifiers that are stored within this type.
  public var qualifiers: CWVNavigationType {
    .init(rawValue: self.rawValue & CWVNavigationType.qualifierMask.rawValue)
  }

  public static func == (lhs: CWVNavigationType, rhs: CWVNavigationType) -> Bool {
    return lhs.coreType.rawValue == rhs.coreType.rawValue
  }

  public func contains(_ member: CWVNavigationType) -> Bool {
    assert(
      member.rawValue > CWVNavigationType.lastCore.rawValue,
      "\(member) is a core type, not a qualifier, replace with an equality check"
    )
    return qualifiers.rawValue & member.rawValue != 0
  }

  // FIXME: Probably need to implement rest of Set-methods

  public var isMainFrame: Bool {
    self != .autoSubframe && self != .manualSubframe
  }

  public var isRedirect: Bool {
    rawValue & CWVNavigationType.isRedirectMask.rawValue != 0
  }

  public var isNewNavigation: Bool {
    self != .reload && contains(.forwardBack)
  }

  public var isWebTriggerable: Bool {
    switch coreType {
    case .link, .autoSubframe, .manualSubframe, .formSubmit:
      return true
    default:
      return false
    }
  }

  public var debugDescription: String {
    switch coreType {
    case .link: return "link"
    case .typed: return "typed"
    case .autoBookmark: return "auto_bookmark"
    case .autoSubframe: return "auto_subframe"
    case .manualSubframe: return "manual_subframe"
    case .generated: return "generated"
    case .autoToplevel: return "auto_toplevel"
    case .formSubmit: return "form_submit"
    case .reload: return "reload"
    case .keyword: return "keyword"
    case .keywordGenerated: return "keyword_generated"
    default: return "<unknown>"
    }
  }
}

extension CWVUserAgentType: CustomDebugStringConvertible {
  public var debugDescription: String {
    switch self {
    case .none: return "none"
    case .automatic: return "automatic"
    case .mobile: return "mobile"
    case .desktop: return "desktop"
    default: return "<unknown>"
    }
  }
}

public enum JavascriptError: Error {
  case invalid
}

extension CWVWebView {
  // Use JS to redirect the page without adding a history entry
  public func replaceLocation(with url: URL) {
    let apostropheEncoded = "%27"
    let safeUrl = url.absoluteString.replacingOccurrences(of: "'", with: apostropheEncoded)
    evaluateSafeJavaScript(
      functionName: "location.replace",
      args: ["'\(safeUrl)'"],
      contentWorld: .defaultClient,
      escapeArgs: false,
      asFunction: true,
      completion: nil
    )
  }

  public func generateJSFunctionString(
    functionName: String,
    args: [Any?],
    escapeArgs: Bool = true
  ) -> (javascript: String, error: Error?) {
    var sanitizedArgs = [String]()
    for arg in args {
      if let arg = arg {
        do {
          if let arg = arg as? String {
            sanitizedArgs.append(escapeArgs ? "'\(arg.htmlEntityEncodedString)'" : "\(arg)")
          } else {
            let data = try JSONSerialization.data(withJSONObject: arg, options: [.fragmentsAllowed])

            if let str = String(data: data, encoding: .utf8) {
              sanitizedArgs.append(str)
            } else {
              throw JavascriptError.invalid
            }
          }
        } catch {
          return ("", error)
        }
      } else {
        sanitizedArgs.append("null")
      }
    }

    if args.count != sanitizedArgs.count {
      assertionFailure("Javascript parsing failed.")
      return ("", JavascriptError.invalid)
    }

    return ("\(functionName)(\(sanitizedArgs.joined(separator: ", ")))", nil)
  }

  public func evaluateSafeJavaScript(
    functionName: String,
    args: [Any] = [],
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true,
    completion: ((Any?, Error?) -> Void)? = nil
  ) {
    var javascript = functionName

    if asFunction {
      let js = generateJSFunctionString(
        functionName: functionName,
        args: args,
        escapeArgs: escapeArgs
      )
      if js.error != nil {
        if let completionHandler = completion {
          completionHandler(nil, js.error)
        }
        return
      }
      javascript = js.javascript
    }

    DispatchQueue.main.async {
      self.evaluateJavaScript(
        javascript,
        contentWorld: contentWorld,
        completionHandler: { value, error in
          completion?(value, error)
        }
      )
    }
  }

  @discardableResult @MainActor public func evaluateSafeJavaScript(
    functionName: String,
    args: [Any] = [],
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true
  ) async -> (Any?, Error?) {
    await withCheckedContinuation { continuation in
      evaluateSafeJavaScript(
        functionName: functionName,
        args: args,
        contentWorld: contentWorld,
        escapeArgs: escapeArgs,
        asFunction: asFunction
      ) { value, error in
        continuation.resume(returning: (value, error))
      }
    }
  }

  @discardableResult
  @MainActor public func evaluateSafeJavaScriptThrowing(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true
  ) async throws -> Any? {
    let result = await evaluateSafeJavaScript(
      functionName: functionName,
      args: args,
      contentWorld: contentWorld,
      escapeArgs: escapeArgs,
      asFunction: asFunction
    )

    if let error = result.1 {
      throw error
    } else {
      return result.0
    }
  }
}
