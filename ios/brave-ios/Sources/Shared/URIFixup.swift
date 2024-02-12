/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

public class URIFixup {
  private static func isValidIPAddress(_ host: String) -> Bool {
    var buffer = [UInt8](repeating: 0, count: Int(INET6_ADDRSTRLEN))
    if inet_pton(AF_INET, host, &buffer) != 0 || inet_pton(AF_INET6, host, &buffer) != 0 {
      return true
    }
    return false
  }

  private static func isValidIPAddressURL(_ string: String) -> Bool {
    // IPv4 addresses MUST have a `.` character delimiting the octets.
    // RFC-2732 states an IPv6 URL should contain brackets as in: `[IP_ADDRESS_HERE]`
    if !(string.contains(".") || (string.contains("[") && string.contains("]"))) {
      return false
    }

    // Validate if the HOST is a valid IP address.
    if let url = URL(string: "https://\(string)"),
      let host = url.host, !host.isEmpty {
      return isValidIPAddress(host)
    } else {
      return false
    }
  }

  private static func validateURL(_ url: URL) -> URL? {
    // Validate the domain to make sure it doesn't have any invalid characters
    // IE: quotes, etc..
    if let host = url.host {
      guard let decodedASCIIURL = host.removingPercentEncoding else {
        return nil
      }

      if decodedASCIIURL.rangeOfCharacter(from: CharacterSet.URLAllowed.inverted) != nil {
        return nil
      }

      // `http://::192.9.5.5` will produce an invalid URL
      // Its host, path, query, fragment, etc.. will all be empty
      // This prevents bad URLs from being passed to the DNS resolver.
      // Instead, the bad URL is forwarded to the search-engine (same behaviour as Desktop).
      if URLComponents(url: url, resolvingAgainstBaseURL: false) == nil {
        return nil
      }

      // The host is a valid IPv4 or IPv6 address
      if isValidIPAddress(host) {
        return url
      }
    }

    return url
  }

  public static func getURL(_ entry: String) -> URL? {
    let trimmed = entry.trimmingCharacters(in: .whitespacesAndNewlines)
    
    // NSURL: idnString from brave core handles the puny code represantation of Hostnames
    // Using Punycode, host names containing Unicode characters are transcoded to a subset of ASCII
    let entryURL = NSURL(idnString: trimmed) as URL?
    if let url = entryURL, InternalURL.isValid(url: url) {
      return url
    }

    guard let escaped = trimmed.addingPercentEncoding(withAllowedCharacters: .URLAllowed) else {
      return nil
    }

    // Then check if the URL includes a scheme. This will handle
    // all valid requests starting with "http://", "about:", etc.
    // However, we ensure that the scheme is one that is listed in
    // the official URI scheme list, so that other such search phrases
    // like "filetype:" are recognised as searches rather than URLs.
    // Use `URL(string: entry)` so it doesn't double percent escape URLs.
    if let url = entryURL, url.schemeIsValid {
      return validateURL(url)
    }

    // If there's no scheme, we're going to prepend "http://". First,
    // make sure there's at least one "." or ":" in the host. This means
    // we'll allow single-word searches (e.g., "foo") at the expense
    // of breaking single-word hosts without a scheme (e.g., "localhost").
    if trimmed.range(of: ".") == nil && trimmed.range(of: ":") == nil {
      return nil
    }

    if trimmed.range(of: ":") != nil {
      // The host is a valid IPv4 or IPv6 address
      if isValidIPAddressURL(trimmed) {
        // IP Addresses do NOT require a Scheme.
        // However, Brave requires that URLs have a scheme.
        return NSURL(idnString: "http://\(escaped)") as URL?
      } else {
        // If host is NOT an IP-Address, it should never contain a colon
        // This is because it also doesn't contain a "." so it isn't a domain at all.
        // IE: foo:5000 & brave:8080 are not valid addresses.
        return nil
      }
    }

    // Partially canonicalize the URL and check if it has a "user"..
    // If it is, it should go to the search engine and not the DNS server..
    // This behaviour is mimicking SAFARI! It has the safest behaviour so far.
    //
    // 1. If the url contains just "user@domain.com", ALL browsers take you to the search engine.
    // 2. If it's an email with a PATH or QUERY such as "user@domain.com/whatever"
    //    where "/whatever" is the path or "user@domain.com?something=whatever"
    //    where "?something=whatever" is the query:
    //    - Firefox warns you that a site is trying to log you in automatically to the domain.
    //    - Chrome takes you to the domain (seems like a security flaw).
    //    - Safari passes on the entire url to the Search Engine just like it does
    //      without a path or query.
    if URL(string: trimmed)?.user != nil || URL(string: escaped)?.user != nil ||
        URL(string: "http://\(trimmed)")?.user != nil || URL(string: "http://\(escaped)")?.user != nil {
      return nil
    }

    // URL contains more than 1 dot, but is NOT a valid IP.
    // IE: brave.com.com.com.com or 123.4.5 or "hello.world.whatever"
    // However, a valid URL can be "brave.com" or "hello.world"
    if let url = URL(string: escaped),
      url.scheme == nil {
      let dotCount = escaped.reduce(0, { $1 == "." ? $0 + 1 : $0 })
      if dotCount > 0 && !isValidIPAddress(escaped) {
        // If there is a "." or ":", prepend "http://" and try again. Since this
        // is strictly an "http://" URL, we also require a host.
        if let url = NSURL(idnString: "http://\(escaped)") as URL?, let host = url.host,
          host.rangeOfCharacter(from: CharacterSet(charactersIn: "1234567890.[]:").inverted) != nil {
          return validateURL(url)
        }
        return nil
      }
    }

    // If there is a "." or ":", prepend "http://" and try again. Since this
    // is strictly an "http://" URL, we also require a host.
    if let url = NSURL(idnString: "http://\(escaped)") as URL?, url.host != nil {
      return validateURL(url)
    }

    return nil
  }
}
