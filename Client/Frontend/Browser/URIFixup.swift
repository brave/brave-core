/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

class URIFixup {

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
        }
        
        return url
    }
    
    static func getURL(_ entry: String) -> URL? {
        let trimmed = entry.trimmingCharacters(in: .whitespacesAndNewlines)
        guard let escaped = trimmed.addingPercentEncoding(withAllowedCharacters: .URLAllowed) else {
            return nil
        }

        // Then check if the URL includes a scheme. This will handle
        // all valid requests starting with "http://", "about:", etc.
        // However, we ensure that the scheme is one that is listed in
        // the official URI scheme list, so that other such search phrases
        // like "filetype:" are recognised as searches rather than URLs.
        if let url = URL(string: escaped), url.schemeIsValid {
            return validateURL(url)
        }

        // If there's no scheme, we're going to prepend "http://". First,
        // make sure there's at least one "." in the host. This means
        // we'll allow single-word searches (e.g., "foo") at the expense
        // of breaking single-word hosts without a scheme (e.g., "localhost").
        if trimmed.range(of: ".") == nil {
            return nil
        }

        if trimmed.range(of: " ") != nil {
            return nil
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
        if URL(string: trimmed)?.user != nil ||
            URL(string: escaped)?.user != nil ||
            URL(string: "http://\(trimmed)")?.user != nil ||
            URL(string: "http://\(escaped)")?.user != nil {
            return nil
        }

        // If there is a ".", prepend "http://" and try again. Since this
        // is strictly an "http://" URL, we also require a host.
        if let url = URL(string: "http://\(escaped)"), url.host != nil {
            return validateURL(url)
        }

        return nil
    }
}
