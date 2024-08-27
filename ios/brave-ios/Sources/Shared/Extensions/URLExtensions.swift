// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import UIKit

// MARK: - Local Resource URL Extensions
extension URL {

  public func allocatedFileSize() -> Int64 {
    // First try to get the total allocated size and in failing that, get the file allocated size
    return getResourceLongLongForKey(URLResourceKey.totalFileAllocatedSizeKey.rawValue)
      ?? getResourceLongLongForKey(URLResourceKey.fileAllocatedSizeKey.rawValue)
      ?? 0
  }

  public func getResourceValueForKey(_ key: String) -> Any? {
    let resourceKey = URLResourceKey(key)
    let keySet = Set<URLResourceKey>([resourceKey])

    var val: Any?
    do {
      let values = try resourceValues(forKeys: keySet)
      val = values.allValues[resourceKey]
    } catch _ {
      return nil
    }
    return val
  }

  mutating public func append(pathComponents: String...) {
    pathComponents.forEach {
      self.appendPathComponent($0)
    }
  }

  public func getResourceLongLongForKey(_ key: String) -> Int64? {
    return (getResourceValueForKey(key) as? NSNumber)?.int64Value
  }

  public func getResourceBoolForKey(_ key: String) -> Bool? {
    return getResourceValueForKey(key) as? Bool
  }

  public var isRegularFile: Bool {
    return getResourceBoolForKey(URLResourceKey.isRegularFileKey.rawValue) ?? false
  }

  public func lastComponentIsPrefixedBy(_ prefix: String) -> Bool {
    return (pathComponents.last?.hasPrefix(prefix) ?? false)
  }

  public func shouldRequestBeOpenedAsPopup() -> Bool {
    // List of schemes that are allowed to be opened in new tabs.
    let schemesAllowedToBeOpenedAsPopups = ["http", "https", "javascript", "about", "whatsapp"]

    // Treat `window.open("")` the same as `window.open("about:blank")`.
    if absoluteString.isEmpty {
      return true
    }

    if let scheme = scheme?.lowercased(), schemesAllowedToBeOpenedAsPopups.contains(scheme) {
      return true
    }

    return false
  }
}

// The list of permanent URI schemes has been taken from http://www.iana.org/assignments/uri-schemes/uri-schemes.xhtml
private let permanentURISchemes = [
  "aaa", "aaas", "about", "acap", "acct", "cap", "cid", "coap", "coaps", "crid", "data", "dav",
  "dict", "dns", "example", "file", "ftp", "geo", "go", "gopher", "h323", "http", "https", "iax",
  "icap", "im", "imap", "info", "ipp", "ipps", "iris", "iris.beep", "iris.lwz", "iris.xpc",
  "iris.xpcs", "itms-apps", "itms-appss", "itmss", "jabber", "javascript", "ldap", "mailto", "mid",
  "msrp", "msrps", "mtqp", "mupdate", "news", "nfs", "ni", "nih", "nntp", "opaquelocktoken",
  "pkcs11", "pop", "pres", "reload", "rtsp", "rtsps", "rtspu", "service", "session", "shttp",
  "sieve", "sip", "sips", "sms", "snmp", "soap.beep", "soap.beeps", "stun", "stuns", "tag", "tel",
  "telnet", "tftp", "thismessage", "tip", "tn3270", "turn", "turns", "tv", "urn", "vemmi", "vnc",
  "ws", "wss", "xcon", "xcon-userid", "xmlrpc.beep", "xmlrpc.beeps", "xmpp", "z39.50r", "z39.50s",
]

private let ignoredSchemes = ["data"]
private let supportedSchemes = permanentURISchemes.filter { !ignoredSchemes.contains($0) }

extension URL {

  public func withQueryParam(_ name: String, value: String) -> URL {
    var components = URLComponents(url: self, resolvingAgainstBaseURL: false)!
    let item = URLQueryItem(name: name, value: value)
    components.queryItems = (components.queryItems ?? []) + [item]
    return components.url!
  }

  public func getQuery() -> [String: String] {
    var results = [String: String]()
    let keyValues = self.query?.components(separatedBy: "&")

    if keyValues?.count ?? 0 > 0 {
      for pair in keyValues! {
        let kv = pair.components(separatedBy: "=")
        if kv.count > 1 {
          results[kv[0]] = kv[1]
        }
      }
    }

    return results
  }

  /// Returns the second level domain (SLD) of a url. It removes any subdomain/TLD
  ///
  /// E.g., https://m.foo.com/bar/baz?noo=abc#123  => foo
  public var hostSLD: String {
    guard let publicSuffix = self.publicSuffix, let baseDomain = self.baseDomain else {
      return self.normalizedHost() ?? self.absoluteString
    }
    return baseDomain.replacingOccurrences(of: ".\(publicSuffix)", with: "")
  }

  public var normalizedHostAndPath: String? {
    return normalizedHost().flatMap { $0 + self.path }
  }

  public var absoluteDisplayString: String {
    var urlString = self.absoluteString
    // For http URLs, get rid of the trailing slash if the path is empty or '/'
    if (self.scheme == "http" || self.scheme == "https") && (self.path == "/")
      && urlString.hasSuffix("/")
    {
      urlString = String(urlString[..<urlString.index(urlString.endIndex, offsetBy: -1)])
    }
    // If it's basic http, strip out the string but leave anything else in
    if urlString.hasPrefix("http://") {
      return String(urlString[urlString.index(urlString.startIndex, offsetBy: 7)...])
    } else {
      return urlString
    }
  }

  // Obtain a schemeless absolute string
  public var schemelessAbsoluteString: String {
    guard let scheme = self.scheme else { return absoluteString }
    return absoluteString.replacingOccurrences(of: "\(scheme)://", with: "")
  }

  public var schemelessAbsoluteDisplayString: String {
    var urlString = withoutWWW.schemelessAbsoluteString

    // For http URLs, get rid of the trailing slash if the path is empty or '/'
    if self.path == "/", urlString.hasSuffix("/") {
      urlString = String(urlString[..<urlString.index(urlString.endIndex, offsetBy: -1)])
    }

    return urlString
  }

  /// Returns the base domain from a given hostname. The base domain name is defined as the public domain suffix
  /// with the base private domain attached to the front. For example, for the URL www.bbc.co.uk, the base domain
  /// would be bbc.co.uk. The base domain includes the public suffix (co.uk) + one level down (bbc).
  /// - returns The base domain string for the given host name.
  public var baseDomain: String? {
    guard !isIPv6, let host = host else { return nil }

    // If this is just a hostname and not a FQDN, use the entire hostname.
    if !host.contains(".") {
      return host
    }

    let registry = (self as NSURL).domainAndRegistry
    return registry.isEmpty ? nil : registry
  }

  /// Returns just the domain, but with the same scheme.
  ///
  /// E.g., https://m.foo.com/bar/baz?noo=abc#123  => https://foo.com
  ///
  /// Any failure? Return this URL.
  public var domainURL: URL {
    if let normalized = self.normalizedHost() {
      // Use URLComponents instead of URL since the former correctly preserves
      // brackets for IPv6 hosts, whereas the latter escapes them.
      var components = URLComponents()
      components.scheme = self.scheme
      components.port = self.port
      components.host = normalized
      return components.url ?? self
    }

    return self
  }

  public var withoutFragment: URL {
    var components = URLComponents(url: self, resolvingAgainstBaseURL: false)
    components?.fragment = nil
    return components?.url ?? self
  }

  public var withoutWWW: URL {
    if let normalized = self.normalizedHost(stripWWWSubdomainOnly: true),
      var components = URLComponents(url: self, resolvingAgainstBaseURL: false)
    {
      components.scheme = self.scheme
      components.port = self.port
      components.host = normalized
      return components.url ?? self
    }

    return self
  }

  public func normalizedHost(stripWWWSubdomainOnly: Bool = false) -> String? {
    // Use components.host instead of self.host since the former correctly preserves
    // brackets for IPv6 hosts, whereas the latter strips them.
    guard let components = URLComponents(url: self, resolvingAgainstBaseURL: false),
      var host = components.host, host != ""
    else {
      return nil
    }

    let textToReplace = stripWWWSubdomainOnly ? "^(www)\\." : "^(www|mobile|m)\\."

    if let range = host.range(of: textToReplace, options: .regularExpression) {
      host.replaceSubrange(range, with: "")
    }

    return host
  }

  /// Returns the public portion of the host name determined by the public suffix list found here: https://publicsuffix.org/list/.
  /// For example for the url www.bbc.co.uk, based on the entries in the TLD list, the public suffix would return co.uk.
  /// - returns The public suffix for within the given hostname.
  public var publicSuffix: String? {
    let registry = (self as NSURL).registry
    return registry.isEmpty ? nil : registry
  }

  public func isWebPage(includeDataURIs: Bool = true) -> Bool {
    let schemes = includeDataURIs ? ["http", "https", "data"] : ["http", "https"]
    return scheme.map { schemes.contains($0) } ?? false
  }

  public func isSecureWebPage() -> Bool {
    return scheme?.contains("https") ?? false
  }

  // This helps find local urls that we do not want to show loading bars on.
  // These utility pages should be invisible to the user
  public var isLocalUtility: Bool {
    guard self.isLocal else {
      return false
    }
    let utilityURLs = [
      "/\(InternalURL.Path.errorpage.rawValue)", "/\(InternalURL.Path.sessionrestore.rawValue)",
      "/about/home", "/\(InternalURL.Path.readermode.rawValue)",
    ]
    return utilityURLs.contains { self.path.hasPrefix($0) }
  }

  public var isLocal: Bool {
    guard isWebPage(includeDataURIs: false) else {
      return false
    }
    // iOS forwards hostless URLs (e.g., http://:6571) to localhost.
    guard let host = host, !host.isEmpty else {
      return true
    }

    return host.lowercased() == "localhost" || host == "127.0.0.1"
  }

  public var isIPv6: Bool {
    return host?.contains(":") ?? false
  }

  /// Returns whether the URL's scheme is one of those listed on the official list of URI schemes.
  /// This only accepts permanent schemes: historical and provisional schemes are not accepted.
  public var schemeIsValid: Bool {
    guard let scheme = scheme else { return false }
    return supportedSchemes.contains(scheme.lowercased())
  }

  public func havingRemovedAuthorisationComponents() -> URL {
    guard var urlComponents = URLComponents(url: self, resolvingAgainstBaseURL: false) else {
      return self
    }
    urlComponents.user = nil
    urlComponents.password = nil
    if let url = urlComponents.url {
      return url
    }
    return self
  }
}

// Helpers to deal with ErrorPage URLs

extension URL {

  // Check if the website is a night mode blocked site
  public var isNightModeBlockedURL: Bool {
    guard let host = self.normalizedHostAndPath else {
      return false
    }

    /// Site domains that should not inject night mode
    let majorsiteList = [
      "twitter", "youtube", "twitch",
      "soundcloud", "github", "netflix",
      "imdb", "mail.proton", "amazon",
      "x",
    ]

    let searchSiteList = [
      "search.brave", "google", "qwant",
      "startpage", "duckduckgo", "presearch",
    ]

    let devSiteList = ["macrumors", "9to5mac", "developer.apple"]

    let casualSiteList = [
      "wowhead", "xbox", "thegamer",
      "cineplex", "starwars",
    ]

    let darkModeEnabledSiteList =
      majorsiteList + searchSiteList + devSiteList + casualSiteList

    return darkModeEnabledSiteList.contains(where: host.contains)
  }

  // Check if the website is search engine
  public var isSearchEngineURL: Bool {
    guard let host = self.normalizedHostAndPath else {
      return false
    }

    /// Site domains that is considered as search engine
    let siteList = [
      "google", "bing", "duckduckgo",
      "ecosia", "qwant", "startpage",
      "yandex", "search.brave",
    ]

    return siteList.contains(where: host.contains)
  }

  // Check if the website is supporting showing Add To playlist toast
  public var isPlaylistSupportedSiteURL: Bool {
    let urlHost = self.host ?? self.hostSLD

    var siteList = Set<String>([
      "youtube.com", "vimeo.com", "twitch.tv",
      "soundcloud.com", "dailymotion.com", "udemy.com",
      "floatplane.com", "newness.com", "ted.com",
      "pandora.tv", "watchnebula.com", "pbs.org",
      "curiositystream.com", "soccer-douga.com", "bitchute.com",
      "rumble.com", "gorf.tv", "odysee.com", "brighteon.com",
      "lbry.tv", "luminarypodcasts.com", "marthastewart.com",
      "bbcgoodfood.com", "bt.com", "skysports.com", "sky.co.nz",
      "kayosports.com.au", "listennotes.com", "vid.puffyan.us",
      "twitter.com", "x.com",
    ])

    /// Additional sites for Japanese locale
    if Locale.current.region?.identifier == "JP" {
      let japanList = Set<String>([
        "nicovideo.jp", "video.fc2.com", "musicpv.jp",
        "openrec.tv", "mirrativ.com", "mildom.com",
        "twitcasting.tv", "creators.yahoo.co.jp",
        "jp.voicetube.com", "openclassrooms.com",
        "udacity.com", "coursera.org", "edx.org",
        "3mcompany.jp", "eikoh-lms.com", "eikoh-http.akamaized.net",
        "asuka-academy.com", "chugakujuken.com", "ic0.tv",
        "aoi-zemi.com", "prog-8.com", "jmooc.jp", "schoo.jp",
        "nlp.netlearning.co.jp", "gacco.org", "dic.okedou.app",
        "okedou.app", "sports.yahoo.co.jp", "soccer.skyperfectv.co.jp",
      ])
      siteList.formUnion(japanList)
    }

    return siteList.contains(where: urlHost.contains)
  }

  public var isPlaylistBlockedSiteURL: Bool {
    let urlHost = self.host ?? self.hostSLD
    return urlHost == "talk.brave.com"
  }

  public func uniquePathForFilename(_ filename: String) throws -> URL {
    let basePath = self.appendingPathComponent(filename)
    let fileExtension = basePath.pathExtension
    let filenameWithoutExtension =
      !fileExtension.isEmpty ? String(filename.dropLast(fileExtension.count + 1)) : filename

    var proposedPath = basePath
    var count = 0

    while FileManager.default.fileExists(atPath: proposedPath.path) {
      count += 1

      let proposedFilenameWithoutExtension = "\(filenameWithoutExtension) (\(count))"
      proposedPath = self.appendingPathComponent(proposedFilenameWithoutExtension)
        .appendingPathExtension(fileExtension)
    }

    return proposedPath
  }
}

// Helper To deal with Bookmark URLs

extension URL {
  public var isBookmarklet: Bool {
    if let scheme = self.scheme {
      return scheme == "javascript"
    }
    return self.absoluteString.hasPrefix("javascript:")
  }

  public var bookmarkletCodeComponent: String? {
    if self.isBookmarklet {
      // Chrome, Safari, Desktop all treat anything after `javascript:` including `//` as the component!
      // So `javascript:/` has a component of `/`
      // `javascript://test` has a component of `//test`
      if self.absoluteString.hasPrefix("javascript:") {
        if let result = String(self.absoluteString.dropFirst("javascript:".count))
          .removingPercentEncoding
        {
          return result.isEmpty ? nil : result
        }
      }
    }
    return nil
  }

  public static func bookmarkletURL(from string: String) -> URL? {
    if string.hasPrefix("javascript:"),
      let escaped = string.addingPercentEncoding(withAllowedCharacters: .urlAllowed)
    {
      return URL(string: escaped)
    }
    return nil
  }
}

extension String {
  public func removeSchemeFromURLString(_ scheme: String?) -> String {
    guard let scheme = scheme else {
      return self
    }

    return replacingOccurrences(of: "\(scheme)://", with: "")
  }
}

// MARK: Helpers to deal with ErrorPage URLs

public struct InternalURL {
  public static let uuid = UUID().uuidString
  public static let scheme = "internal"
  public static let host = "local"
  public static let baseUrl = "\(scheme)://\(host)"

  public enum Path: String {
    case errorpage
    case sessionrestore
    case readermode = "reader-mode"
    case blocked
    case httpBlocked = "http-blocked"
    case basicAuth = "basic-auth"

    func matches(_ string: String) -> Bool {
      return string.range(
        of: "/?\(self.rawValue)",
        options: .regularExpression,
        range: nil,
        locale: nil
      ) != nil
    }
  }

  public enum Param: String {
    case uuidkey = "uuidkey"
    case url = "url"
    func matches(_ string: String) -> Bool { return string == self.rawValue }
  }

  public let url: URL
  private let sessionRestoreHistoryItemBaseUrl =
    "\(InternalURL.baseUrl)/\(InternalURL.Path.sessionrestore.rawValue)?url="

  public static func isValid(url: URL) -> Bool {
    return InternalURL.scheme == url.scheme && InternalURL.host == url.host
  }

  public init?(_ url: URL) {
    guard InternalURL.isValid(url: url) else {
      return nil
    }

    self.url = url
  }

  public var isAuthorized: Bool {
    return (url.getQuery()[InternalURL.Param.uuidkey.rawValue] ?? "") == InternalURL.uuid
  }

  public var stripAuthorization: String {
    guard var components = URLComponents(string: url.absoluteString),
      let items = components.queryItems
    else { return url.absoluteString }
    components.queryItems = items.filter { !Param.uuidkey.matches($0.name) }
    if let items = components.queryItems, items.count == 0 {
      components.queryItems = nil  // This cleans up the url to not end with a '?'
    }
    return components.url?.absoluteString ?? ""
  }

  public static func authorize(url: URL) -> URL? {
    guard var components = URLComponents(string: url.absoluteString) else { return nil }
    if components.queryItems == nil {
      components.queryItems = []
    }

    if var item = components.queryItems?.first(where: { Param.uuidkey.matches($0.name) }) {
      item.value = InternalURL.uuid
    } else {
      components.queryItems?.append(
        URLQueryItem(name: Param.uuidkey.rawValue, value: InternalURL.uuid)
      )
    }
    return components.url
  }

  public var isSessionRestore: Bool {
    return url.absoluteString.hasPrefix(sessionRestoreHistoryItemBaseUrl)
  }

  public var isErrorPage: Bool {
    // Error pages can be nested in session restore URLs, and session restore handler will forward them to the error page handler
    let path =
      url.absoluteString.hasPrefix(sessionRestoreHistoryItemBaseUrl)
      ? extractedUrlParam?.path : url.path
    return InternalURL.Path.errorpage.matches(path ?? "")
  }

  public var isBlockedPage: Bool {
    return InternalURL.Path.blocked.matches(url.path)
  }

  public var isHTTPBlockedPage: Bool {
    return InternalURL.Path.httpBlocked.matches(url.path)
  }

  public var isReaderModePage: Bool {
    return InternalURL.Path.readermode.matches(url.path)
  }

  public var isBasicAuthURL: Bool {
    return InternalURL.Path.basicAuth.matches(url.path)
  }

  public var originalURLFromErrorPage: URL? {
    if !url.absoluteString.hasPrefix(sessionRestoreHistoryItemBaseUrl) {
      return isErrorPage ? extractedUrlParam : nil
    }
    if let urlParam = extractedUrlParam, let nested = InternalURL(urlParam), nested.isErrorPage {
      return nested.extractedUrlParam
    }
    return nil
  }

  public var extractedUrlParam: URL? {
    if let nestedUrl = url.getQuery()[InternalURL.Param.url.rawValue]?.unescape() {
      return URL(string: nestedUrl)
    }
    return nil
  }

  public var isAboutHomeURL: Bool {
    if let urlParam = extractedUrlParam, let internalUrlParam = InternalURL(urlParam) {
      return internalUrlParam.aboutComponent?.hasPrefix("home") ?? false
    }
    return aboutComponent?.hasPrefix("home") ?? false
  }

  public var isAboutURL: Bool {
    return aboutComponent != nil
  }

  /// Return the path after "about/" in the URI.
  public var aboutComponent: String? {
    let aboutPath = "/about/"
    guard let url = URL(string: stripAuthorization) else {
      return nil
    }

    if url.path.hasPrefix(aboutPath) {
      return String(url.path.dropFirst(aboutPath.count))
    }
    return nil
  }

  public var isWeb3URL: Bool {
    return web3Component != nil
  }

  /// Return the path after "web3/" in the URI.
  public var web3Component: String? {
    let web3Path = "/web3/"
    guard let url = NSURL(idnString: stripAuthorization) as? URL else {
      return nil
    }

    if url.path.hasPrefix(web3Path) {
      return String(url.path.dropFirst(web3Path.count))
    }
    return nil
  }

  public var displayURL: URL? {
    if isErrorPage {
      return originalURLFromErrorPage
    } else if isReaderModePage {
      return extractedUrlParam
    }
    return nil
  }
}
