/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Fuzi

private let TypeSearch = "text/html"
private let TypeSuggest = "application/x-suggestions+json"

class OpenSearchEngine: NSObject, NSSecureCoding {
  static let preferredIconSize = 30

  struct EngineNames {
    static let duckDuckGo = "DuckDuckGo"
    static let qwant = "Qwant"
    static let brave = "Brave Search beta"
    static let yahoo = "Yahoo"
    static let yahooJP = "Yahoo! JAPAN"
  }

  static let defaultSearchClientName = "brave"

  let shortName: String
  let referenceURL: String?

  // Backwards compatibility workaround, see #3056.
  // We use `shortName` to store persist what engines are set as default, order etc.
  // This means there's no easy way to change display text for the search engine without
  // saved engines breaking.
  // This updates the engines name in the UI, without changing it at the xml level.
  // In the future we might refactor it.
  var displayName: String {
    switch shortName.lowercased() {
    case "startpage":
      return "Startpage"
    case "Яндекс".lowercased():
      return "Yandex"
    case EngineNames.brave.lowercased():
      return "Brave Search"
    case "naver".lowercased():
      return "네이버"
    default:
      return shortName
    }
  }
  let engineID: String?
  let image: UIImage
  let isCustomEngine: Bool
  let searchTemplate: String
  let suggestTemplate: String?

  fileprivate let SearchTermComponent = "{searchTerms}"
  fileprivate let LocaleTermComponent = "{moz:locale}"
  fileprivate let RegionalClientComponent = "{customClient}"

  fileprivate lazy var searchQueryComponentKey: String? = self.getQueryArgFromTemplate()

  init(
    engineID: String? = nil, shortName: String, referenceURL: String? = nil, image: UIImage, searchTemplate: String,
    suggestTemplate: String? = nil, isCustomEngine: Bool
  ) {
    self.shortName = shortName
    self.referenceURL = referenceURL
    self.image = image
    self.searchTemplate = searchTemplate
    self.suggestTemplate = suggestTemplate
    self.isCustomEngine = isCustomEngine
    self.engineID = engineID
  }

  required init?(coder aDecoder: NSCoder) {
    // this catches the cases where bool encoded in Swift 2 needs to be decoded with decodeObject, but a Bool encoded in swift 3 needs
    // to be decoded using decodeBool. This catches the upgrade case to ensure that we are always able to fetch a keyed valye for isCustomEngine
    // http://stackoverflow.com/a/40034694
    let isCustomEngine = aDecoder.decodeBool(forKey: "isCustomEngine")
    guard let searchTemplate = aDecoder.decodeObject(of: NSString.self, forKey: "searchTemplate") as String?,
      let shortName = aDecoder.decodeObject(of: NSString.self, forKey: "shortName") as String?,
      let image = aDecoder.decodeObject(of: UIImage.self, forKey: "image")
    else {
      assertionFailure()
      return nil
    }

    self.searchTemplate = searchTemplate
    self.shortName = shortName
    self.referenceURL = aDecoder.decodeObject(of: NSString.self, forKey: "href") as String?
    self.isCustomEngine = isCustomEngine
    self.image = image
    self.engineID = aDecoder.decodeObject(of: NSString.self, forKey: "engineID") as String?
    self.suggestTemplate = aDecoder.decodeObject(of: NSString.self, forKey: "suggestTemplate") as String?
  }

  func encode(with aCoder: NSCoder) {
    aCoder.encode(searchTemplate, forKey: "searchTemplate")
    aCoder.encode(suggestTemplate, forKey: "suggestTemplate")
    aCoder.encode(shortName, forKey: "shortName")
    aCoder.encode(isCustomEngine, forKey: "isCustomEngine")
    aCoder.encode(image, forKey: "image")
    aCoder.encode(engineID, forKey: "engineID")
    aCoder.encode(referenceURL, forKey: "href")
  }

  static var supportsSecureCoding: Bool {
    return true
  }

  /**
     * Returns the search URL for the given query.
     */
  func searchURLForQuery(_ query: String, locale: Locale = Locale.current, isBraveSearchPromotion: Bool = false) -> URL? {
    return getURLFromTemplate(
      searchTemplate,
      query: query,
      locale: locale,
      isBraveSearchPromotion: isBraveSearchPromotion)
  }

  /**
     * Return the arg that we use for searching for this engine
     * Problem: the search terms may not be a query arg, they may be part of the URL - how to deal with this?
     **/
  fileprivate func getQueryArgFromTemplate() -> String? {
    // we have the replace the templates SearchTermComponent in order to make the template
    // a valid URL, otherwise we cannot do the conversion to NSURLComponents
    // and have to do flaky pattern matching instead.
    let placeholder = "PLACEHOLDER"
    let template =
      searchTemplate
      .replacingOccurrences(of: SearchTermComponent, with: placeholder)
      .replacingOccurrences(of: RegionalClientComponent, with: placeholder)
    let components = URLComponents(string: template)
    let searchTerm = components?.queryItems?.filter { item in
      return item.value == placeholder
    }
    guard let term = searchTerm, !term.isEmpty else { return nil }
    return term[0].name
  }

  /**
     * check that the URL host contains the name of the search engine somewhere inside it
     **/
  fileprivate func isSearchURLForEngine(_ url: URL?) -> Bool {
    guard let urlHost = url?.hostSLD,
      let queryEndIndex = searchTemplate.range(of: "?")?.lowerBound,
      let templateURL = URL(string: String(searchTemplate[..<queryEndIndex]))
    else { return false }
    return urlHost == templateURL.hostSLD
  }

  /**
     * Returns the query that was used to construct a given search URL
     **/
  func queryForSearchURL(_ url: URL?) -> String? {
    if isSearchURLForEngine(url) {
      if let key = searchQueryComponentKey,
        let value = url?.getQuery()[key] {
        return value.replacingOccurrences(of: "+", with: " ").removingPercentEncoding
      }
    }
    return nil
  }

  /**
     * Returns the search suggestion URL for the given query.
     */
  func suggestURLForQuery(_ query: String, locale: Locale = Locale.current) -> URL? {
    if let suggestTemplate = suggestTemplate {
      return getURLFromTemplate(suggestTemplate, query: query, locale: locale)
    }
    return nil
  }

  fileprivate func getURLFromTemplate(_ searchTemplate: String, query: String, locale: Locale, isBraveSearchPromotion: Bool = false) -> URL? {
    guard let escapedQuery = query.addingPercentEncoding(withAllowedCharacters: .searchTermsAllowed) else {
      return nil
    }

    // Escape the search template as well in case it contains not-safe characters like symbols
    let templateAllowedSet = NSMutableCharacterSet()
    templateAllowedSet.formUnion(with: .URLAllowed)

    // Allow brackets since we use them in our template as our insertion point
    templateAllowedSet.formUnion(with: CharacterSet(charactersIn: "{}"))

    guard
      let encodedSearchTemplate = searchTemplate.addingPercentEncoding(
        withAllowedCharacters:
          templateAllowedSet as CharacterSet)
    else { return nil }

    let localeString = locale.identifier
    let urlString =
      encodedSearchTemplate
      .replacingOccurrences(of: SearchTermComponent, with: escapedQuery, options: .literal, range: nil)
      .replacingOccurrences(of: LocaleTermComponent, with: localeString, options: .literal, range: nil)
      .replacingOccurrences(
        of: RegionalClientComponent, with: regionalClientParam(locale),
        options: .literal, range: nil)

    var searchUrl = URL(string: urlString)
    
    if isBraveSearchPromotion {
      searchUrl = searchUrl?.withQueryParam("action", value: "makeDefault")
    }
    
    return searchUrl
  }

  private func regionalClientParam(_ locale: Locale) -> String {
    if shortName == EngineNames.duckDuckGo, let region = locale.regionCode {
      switch region {
      case "AU", "IE", "NZ": return "braveed"
      case "DE": return "bravened"
      default: break
      }
    }

    return OpenSearchEngine.defaultSearchClientName
  }
}

/**
 * OpenSearch XML parser.
 *
 * This parser accepts standards-compliant OpenSearch 1.1 XML documents in addition to
 * the Firefox-specific search plugin format.
 *
 * OpenSearch spec: http://www.opensearch.org/Specifications/OpenSearch/1.1
 */
class OpenSearchParser {
  fileprivate let pluginMode: Bool

  init(pluginMode: Bool) {
    self.pluginMode = pluginMode
  }

  func parse(_ file: String, engineID: String, referenceURL: String?) -> OpenSearchEngine? {
    guard let data = try? Data(contentsOf: URL(fileURLWithPath: file)) else {
      print("Invalid search file")
      return nil
    }

    return parse(data, engineID: engineID, referenceURL: referenceURL)
  }

  func parse(_ data: Data, engineID: String = "", referenceURL: String? = nil, image: UIImage? = nil, isCustomEngine: Bool = false) -> OpenSearchEngine? {
    guard let indexer = try? XMLDocument(data: data),
      let docIndexer = indexer.root
    else {
      print("Invalid XML document")
      return nil
    }

    let shortNameIndexer = docIndexer.children(tag: "ShortName")
    if shortNameIndexer.count != 1 {
      print("ShortName must appear exactly once")
      return nil
    }

    let shortName = shortNameIndexer[0].stringValue
    if shortName == "" {
      print("ShortName must contain text")
      return nil
    }

    let urlIndexers = docIndexer.children(tag: "Url")
    if urlIndexers.isEmpty {
      print("Url must appear at least once")
      return nil
    }

    var searchTemplate: String!
    var suggestTemplate: String?
    for urlIndexer in urlIndexers {
      let type = urlIndexer.attributes["type"]
      if type == nil {
        print("Url element requires a type attribute", terminator: "\n")
        return nil
      }

      if type != TypeSearch && type != TypeSuggest {
        // Not a supported search type.
        continue
      }

      var template = urlIndexer.attributes["template"]
      if template == nil {
        print("Url element requires a template attribute", terminator: "\n")
        return nil
      }

      if pluginMode {
        let paramIndexers = urlIndexer.children(tag: "Param")

        if !paramIndexers.isEmpty {
          template! += "?"
          var firstAdded = false
          for paramIndexer in paramIndexers {
            if firstAdded {
              template! += "&"
            } else {
              firstAdded = true
            }

            let name = paramIndexer.attributes["name"]
            let value = paramIndexer.attributes["value"]
            if name == nil || value == nil {
              print("Param element must have name and value attributes", terminator: "\n")
              return nil
            }
            template! += name! + "=" + value!
          }
        }
      }

      if type == TypeSearch {
        searchTemplate = template
      } else {
        suggestTemplate = template
      }
    }

    if searchTemplate == nil {
      print("Search engine must have a text/html type")
      return nil
    }

    let imageIndexers = docIndexer.children(tag: "Image")
    var largestImage = 0
    var largestImageElement: XMLElement?

    // TODO: For now, just use the largest icon.
    for imageIndexer in imageIndexers {
      let imageWidth = Int(imageIndexer.attributes["width"] ?? "")
      let imageHeight = Int(imageIndexer.attributes["height"] ?? "")

      // Only accept square images.
      if imageWidth != imageHeight {
        continue
      }

      if let imageWidth = imageWidth {
        if imageWidth > largestImage {
          largestImage = imageWidth
          largestImageElement = imageIndexer
        }
      }
    }

    let uiImage: UIImage

    if let image = image {
      uiImage = image
    } else if let imageElement = largestImageElement,
      let imageURL = URL(string: imageElement.stringValue),
      let imageData = try? Data(contentsOf: imageURL),
      let image = UIImage.imageFromDataThreadSafe(imageData) {
      uiImage = image
    } else {
      print("Error: Invalid search image data")
      return nil
    }

    return OpenSearchEngine(engineID: engineID, shortName: shortName, referenceURL: referenceURL, image: uiImage, searchTemplate: searchTemplate, suggestTemplate: suggestTemplate, isCustomEngine: isCustomEngine)
  }
}
