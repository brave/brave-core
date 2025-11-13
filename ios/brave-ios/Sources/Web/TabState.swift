// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import FaviconModels
import Foundation
import UIKit
import WebKit

public enum UserAgentType {
  case none
  case automatic
  case mobile
  case desktop
}

public enum SecureContentState {
  case unknown
  case localhost
  case secure
  case invalidCertificate
  case missingSSL
  case mixedContent
}

public class TabStateFactory {
  public struct CreateTabParams {
    public var id: UUID
    public var initialConfiguration: WKWebViewConfiguration?
    public var lastActiveTime: Date?
    public var braveCore: BraveProfileController?

    public init(
      id: UUID = .init(),
      initialConfiguration: WKWebViewConfiguration? = nil,
      lastActiveTime: Date? = nil,
      braveCore: BraveProfileController? = nil
    ) {
      self.id = id
      self.initialConfiguration = initialConfiguration
      self.lastActiveTime = lastActiveTime
      self.braveCore = braveCore
    }
  }

  public static func create(with params: CreateTabParams) -> any TabState {
    let wkConfiguration = params.initialConfiguration ?? .init()
    wkConfiguration.enablePageTopColorSampling()
    if let braveCore = params.braveCore, FeatureList.kUseChromiumWebViews.enabled {
      let cwvConfiuration =
        wkConfiguration.websiteDataStore.isPersistent
        ? braveCore.defaultWebViewConfiguration
        : braveCore.nonPersistentWebViewConfiguration
      return ChromiumTabState(
        id: params.id,
        configuration: cwvConfiuration,
        wkConfiguration: wkConfiguration
      )
    }
    let webKitTabState = WebKitTabState(id: params.id, configuration: wkConfiguration)
    if let lastActiveTime = params.lastActiveTime {
      webKitTabState.lastActiveTime = lastActiveTime
    }
    return webKitTabState
  }
}

/// An error that may occur while restoring tab data
public enum TabRestorationError: Error {
  /// The data was not valid
  case invalidData
}

/// Core interface for interaction with the web
@dynamicMemberLookup
public protocol TabState: AnyObject {
  typealias ID = UUID
  /// A unique identifier associated with this TabState
  var id: ID { get }
  /// Whehter or not the TabState persists data
  var isPrivate: Bool { get }
  /// Arbitrary data that is associated with this TabState
  var data: TabDataValues { get set }
  /// The view containing the contents of the current web page.
  var view: UIView { get }
  /// The TabState that was responsibile for opening this page
  var opener: (any TabState)? { get set }
  /// Whether or not the web contents are visible to the user.
  var isVisible: Bool { get set }
  /// The last time this TabState was made active (either by creation or when visibility changed)
  var lastActiveTime: Date? { get }
  /// Gets a proxy for the underlying web view container
  var webViewProxy: WebViewProxy? { get }
  /// Whether or not the underlying web view has been created
  var isWebViewCreated: Bool { get }
  /// Creates the web view if one has not already been created
  func createWebView()
  /// Destroys the web view if one has been created
  func deleteWebView()

  // MARK: - Restoration

  /// Restoration session data associated with this TabState.
  var sessionData: Data? { get }
  /// Attempts to restore the TabState using the session data provided
  ///
  /// - Throws: `RestorationError` if restoration fails
  func restore(using sessionData: Data) throws
  /// Whether or not the TabState is restoring data.
  ///
  /// This is set to true when `restore(using:)` is called and flipped off once the navigation
  /// is committed
  var isRestoring: Bool { get }

  // MARK: - Delegates, Observeration and Policy Decisions

  /// The delegate that will handle events that may need to be handled by the user or are otherwise
  /// required to implement certain functionality in this TabState
  var delegate: TabDelegate? { get set }
  /// The delegate that will handle downloading files from the web
  var downloadDelegate: TabDownloadDelegate? { get set }
  /// Adds a policy decider to this TabState
  func addPolicyDecider(_ policyDecider: some TabPolicyDecider)
  /// Removes a policy decider that was previously added to this TabState
  func removePolicyDecider(_ policyDecider: some TabPolicyDecider)
  /// Adds an observer to this TabState
  func addObserver(_ observer: some TabObserver)
  /// Removes an observer that was previously added to this TabState
  func removeObserver(_ observer: some TabObserver)

  // MARK: - State

  /// The current pages active secure content state
  var visibleSecureContentState: SecureContentState { get }
  /// The certificiate assoicated with this page if one exists
  var serverTrust: SecTrust? { get }
  /// The current pages favicon
  // TODO: Should be get only, make favicon fetch logic internal (brave/brave-browser#45095)
  var favicon: Favicon? { get set }
  /// The current URL loaded on the page, regardless of the navigation status or spoofing
  @available(iOS, deprecated, message: "Use `visibleURL` or `lastCommittedURL` instead")
  var url: URL? { get }
  /// Gets the URL currently being displayed in the URL bar, if there is one.
  ///
  /// This URL might be a pending navigation that hasn't committed yet, so it is not guaranteed to
  /// match the current page in this TabState.
  var visibleURL: URL? { get }
  /// The URL associated with the most recent navigation that committed and represents the current
  /// security context
  var lastCommittedURL: URL? { get }
  /// The URL associated with the previous navigation that committed
  var previousCommittedURL: URL? { get }
  /// The MIME type for the current page
  var contentsMimeType: String? { get }
  /// The title for the page
  var title: String? { get }
  /// Whether or not the page is currently loading
  var isLoading: Bool { get }
  /// The estimated progress of a navigation (0-1)
  var estimatedProgress: Double { get }
  /// Whether or not the page can navigate backwards
  var canGoBack: Bool { get }
  /// Whether or not the page can navigate forwards
  var canGoForward: Bool { get }
  /// The current back forward list
  var backForwardList: (any BackForwardListProxy)? { get }
  /// The current redirect chain for the navigation.
  ///
  /// If no redirects occur during the navigation, this only contains the original request URL
  @available(
    iOS,
    deprecated,
    message: "Assemble a redirect chain in a tab helper using TabObserver instead"
  )
  var redirectChain: [URL] { get }
  /// The original request for the current page
  ///
  /// Remove when CWVBackFowardListItem exposes original request URL
  var currentInitialURL: URL? { get }
  /// The current user agent type used to display this page
  var currentUserAgentType: UserAgentType { get }

  // MARK: - Interaction

  /// Starts a navigation for the given request
  func loadRequest(_ request: URLRequest)
  /// Override the current `visibleURL`
  ///
  /// This should only be used in certain scenarios such as restoring a tab, creating a child tab as
  /// typically the `visibleURL` property is updated based on web navigations.
  ///
  /// - warning: This does not notify TabObserver's that the url has changed
  func setVirtualURL(_ url: URL?)
  /// Reloads the page contents
  func reload()
  /// Reloads the page contents with an explicit user agent type
  func reloadWithUserAgentType(_ userAgentType: UserAgentType)
  /// Stops any active load
  func stopLoading()
  /// Goes to the previous item in the back forward list
  func goBack()
  /// Goes to the next item in the back forward list
  func goForward()
  /// Goes to an explicit item in the back forward list
  func goToBackForwardListItem(_ item: any BackForwardListItemProxy)
  /// Whether or not the TabState can be captured in a snapshot
  var canTakeSnapshot: Bool { get }
  /// Takes a snapshot of the web contents with the given rect. Pass in a `null` rect to capture
  /// the entire page. `handler` may be called more than once
  func takeSnapshot(rect: CGRect, handler: @escaping (UIImage?) -> Void)
  /// Creates a full page PDF of the web contents
  func createFullPagePDF() async throws -> Data?
  /// Presents the find in page interaction using the provided text as an initial search query
  func presentFindInteraction(with text: String)
  /// Dismisses any active find in page interactions
  func dismissFindInteraction()
  /// Evaluates some JavaScript on the page content world without any escaping.
  ///
  /// This should only be used for non-function calling JavaScript that does not take any explicit
  /// arguments or input. For all other JavaScript execution use `evaluateJavaScript`
  func evaluateJavaScriptUnsafe(_ javascript: String)

  // MARK: - WebKit specific
  // These APIs require the use of WKWebView and its associated APIs as there are no WebState
  // alternatives for them or require the use of private API

  /// Loads HTML content directly into the page
  func loadHTMLString(_ htmlString: String, baseURL: URL?)
  /// Evaluates some JavaScript in a safe manner on a given frame & content world
  @discardableResult
  @MainActor func evaluateJavaScript(
    functionName: String,
    args: [Any],
    frame: WKFrameInfo?,
    contentWorld: WKContentWorld,
    escapeArgs: Bool,
    asFunction: Bool
  ) async throws -> Any?
  /// See `-[WKWebView callAsyncJavaScript:]`
  @discardableResult
  func callAsyncJavaScript(
    _ functionBody: String,
    arguments: [String: Any],
    in frame: WKFrameInfo?,
    contentWorld: WKContentWorld
  ) async throws -> Any?
  /// The configuration assosciated with the underlying WKWebView.
  ///
  /// This will be the initial configuration passed in TabStateFactory until `isWebViewCreated`
  /// is true, at which point, it will be the configuration associated with the web view
  var configuration: WKWebViewConfiguration { get }
  /// The print formatter associated with the underlying web view to allow for printing the page
  var viewPrintFormatter: UIViewPrintFormatter? { get }
  /// Returns the PDF data for the current page if one is being displayed
  var dataForDisplayedPDF: Data? { get }
  /// Returns a colour that was sampled from the top of the page for UI purposes
  var sampledPageTopColor: UIColor? { get }
  /// The scale applied to the WKWebView which can be used to apply custom zoom settings to a page
  var viewScale: CGFloat { get set }
  /// Clears the back forward list of the WKWebView
  func clearBackForwardList()

  // MARK: - Chromium specific
  func updateScripts()
}

extension TabState {
  /// Reloads the page with the user agent type swapped
  public func switchUserAgent() {
    reloadWithUserAgentType(currentUserAgentType == .mobile ? .desktop : .mobile)
  }

  public subscript<Value>(dynamicMember member: KeyPath<TabDataValues, Value>) -> Value {
    return data[keyPath: member]
  }

  public subscript<Value>(dynamicMember member: WritableKeyPath<TabDataValues, Value>) -> Value {
    get { return data[keyPath: member] }
    set { data[keyPath: member] = newValue }
  }
}

// Default args for methods
extension TabState {
  /// Presents the find in page interaction
  public func presentFindInteraction() {
    presentFindInteraction(with: "")
  }

  /// Evaluates some JavaScript in a safe manner on a given frame & content world
  @discardableResult
  @MainActor public func evaluateJavaScript(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true
  ) async throws -> Any? {
    try await evaluateJavaScript(
      functionName: functionName,
      args: args,
      frame: frame,
      contentWorld: contentWorld,
      escapeArgs: escapeArgs,
      asFunction: asFunction
    )
  }

  /// Evaluates some JavaScript in a safe manner on a given frame & content world
  public func evaluateJavaScript(
    functionName: String,
    args: [Any] = [],
    frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld,
    escapeArgs: Bool = true,
    asFunction: Bool = true,
    completionHandler: ((Any?, Error?) -> Void)? = nil
  ) {
    Task { @MainActor in
      do {
        let result = try await evaluateJavaScript(
          functionName: functionName,
          args: args,
          frame: frame,
          contentWorld: contentWorld,
          escapeArgs: escapeArgs,
          asFunction: asFunction
        )
        completionHandler?(result, nil)
      } catch {
        completionHandler?(nil, error)
      }
    }
  }

  /// See `-[WKWebView callAsyncJavaScript:]`
  @discardableResult
  public func callAsyncJavaScript(
    _ functionBody: String,
    arguments: [String: Any] = [:],
    in frame: WKFrameInfo? = nil,
    contentWorld: WKContentWorld
  ) async throws -> Any? {
    try await callAsyncJavaScript(
      functionBody,
      arguments: arguments,
      in: frame,
      contentWorld: contentWorld
    )
  }
}

/// A basic proxy over the underlying web view that may be displaying web content
public protocol WebViewProxy {
  var scrollView: UIScrollView? { get }
  var bounds: CGRect { get }
  var frame: CGRect { get }
  func becomeFirstResponder() -> Bool
}

public protocol BackForwardListProxy {
  var backList: [any BackForwardListItemProxy] { get }
  var forwardList: [any BackForwardListItemProxy] { get }
  var currentItem: (any BackForwardListItemProxy)? { get }
  var backItem: (any BackForwardListItemProxy)? { get }
  var forwardItem: (any BackForwardListItemProxy)? { get }
}

public protocol BackForwardListItemProxy {
  var url: URL { get }
  var title: String? { get }
}

public struct TabDataValues {
  private var storage: [AnyHashable: Any] = [:]

  public subscript<Key: TabDataKey>(key: Key.Type) -> Key.Value {
    get {
      guard let helper = storage[ObjectIdentifier(key)] as? Key.Value else {
        return Key.defaultValue
      }
      return helper
    }
    set {
      storage[ObjectIdentifier(key)] = newValue
    }
  }
}

public protocol TabDataKey {
  associatedtype Value
  static var defaultValue: Value { get }
}

public protocol TabHelper {
  init(tab: some TabState)
  static var keyPath: WritableKeyPath<TabDataValues, Self?> { get }
  static func create(for tab: any TabState)
  static func remove(from tab: any TabState)
  static func from(tab: any TabState) -> Self?
}

extension TabHelper {
  public static func create(for tab: any TabState) {
    if tab.data[keyPath: keyPath] == nil {
      tab.data[keyPath: keyPath] = Self(tab: tab)
    }
  }
  public static func remove(from tab: any TabState) {
    tab.data[keyPath: keyPath] = nil
  }
  public static func from(tab: any TabState) -> Self? {
    tab.data[keyPath: keyPath]
  }
}
