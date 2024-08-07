// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit
import XCTest

@testable import Shared

extension String {
  fileprivate var isIPv6: Bool {
    return self.contains(":")
  }

  fileprivate var baseDomain: String? {
    guard !NSURL.isHostIPAddress(host: self) else { return nil }

    // If this is just a hostname and not a FQDN, use the entire hostname.
    if !self.contains(".") {
      return self
    }

    let registry = NSURL.domainAndRegistry(host: self)
    return registry.isEmpty ? nil : registry
  }

  fileprivate var publicSuffix: String? {
    let registry = NSURL.registry(host: self)
    return registry.isEmpty ? nil : registry
  }
}

class NSURLExtensionsTests: XCTestCase {
  func testRemovesHTTPFromURL() {
    let url = URL(string: "http://google.com")
    if let actual = url?.absoluteDisplayString {
      XCTAssertEqual(actual, "google.com")
    } else {
      XCTFail("Actual url is nil")
    }
  }

  func testRemovesHTTPAndTrailingSlashFromURL() {
    let url = URL(string: "http://google.com/")
    if let actual = url?.absoluteDisplayString {
      XCTAssertEqual(actual, "google.com")
    } else {
      XCTFail("Actual url is nil")
    }
  }

  func testRemovesHTTPButNotTrailingSlashFromURL() {
    let url = URL(string: "http://google.com/foo/")
    if let actual = url?.absoluteDisplayString {
      XCTAssertEqual(actual, "google.com/foo/")
    } else {
      XCTFail("Actual url is nil")
    }
  }

  func testKeepsHTTPSInURL() {
    let url = URL(string: "https://google.com")
    if let actual = url?.absoluteDisplayString {
      XCTAssertEqual(actual, "https://google.com")
    } else {
      XCTFail("Actual url is nil")
    }
  }

  func testKeepsHTTPSAndRemovesTrailingSlashInURL() {
    let url = URL(string: "https://google.com/")
    if let actual = url?.absoluteDisplayString {
      XCTAssertEqual(actual, "https://google.com")
    } else {
      XCTFail("Actual url is nil")
    }
  }

  func testKeepsHTTPSAndTrailingSlashInURL() {
    let url = URL(string: "https://google.com/foo/")
    if let actual = url?.absoluteDisplayString {
      XCTAssertEqual(actual, "https://google.com/foo/")
    } else {
      XCTFail("Actual url is nil")
    }
  }

  func testSchemelessAbsouluteDisplayString() {
    // Test removes HTTP scheme from URL
    let testURL1 = URL(string: "http://brave.com")

    if let actual = testURL1?.schemelessAbsoluteDisplayString {
      XCTAssertEqual(actual, "brave.com")
    } else {
      XCTFail("Actual url is nil")
    }

    // Test removes HTTP scheme and trailing slash from URL
    let testURL2 = URL(string: "http://brave.com/")

    if let actual = testURL2?.schemelessAbsoluteDisplayString {
      XCTAssertEqual(actual, "brave.com")
    } else {
      XCTFail("Actual url is nil")
    }

    // Test removes HTTP scheme but not trailing slash because path is not empty
    let testURL3 = URL(string: "http://brave.com/foo/")
    if let actual = testURL3?.schemelessAbsoluteDisplayString {
      XCTAssertEqual(actual, "brave.com/foo/")
    } else {
      XCTFail("Actual url is nil")
    }

    // Test removes HTTPS scheme from URL
    let testURL4 = URL(string: "https://brave.com")
    if let actual = testURL4?.schemelessAbsoluteDisplayString {
      XCTAssertEqual(actual, "brave.com")
    } else {
      XCTFail("Actual url is nil")
    }

    // Test removes HTTPS scheme and trailing slash from URL
    let testURL5 = URL(string: "https://brave.com/")
    if let actual = testURL5?.schemelessAbsoluteDisplayString {
      XCTAssertEqual(actual, "brave.com")
    } else {
      XCTFail("Actual url is nil")
    }

    // Test removes HTTPS scheme but not trailing slash because path is not empty
    let testURL6 = URL(string: "https://brave.com/foo/")
    if let actual = testURL6?.schemelessAbsoluteDisplayString {
      XCTAssertEqual(actual, "brave.com/foo/")
    } else {
      XCTFail("Actual url is nil")
    }

    // Test removes HTTPS scheme and www
    let testURL7 = URL(string: "https://www.brave.com")
    if let actual = testURL7?.schemelessAbsoluteDisplayString {
      XCTAssertEqual(actual, "brave.com")
    } else {
      XCTFail("Actual url is nil")
    }
  }

  func testKeepsAboutSchemeInURL() {
    let url = URL(string: "about:home")
    if let actual = url?.absoluteDisplayString {
      XCTAssertEqual(actual, "about:home")
    } else {
      XCTFail("Actual url is nil")
    }
  }

  //MARK: Public Suffix
  func testNormalBaseDomainWithSingleSubdomain() {
    // TLD Entry: co.uk
    let url = "http://a.bbc.co.uk".asURL!
    let expected = url.publicSuffix!
    XCTAssertEqual("co.uk", expected)
    XCTAssertEqual("co.uk", url.host?.publicSuffix!)
  }

  func testBaseDomainWithTrailingDot() {
    var url = URL(string: "https://test.domain.com")
    XCTAssertEqual(url?.baseDomain, "domain.com")
    XCTAssertEqual(url?.host?.baseDomain, "domain.com")

    url = URL(string: "https://test.domain.com.")
    XCTAssertEqual(url?.baseDomain, "domain.com.")
    XCTAssertEqual(url?.host?.baseDomain, "domain.com.")

    url = URL(string: "https://test.domain.com..")
    XCTAssertEqual(url?.baseDomain, nil)
    XCTAssertEqual(url?.host?.baseDomain, nil)

    url = URL(string: "https://foo")
    XCTAssertEqual(url?.baseDomain, "foo")
    XCTAssertEqual(url?.host?.baseDomain, "foo")

    url = URL(string: "https://foo.")
    XCTAssertEqual(url?.host?.baseDomain, nil)

    url = URL(string: "https://.")
    XCTAssertEqual(url?.baseDomain, nil)
    XCTAssertEqual(url?.host?.baseDomain, nil)
  }

  func testCanadaComputers() {
    let url = "http://m.canadacomputers.com".asURL!
    let actual = url.baseDomain
    XCTAssertEqual("canadacomputers.com", actual)
    XCTAssertEqual("canadacomputers.com", url.host?.baseDomain)
  }

  func testMultipleSuffixesInsideURL() {
    let url = "http://com:org@m.canadacomputers.co.uk".asURL!
    let actual = url.baseDomain
    XCTAssertEqual("canadacomputers.co.uk", actual)
    XCTAssertEqual("canadacomputers.co.uk", url.host?.baseDomain)
  }

  func testNormalBaseDomainWithManySubdomains() {
    // TLD Entry: co.uk
    let url = "http://a.b.c.d.bbc.co.uk".asURL!
    let expected = url.publicSuffix
    XCTAssertEqual("co.uk", expected)
    XCTAssertEqual("co.uk", url.host?.publicSuffix)
  }

  func testWildCardDomainWithSingleSubdomain() {
    // TLD Entry: *.kawasaki.jp
    let url = "http://a.kawasaki.jp".asURL!
    let expected = url.publicSuffix
    XCTAssertEqual(expected, nil)
    XCTAssertEqual(url.host?.publicSuffix, nil)
  }

  func testWildCardDomainWithManySubdomains() {
    // TLD Entry: *.kawasaki.jp
    let url = "http://a.b.c.d.kawasaki.jp".asURL!
    let expected = url.publicSuffix
    XCTAssertEqual("d.kawasaki.jp", expected)
    XCTAssertEqual("d.kawasaki.jp", url.host?.publicSuffix)
  }

  func testExceptionDomain() {
    // TLD Entry: !city.kawasaki.jp
    let url = "http://city.kawasaki.jp".asURL!
    let expected = url.publicSuffix!
    XCTAssertEqual("kawasaki.jp", expected)
    XCTAssertEqual("kawasaki.jp", url.host?.publicSuffix)
  }

  //MARK: Base Domain
  func testNormalBaseSubdomain() {
    // TLD Entry: co.uk
    let url = "http://bbc.co.uk".asURL!
    let expected = url.baseDomain!
    XCTAssertEqual("bbc.co.uk", expected)
    XCTAssertEqual("bbc.co.uk", url.host?.baseDomain)
  }

  func testNormalBaseSubdomainWithAdditionalSubdomain() {
    // TLD Entry: co.uk
    let url = "http://a.bbc.co.uk".asURL!
    let expected = url.baseDomain!
    XCTAssertEqual("bbc.co.uk", expected)
    XCTAssertEqual("bbc.co.uk", url.host?.baseDomain)
  }

  func testBaseDomainForWildcardDomain() {
    // TLD Entry: *.kawasaki.jp
    let url = "http://a.b.kawasaki.jp".asURL!
    let expected = url.baseDomain!
    XCTAssertEqual("a.b.kawasaki.jp", expected)
    XCTAssertEqual("a.b.kawasaki.jp", url.host?.baseDomain)
  }

  func testBaseDomainForWildcardDomainWithAdditionalSubdomain() {
    // TLD Entry: *.kawasaki.jp
    let url = "http://a.b.c.kawasaki.jp".asURL!
    let expected = url.baseDomain!
    XCTAssertEqual("b.c.kawasaki.jp", expected)
    XCTAssertEqual("b.c.kawasaki.jp", url.host?.baseDomain)
  }

  func testBaseDomainForExceptionDomain() {
    // TLD Entry: !city.kawasaki.jp
    let url = "http://city.kawasaki.jp".asURL!
    let expected = url.baseDomain!
    XCTAssertEqual("city.kawasaki.jp", expected)
    XCTAssertEqual("city.kawasaki.jp", url.host?.baseDomain)
  }

  func testBaseDomainForExceptionDomainWithAdditionalSubdomain() {
    // TLD Entry: !city.kawasaki.jp
    let url = "http://a.city.kawasaki.jp".asURL!
    let expected = url.baseDomain!
    XCTAssertEqual("city.kawasaki.jp", expected)
    XCTAssertEqual("city.kawasaki.jp", url.host?.baseDomain)
  }

  func testBugzillaURLDomain() {
    let url =
      "https://bugzilla.mozilla.org/enter_bug.cgi?format=guided#h=dupes|Data%20%26%20BI%20Services%20Team|"
    let nsURL = url.asURL
    XCTAssertNotNil(nsURL, "URL parses.")

    let host = nsURL!.normalizedHost()
    XCTAssertEqual(host!, "bugzilla.mozilla.org")
    XCTAssertEqual(nsURL!.fragment!, "h=dupes%7CData%2520%2526%2520BI%2520Services%2520Team%7C")
  }

  func testIPv6Domain() {
    let url = "http://[::1]/foo/bar".asURL!
    XCTAssertTrue(url.isIPv6)
    XCTAssertNil(url.baseDomain)
    XCTAssertNil("[::1]".baseDomain)
    XCTAssertEqual(url.normalizedHost()!, "[::1]")
  }

  func testisAboutHomeURL() {
    let goodurls = [
      "\(InternalURL.baseUrl)/about/home/#panel=0"
    ]
    let badurls = [
      "http://google.com",
      "http://localhost:6571/sessionrestore.html",
    ]

    goodurls.forEach {
      if let url = URL(string: $0) {
        XCTAssertTrue(InternalURL(url)?.isAboutHomeURL == true, $0)
      } else {
        XCTAssert(false, "Invalid URL: \($0)")
      }
    }
    badurls.forEach {
      if let url = URL(string: $0) {
        XCTAssertFalse(InternalURL(url)?.isAboutHomeURL == true, $0)
      } else {
        XCTAssert(false, "Invalid URL: \($0)")
      }
    }
  }

  func testisAboutURL() {
    let goodurls = [
      "\(InternalURL.baseUrl)/about/home/#panel=0",
      "\(InternalURL.baseUrl)/about/firefox",
    ]
    let badurls = [
      "http://google.com",
      "http://localhost:6571/sessionrestore.html",
    ]

    goodurls.forEach {
      if let url = URL(string: $0) {
        XCTAssertTrue(InternalURL(url)?.isAboutURL == true, $0)
      } else {
        XCTAssert(false, "Invalid URL: \($0)")
      }
    }

    badurls.forEach {
      if let url = URL(string: $0) {
        XCTAssertFalse(InternalURL(url)?.isAboutURL == true, $0)
      } else {
        XCTAssert(false, "Invalid URL: \($0)")
      }
    }
  }

  func testhavingRemovedAuthorisationComponents() {
    let goodurls = [
      (
        "https://Aladdin:OpenSesame@www.example.com/index.html",
        "https://www.example.com/index.html"
      ),
      ("https://www.example.com/noauth", "https://www.example.com/noauth"),
    ]

    goodurls.forEach {
      XCTAssertEqual(URL(string: $0.0)!.havingRemovedAuthorisationComponents().absoluteString, $0.1)
    }
  }

  func testschemeIsValid() {
    let goodurls = [
      "http://\(InternalURL.Path.readermode.rawValue)/page",
      "https://google.com",
      "tel:6044044004",
    ]
    let badurls = [
      "blah://google.com",
      "hax://localhost:6571/sessionrestore.html",
      "leet://codes.com",
      "data:text/html;base64,SGVsbG8gV29ybGQhCg==",
    ]

    goodurls.forEach { XCTAssertTrue(URL(string: $0)!.schemeIsValid, $0) }
    badurls.forEach { XCTAssertFalse(URL(string: $0)!.schemeIsValid, $0) }
  }

  func testIsLocalUtility() {
    let goodurls = [
      "http://localhost:6571/reader-mode/page",
      "http://LOCALhost:6571/\(InternalURL.Path.sessionrestore)/sessionrestore.html",
    ]
    let badurls = [
      "http://google.com",
      "tel:6044044004",
      "hax://localhost:6571/testhomepage",
      "http://127.0.0.1:6571/test/atesthomepage.html",
    ]

    goodurls.forEach { XCTAssertTrue(URL(string: $0)!.isLocalUtility, $0) }
    badurls.forEach { XCTAssertFalse(URL(string: $0)!.isLocalUtility, $0) }
  }

  func testisLocal() {
    let goodurls = [
      "http://localhost:6571/reader-mode/page",
      "http://LOCALhost:6571/sessionrestore.html",
      "http://127.0.0.1:6571/sessionrestore.html",
      "http://:6571/sessionrestore.html",

    ]
    let badurls = [
      "http://google.com",
      "tel:6044044004",
      "hax://localhost:6571/about",
    ]

    goodurls.forEach { XCTAssertTrue(URL(string: $0)!.isLocal, $0) }
    badurls.forEach { XCTAssertFalse(URL(string: $0)!.isLocal, $0) }
  }

  func testisWebPage() {
    let goodurls = [
      "http://localhost:6571/reader-mode/page",
      "https://127.0.0.1:6571/sessionrestore.html",
      "data://:6571/sessionrestore.html",

    ]
    let badurls = [
      "about://google.com",
      "tel:6044044004",
      "hax://localhost:6571/about",
    ]

    goodurls.forEach { XCTAssertTrue(URL(string: $0)!.isWebPage(), $0) }
    badurls.forEach { XCTAssertFalse(URL(string: $0)!.isWebPage(), $0) }
  }

  func testdomainWithoutWWW() {
    let urls = [
      ("https://www.example.com/index.html", "https://example.com/index.html"),
      ("https://mail.example.com/index.html", "https://mail.example.com/index.html"),
      ("https://mail.example.co.uk/index.html", "https://mail.example.co.uk/index.html"),
    ]
    urls.forEach { XCTAssertEqual(URL(string: $0.0)!.withoutWWW.absoluteString, $0.1) }
  }

  func testdomainWithoutFragment() {
    let urls = [
      ("https://www.example.com/index.html#Fragment", "https://www.example.com/index.html"),
      (
        "https://mail.example.com/index.html?Key=Value#Fragment",
        "https://mail.example.com/index.html?Key=Value"
      ),
      (
        "https://mail.example.co.uk/index.html?Key=Value&Key2=Value2#Fragment",
        "https://mail.example.co.uk/index.html?Key=Value&Key2=Value2"
      ),
    ]
    urls.forEach { XCTAssertEqual(URL(string: $0.0)!.withoutFragment.absoluteString, $0.1) }
  }

  func testdomainUrl() {
    let urls = [
      ("https://www.example.com/index.html", "https://example.com"),
      ("https://mail.example.com/index.html", "https://mail.example.com"),
      ("https://mail.example.co.uk/index.html", "https://mail.example.co.uk"),
    ]
    urls.forEach { XCTAssertEqual(URL(string: $0.0)!.domainURL.absoluteString, $0.1) }
  }

  func testdisplayURL() {
    let goodurls = [
      (
        "\(InternalURL.baseUrl)/\(InternalURL.Path.readermode.rawValue)?url=https%3A%2F%2Fen%2Em%2Ewikipedia%2Eorg%2Fwiki%2F",
        "https://en.m.wikipedia.org/wiki/"
      ),
      ("https://mail.example.co.uk/index.html", "https://mail.example.co.uk/index.html"),
      (
        "\(InternalURL.baseUrl)/\(InternalURL.Path.readermode.rawValue)?url=http%3A//mozilla.com",
        "http://mozilla.com"
      ),
    ]
    let badurls = [
      "\(InternalURL.baseUrl)/\(InternalURL.Path.readermode.rawValue)/page?url=https%3A%2F%2Fen%2Em%2Ewikipedia%2Eorg%2Fwiki%2F"
    ]

    goodurls.forEach { XCTAssertEqual(URL(string: $0.0)!.displayURL?.absoluteString, $0.1) }
    badurls.forEach { XCTAssertNil(URL(string: $0)!.displayURL) }
  }

  func testnormalizedHostAndPath() {
    let goodurls = [
      ("https://www.example.com/index.html", "example.com/index.html"),
      ("https://mail.example.com/index.html", "mail.example.com/index.html"),
      ("https://mail.example.co.uk/index.html", "mail.example.co.uk/index.html"),
      ("https://m.example.co.uk/index.html", "example.co.uk/index.html"),
    ]
    let badurls = [
      "http:///errors/error.html",
      "http://:6571/about/home",
    ]

    goodurls.forEach { XCTAssertEqual(URL(string: $0.0)!.normalizedHostAndPath, $0.1) }
    badurls.forEach { XCTAssertNil(URL(string: $0)!.normalizedHostAndPath) }
  }

  func testhostSLD() {
    let urls = [
      ("https://www.example.com/index.html", "example"),
      ("https://m.foo.com/bar/baz?noo=abc#123", "foo"),
      ("https://user:pass@m.foo.com/bar/baz?noo=abc#123", "foo"),
    ]
    urls.forEach { XCTAssertEqual(URL(string: $0.0)!.hostSLD, $0.1) }
  }

  func testgetQuery() {
    let url = URL(string: "http://example.com/path?a=1&b=2&c=3")!
    let params = ["a": "1", "b": "2", "c": "3"]

    let urlParams = url.getQuery()
    params.forEach {
      XCTAssertEqual(urlParams[$0], $1, "The values in params should be the same in urlParams")
    }
  }

  func testWithQueryParam() {
    let urlA = URL(string: "http://foo.com/bar/")!
    let urlB = URL(string: "http://bar.com/noo")!
    let urlC = urlA.withQueryParam("ppp", value: "123")
    let urlD = urlB.withQueryParam("qqq", value: "123")
    let urlE = urlC.withQueryParam("rrr", value: "aaa")

    XCTAssertEqual("http://foo.com/bar/?ppp=123", urlC.absoluteString)
    XCTAssertEqual("http://bar.com/noo?qqq=123", urlD.absoluteString)
    XCTAssertEqual("http://foo.com/bar/?ppp=123&rrr=aaa", urlE.absoluteString)
  }

  func testAppendPathComponentsHelper() {
    var urlA = URL(string: "http://foo.com/bar/")!
    var urlB = URL(string: "http://bar.com/noo")!

    urlA.append(pathComponents: "foo")
    urlA.append(pathComponents: "one", "two")

    urlB.append(pathComponents: "", "", "test", "", "one", "")

    XCTAssertEqual("http://foo.com/bar/foo/one/two", urlA.absoluteString)
    XCTAssertEqual("http://bar.com/noo/test/one/", urlB.absoluteString)

  }

  func testHidingFromDataDetectors() {
    guard let detector = try? NSDataDetector(types: NSTextCheckingResult.CheckingType.link.rawValue)
    else {
      XCTFail()
      return
    }

    let urls = ["https://example.com", "example.com", "http://example.com"]
    for u in urls {
      let url = URL(string: u)!

      let original = url.absoluteDisplayString.replacingOccurrences(of: ".", with: "\u{2024}")
      let matches = detector.matches(
        in: original,
        options: [],
        range: NSMakeRange(0, original.count)
      )
      guard matches.count > 0 else {
        print("\(url) doesn't match as a URL")
        continue
      }

      let modified = url.absoluteDisplayString.replacingOccurrences(of: ".", with: "\u{2024}")
      XCTAssertNotEqual(original, modified)

      let newMatches = detector.matches(
        in: modified,
        options: [],
        range: NSMakeRange(0, modified.count)
      )

      XCTAssertEqual(0, newMatches.count, "\(modified) is not a valid URL")
    }
  }

  func testIsBookmarkletURL() {
    let javascriptURLs = [
      "javascript:",
      "javascript://",
      "javascript:void(window.close(self))",
      "javascript:window.open('https://brave.com')",
    ]

    let nonJavascriptURLs = [
      "https://brave",
      "https://brave.com",
      "https://javascript://",
      "javascript//",
    ]

    javascriptURLs.forEach {
      XCTAssertNotNil(URL.bookmarkletURL(from: $0))
    }

    nonJavascriptURLs.forEach {
      XCTAssertNil(URL.bookmarkletURL(from: $0))
    }
  }

  func testIsBookmarkletURLComponent() {
    let javascriptURLs = [
      "javascript:/",
      "javascript://",
      "javascript://something",
      "javascript:void(window.close(self))",
      "javascript:window.open('https://brave.com')",
      "javascript://%0a%0dalert('UXSS')",
    ]

    let badURLs = [
      "javascript:",
      "https://test",
      "javascript//test",
      "javascript/test",
    ]

    javascriptURLs.forEach {
      XCTAssertNotNil(URL.bookmarkletURL(from: $0)?.bookmarkletCodeComponent)
    }

    badURLs.forEach {
      XCTAssertNil(URL.bookmarkletURL(from: $0)?.bookmarkletCodeComponent)
    }
  }

  func testDisplayString() {
    func checkDisplayURLString(testURL: URL?, displayString: String) {
      if let actual = testURL?.displayURL?.absoluteString {
        XCTAssertEqual(actual, displayString)
      } else {
        XCTFail("Actual url is nil")
      }
    }

    let urlMap = [
      URL(string: "https://www.youtube.com"): "https://www.youtube.com",
      URL(string: "http://google.com"): "http://google.com",
      URL(string: "www.brave.com"): "www.brave.com",
      URL(string: "http://brave.com/foo/"): "http://brave.com/foo/",
      URL(string: "http://brave.com/foo"): "http://brave.com/foo",
      URL(string: "blob://http://brave.com/foo"): "blob://http//brave.com/foo",
      URL(string: "blob://02C00302-CE62-4DAE-AD70-FDEE19594856"):
        "blob://02C00302-CE62-4DAE-AD70-FDEE19594856",
      URL(string: "file:///Users/brave/documents/foo.txt"): "file://foo.txt",
      URL(string: "file://http://brave.com/foo.txt"): "file://foo.txt",
      URL(
        string:
          "\(InternalURL.baseUrl)/\(InternalURL.Path.sessionrestore.rawValue)?url=http%3A%2F%2Fbrave.com%2Ffoo"
      ): "http://brave.com/foo",
      URL(
        string:
          "\(InternalURL.baseUrl)/\(InternalURL.Path.sessionrestore.rawValue)?url=blob:https://brave.com/66823669-a00d-4c54-b1d6-f86df100b876"
      ): "blob:https://brave.com/66823669-a00d-4c54-b1d6-f86df100b876",
    ]

    urlMap.forEach {
      checkDisplayURLString(testURL: $0, displayString: $1)
    }
  }
}
