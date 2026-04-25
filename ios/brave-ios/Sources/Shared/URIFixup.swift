// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
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
    if let url = NSURL(idnString: "https://\(string)") {
      return url.isHostIPAddress
    }

    return false
  }

  private static func validateURL(_ url: URL) -> URL? {
    // Validate the domain to make sure it doesn't have any invalid characters
    // IE: quotes, etc..
    if let host = url.host {
      guard let decodedASCIIURL = host.removingPercentEncoding else {
        return nil
      }

      if decodedASCIIURL.rangeOfCharacter(from: CharacterSet.urlAllowed.inverted) != nil {
        return nil
      }

      // `http://::192.9.5.5` will produce an invalid URL
      // Its host, path, query, fragment, etc.. will all be empty
      // This prevents bad URLs from being passed to the DNS resolver.
      // Instead, the bad URL is forwarded to the search-engine (same behaviour as Desktop).
      if URLComponents(url: url, resolvingAgainstBaseURL: false) == nil {
        return nil
      }

      // The host is local host
      if host == "localhost" {
        return url
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
    let entryURL = NSURL(idnString: trimmed)

    // If the URL is internal, don't validate it
    if let url = entryURL, InternalURL.isValid(url: url as URL) {
      return url as URL
    }

    guard let escaped = trimmed.addingPercentEncoding(withAllowedCharacters: .urlAllowed) else {
      return nil
    }

    // Then check if the URL includes a scheme. This will handle
    // all valid requests starting with "http://", "about:", etc.
    // However, we ensure that the scheme is one that is listed in
    // the official URI scheme list, so that other such search phrases
    // like "filetype:" are recognised as searches rather than URLs.
    // Use `URL(string: entry)` so it doesn't double percent escape URLs.
    if let url = entryURL as URL?, url.schemeIsValid {
      guard let match = AutocompleteClassifier.classify(url.absoluteString) else {
        return nil
      }

      // brave.com -> https://brave.com
      // brave.com. -> https://brave.com.
      // 127.0.0.1 -> http://127.0.0.1:80
      // localhost -> https://localhost:80
      if match.type == .urlWhatYouTyped {
        // Always return the suggested URL and not the input the user entered
        // Do not return `entryURL` which is based on user input
        return match.destinationURL
      }

      // dev@brave.com -> search_engine/?q=...
      // Brandon -> search_engine/?q=Brandon
      // http://dev@brave.com -> search_engine/?q=...
      if match.type == .searchWhatYouTyped {
        // Do not return the destinationURL which can be a search engine URL
        // this is because this class can be used for validation, and not just URL-Bar/Omnibox.
        // So return nil, indicating that the input is not a valid URL
        return nil
      }

      return validateURL(url)
    }

    // See above comments
    guard let match = AutocompleteClassifier.classify(entry) else {
      return nil
    }

    if [.bookmarkTitle, .historyUrl, .openTab, .urlWhatYouTyped].contains(match.type) {
      return match.destinationURL
    }

    if match.type == .searchWhatYouTyped {
      // Technically we should be returning `destinationURL` here.
      // But if the user's default search engine is `Google`, then `destinationURL` will be `BraveSearch`!
      // That's because we don't use Chromium's `SearchEngine` shared in Brave-Core, and we use our own!
      // If we ever refactor to use the SearchEngine logic from Brave-Core, we can safely return `destinationURL` here,
      // and get rid of ALL of the above code entirely.
      return nil
    }

    return nil
  }
}
