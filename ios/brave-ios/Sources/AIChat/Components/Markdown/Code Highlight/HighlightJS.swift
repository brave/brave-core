// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import JavaScriptCore
import SwiftUI
import os.log
import Shared

class HighlightJS {
  private let context: JSContext
  private let theme: [BasicCSSParser.CSSStyle]
  private let script: String
  
  static let dark = HighlightJS(darkTheme: true)
  static let light = HighlightJS(darkTheme: false)
  
  private init(darkTheme: Bool) {
    let styleSheet = HighlightJS.loadScript(named: "atom-one-\(darkTheme ? "dark" : "light").min", type: "css") ?? ""
    theme = BasicCSSParser.parse(styleSheet)
    script = HighlightJS.loadScript(named: "highlight.min", type: "js") ?? ""
    
    context = JSContext()
    context.evaluateScript("var window = {};\n\n\(script)")
  }
  
  private static func loadScript(named: String, type: String) -> String? {
    guard let path = Bundle.module.path(forResource: named, ofType: type),
          let source = try? String(contentsOfFile: path) else {
      Logger.module.error("Failed to load script: \(named).js")
      assertionFailure("Failed to Load Script: \(named).js")
      return nil
    }
    return source
  }
  
  func highlight(_ code: String, preferredFont: UIFont, language: String? = nil) -> (backgroundColor: Color?, string: AttributedString)? {
    let result: JSValue?
    if let language = language {
      let highlight = context.objectForKeyedSubscript("hljs").objectForKeyedSubscript("highlight")
      result = highlight?.call(withArguments: [normalizeLanguage(language), code, false /* False = Do not highlight invalid syntax */])
    } else {
      let highlight = context.objectForKeyedSubscript("hljs").objectForKeyedSubscript("highlightAuto")
      result = highlight?.call(withArguments: [code])
    }
    
    if let result = result?.objectForKeyedSubscript("value"), !result.isUndefined, !result.isNull, let code = result.toString() {
      
      // If our custom parser can parse the string, then use it.
      // It's significantly faster than Apple's parser.
      if let string = HLJSParser.parse(code, styleSheet: theme, preferredFont: preferredFont) {
        return processAttributedString(string, preferredFont: preferredFont)
      }
      
      // Fallback to Apple's parser.
      let styledCode = "<style>\(theme)</style><pre><code class='hljs'>\(code)</code></pre>"

      if let data = styledCode.data(using: .utf8),
         let string = try? NSMutableAttributedString(data: data, 
                                                     options: [
                                                      .documentType: NSAttributedString.DocumentType.html,
                                                      .characterEncoding: String.Encoding.utf8.rawValue
                                                     ],
                                                     documentAttributes: nil) {
        return processAttributedString(string, preferredFont: preferredFont)
      }
    }
    
    return nil
  }
  
  private func availableLanguages() -> [String] {
    let hljs = context.objectForKeyedSubscript("hljs").objectForKeyedSubscript("listLanguages")
    let result = hljs?.call(withArguments: [])
    if let result = result, !result.isUndefined, !result.isNull, 
       let languages = result.toArray() as? [String] {
      return languages
    }
    return []
  }
  
  private func normalizeLanguage(_ language: String) -> String {
    // If we want more languages, we have to use the full HighlightJS package which is very tiny anyway
    // I'm only using a subset atm though:
    
    /*let supportedLanguages = [
      "bash", "c", "cpp", "csharp", "css", "diff", "go", "graphql",
      "ini", "java", "javascript", "json", "kotlin", "less", "lua",
      "makefile", "markdown", "objectivec", "perl", "php", "php-template",
      "plaintext", "python", "python-repl", "r", "ruby", "rust", "scss",
      "shell", "sql", "swift", "typescript", "vbnet", "wasm", "xml", "yaml"]*/
    
    if language == "c++" {
      return "cpp"
    }
    
    if language == "c#" {
      return "csharp"
    }
    
    if language == "js" {
      return "javascript"
    }
    
    if language == "objc" || language == "obj-c" || language == "objective-c" {
      return "objectivec"
    }
    
    return language
  }
  
  private func processAttributedString(_ string: NSMutableAttributedString, preferredFont: UIFont) -> (backgroundColor: Color?, string: AttributedString)? {
    let maxRange = NSRange(location: 0, length: string.length)
    let backgroundColor = string.attribute(.backgroundColor, at: 0, longestEffectiveRange: nil, in: maxRange) as? UIColor
    
    string.removeAttribute(.backgroundColor, range: maxRange)
    string.removeAttribute(.paragraphStyle, range: maxRange)
    string.removeAttribute(.font, range: maxRange)
    string.addAttribute(.font, value: preferredFont.with(traits: .traitMonoSpace), range: maxRange)
    
    if let backgroundColor = backgroundColor {
      return (Color(uiColor: backgroundColor), AttributedString(string))
    }
    return (nil, AttributedString(string))
  }
}

private struct HLJSParser {
  static func parse(_ string: String, styleSheet: [BasicCSSParser.CSSStyle], preferredFont: UIFont) -> NSMutableAttributedString? {
    let scanner = Scanner(string: string)
    scanner.charactersToBeSkipped = nil
      
    let result = NSMutableAttributedString()
    var styleAttributes = ["hljs"]
    
    while !scanner.isAtEnd {
      let tagContent = scanner.scanUpToString("<")
      if let tagContent = tagContent {
        if !tagContent.isEmpty {
          result.append(styleString(tagContent, properties: styleAttributes, styleSheet: styleSheet, preferredFont: preferredFont))
        }
        
        if scanner.isAtEnd {
          break
        }
      }
      
      scanner.currentIndex = scanner.string.index(after: scanner.currentIndex)
      let nextToken = scanner.string[scanner.currentIndex..<scanner.string.index(after: scanner.currentIndex)]
      
      if nextToken == "s" {
        _ = scanner.scanString("span class=\"")
        let tagAttributes = scanner.scanUpToString("\">")
        _ = scanner.scanString("\">")
        
        if let tagAttributes = tagAttributes {
          styleAttributes.append(".\(tagAttributes)")
        }
      } else if nextToken == "/" {
        _ = scanner.scanString("/span>")
        styleAttributes.removeLast()
      } else {
        result.append(styleString("<", properties: styleAttributes, styleSheet: styleSheet, preferredFont: preferredFont))
        scanner.currentIndex = scanner.string.index(after: scanner.currentIndex)
      }
    }
    
    guard let regex = try? NSRegularExpression(pattern: "&#?[A-Z0-9]+?;", options: [.caseInsensitive]) else {
      return result
    }

    let matches = regex.matches(in: result.string, options: [], range: NSRange(location: 0, length: result.length))
    for match in matches.reversed() {
      if let range = Range(match.range, in: result.string) {
        let entity = result.string[range]
        result.replaceCharacters(in: match.range, with: decodeEntities(String(entity)))
      }
    }
    
    return result
  }
  
  private static func styleString(_ string: String, properties: [String], styleSheet: [BasicCSSParser.CSSStyle], preferredFont: UIFont) -> NSAttributedString {
    let result = NSMutableAttributedString(string: string)
    result.addAttribute(.font, value: preferredFont, range: NSRange(location: 0, length: result.length))
    
    let properties = properties.flatMap({ $0.split(separator: " ") }).map({
      return $0.hasPrefix(".") ? String($0) : ".\($0)"
    })
    
    for property in properties {
      let rules = styleSheet.first(where: { $0.selector == property }).map({ $0.declaration }) ?? []
      
      for rule in rules {
        let cssAttributeName = rule.property
        let cssAttributeValue = rule.value
        
        let attribute: NSAttributedString.Key?
        var attributeValue: Any?
        
        switch cssAttributeName {
        case "color":
          attribute = .foregroundColor
          attributeValue = UIColor(colorString: cssAttributeValue)
        case "background-color":
          attribute = .backgroundColor
          attributeValue = UIColor(colorString: cssAttributeValue)
        case "font-weight":
          attribute = .font
          if cssAttributeValue == "bold" {
            attributeValue = preferredFont.with(traits: .traitBold)
          }
        case "font-style":
          attribute = .font
          if cssAttributeValue == "italic" {
            attributeValue = preferredFont.with(traits: .traitItalic)
          }
        case "font-size":
          attribute = .font
          if let fontSize = Float(cssAttributeValue) {
            attributeValue = preferredFont.withSize(CGFloat(fontSize))
          }
        case "text-decoration":
          attribute = .underlineStyle
          attributeValue = NSUnderlineStyle.single.rawValue
        default:
          attribute = nil
        }

        if let attribute = attribute, let attributeValue = attributeValue {
          result.addAttribute(attribute, value: attributeValue, range: NSRange(location: 0, length: string.count))
        }
      }
    }
    
    return result
  }
  
  private static func decodeEntities(_ string: String) -> String {
    var decodedString = string
    
    let code: UInt32?
    if string.hasPrefix("&#x") || string.hasPrefix("&#X") {
      code = UInt32(string.dropFirst(3).dropLast(), radix: 16)
    } else if string.hasPrefix("&#") {
      code = UInt32(string.dropFirst(2).dropLast(), radix: 10)
    } else {
      code = nil
    }
    
    if let code = code, let scalar = UnicodeScalar(code) {
      return String(scalar)
    }
      
    let entities = [
      "&quot;" : "\"", "&amp;" : "&", "&apos;" : "'",
      "&lt;" : "<", "&gt;" : ">", "&nbsp;" : " ",
      "&iexcl;" : "¡", "&cent;" : "¢", "&pound;" : "£",
      "&curren;" : "¤", "&yen;" : "¥", "&brvbar;" : "¦",
      "&sect;" : "§", "&uml;" : "¨", "&copy;" : "©",
      "&ordf;" : "ª", "&laquo;" : "«", "&not;" : "¬",
      "&shy;" : "\u{00AD}", "&reg;" : "®", "&macr;" : "¯",
      "&deg;" : "°", "&plusmn;" : "±", "&sup2;" : "²",
      "&sup3;" : "³", "&acute;" : "´", "&micro;" : "µ",
      "&para;" : "¶", "&middot;" : "·", "&cedil;" : "¸",
      "&sup1;" : "¹", "&ordm;" : "º", "&raquo;" : "»",
      "&frac14;" : "¼", "&frac12;" : "½", "&frac34;" : "¾",
      "&iquest;" : "¿", "&divide;" : "÷",
      "&ETH;" : "Ð", "&eth;" : "ð", "&THORN;" : "Þ",
      "&thorn;" : "þ", "&AElig;" : "Æ", "&aelig;" : "æ",
      "&OElig;" : "Œ", "&oelig;" : "œ", "&Aring;" : "Å",
      "&Auml;" : "Ä", "&Ccedil;" : "Ç", "&Egrave;" : "È",
      "&Eacute;" : "É", "&Ecirc;" : "Ê", "&Euml;" : "Ë",
      "&Igrave;" : "Ì", "&Iacute;" : "Í", "&Icirc;" : "Î",
      "&Iuml;" : "Ï", "&Ntilde;" : "Ñ", "&Ograve;" : "Ò",
      "&Oacute;" : "Ó", "&Ocirc;" : "Ô", "&Otilde;" : "Õ",
      "&Ouml;" : "Ö", "&times;" : "×", "&Oslash;" : "Ø",
      "&Ugrave;" : "Ù", "&Uacute;" : "Ú", "&Ucirc;" : "Û",
      "&Uuml;" : "Ü", "&Yacute;" : "Ý", "&szlig;" : "ß",
      "&agrave;" : "à", "&aacute;" : "á", "&acirc;" : "â",
      "&atilde;" : "ã", "&auml;" : "ä", "&aring;" : "å",
      "&ccedil;" : "ç", "&egrave;" : "è", "&eacute;" : "é",
      "&ecirc;" : "ê", "&euml;" : "ë", "&igrave;" : "ì",
      "&iacute;" : "í", "&icirc;" : "î", "&iuml;" : "ï",
      "&ntilde;" : "ñ", "&ograve;" : "ò", "&oacute;" : "ó",
      "&ocirc;" : "ô", "&otilde;" : "õ", "&ouml;" : "ö",
      "&oslash;" : "ø", "&ugrave;" : "ù", "&uacute;" : "ú",
      "&ucirc;" : "û", "&uuml;" : "ü", "&yacute;" : "ý",
      "&yuml;" : "ÿ"
    ]
      
    for (entity, character) in entities {
      decodedString = decodedString.replacingOccurrences(of: entity, with: character)
    }
    
    return decodedString
  }
}


struct BasicCSSParser {
  struct CSSStyle: Hashable {
    let selector: String
    let declaration: Set<CSSDeclaration>
  }

  struct CSSDeclaration: Hashable {
    let property: String
    let value: String
  }

  static func parse(_ styleSheet: String) -> [CSSStyle] {
    var result = [CSSStyle]()
    
    //(?:((?:\.[A-z0-9_-]*(?:[,\s]\.[A-z0-9_-]*)*)*))(?:\s*\{\s*(.*?)\s*\})
    let pattern = "(?:((?:\\.[A-z0-9_-]*(?:[,\\s]\\.[A-z0-9_-]*)*)*))(?:\\s*\\{\\s*(.*?)\\s*\\})"
    let regex = try! NSRegularExpression(pattern: pattern, options: [])
    let matches = regex.matches(in: styleSheet, options: [], range: NSRange(location: 0, length: styleSheet.utf16.count))
    
    for match in matches {
      if match.numberOfRanges != 3 {  // Entire match, selector match, declaration match
        continue
      }
      
      let selectorRange = match.range(at: 1)
      let declarationRange = match.range(at: 2)
      
      if let selectorRange = Range(selectorRange, in: styleSheet),
         let declarationRange = Range(declarationRange, in: styleSheet) {
        
        let selector = String(styleSheet[selectorRange])
        let declarationString = String(styleSheet[declarationRange])
        
        var declarations = [CSSDeclaration]()
        for declaration in declarationString.components(separatedBy: ";") {
          let keyValue = declaration.components(separatedBy: ":")
          if keyValue.count == 2 {
            declarations.append(
              .init(
                property: keyValue[0].trimmingCharacters(in: .whitespacesAndNewlines),
                value: keyValue[1].trimmingCharacters(in: .whitespacesAndNewlines)
              )
            )
          }
        }
        
        if !declarations.isEmpty {
          result.append(
            .init(
              selector: selector,
              declaration: Set(declarations)
            )
          )
        }
      }
    }
      
    var styles = [CSSStyle]()
    for style in result {
      // Technically:
      // .hljs-class .hljs-title => apply .hljs-title only to elements that are a decendent of an element with .hljs-class
      // .hljs-title.class_ => apply style only if element has both classes
      // .hljs-title, .hljs-class -> apply style to element with at least one of the classes
      //
      // let selectors = style.selector.replacingOccurrences(of: " ", with: ",").components(separatedBy: ",")
      
      let selectors = style.selector.components(separatedBy: ",").map({ $0.trimmingCharacters(in: .whitespacesAndNewlines) })
      for selector in selectors {
        
        if let index = styles.firstIndex(where: { $0.selector == selector }) {
          var declaration = styles[index].declaration
          declaration.formUnion(style.declaration)
          styles[index] = .init(selector: selector, declaration: declaration)
        } else {
          styles.append(.init(selector: selector, declaration: style.declaration))
        }
      }
    }
    
    return styles
  }
}
