// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Fuzi
import JavaScriptCore
import Shared
import SwiftUI
import os.log

class HighlightJS {
  private var isDarkTheme: Bool
  private var context: JSContext?
  private var theme: [BasicCSSParser.CSSStyle]?
  private var script: String?

  static let dark = HighlightJS(darkTheme: true)
  static let light = HighlightJS(darkTheme: false)

  private init(darkTheme: Bool) {
    isDarkTheme = darkTheme
  }

  func prepareHighlighter() async {
    if context != nil {
      return
    }
    let scriptName = "atom-one-\(isDarkTheme ? "dark" : "light").min"
    guard let stylesheetPath = Bundle.module.url(forResource: scriptName, withExtension: "css"),
      let scriptPath = Bundle.module.url(forResource: "highlight.min", withExtension: "js"),
      let stylesheet = await AsyncFileManager.default.utf8Contents(at: stylesheetPath),
      let script = await AsyncFileManager.default.utf8Contents(at: scriptPath)
    else {
      return
    }
    theme = BasicCSSParser.parse(stylesheet)
    context = JSContext()
    context?.evaluateScript("var window = {};\n\n\(script)")
  }

  func highlight(
    _ code: String,
    preferredFont: UIFont,
    language: String? = nil
  ) -> (backgroundColor: Color?, string: AttributedString)? {
    guard let context, let theme else {
      Logger.module.warning("Attempted to highlight code without first calling prepareHighlighter")
      return nil
    }
    let result: JSValue?
    if let language = language {
      let highlight = context.objectForKeyedSubscript("hljs").objectForKeyedSubscript("highlight")
      result = highlight?.call(withArguments: [
        normalizeLanguage(language), code, false,  // False = Do not highlight invalid syntax
      ])
    } else {
      let highlight = context.objectForKeyedSubscript("hljs").objectForKeyedSubscript(
        "highlightAuto"
      )
      result = highlight?.call(withArguments: [code])
    }

    if let result = result?.objectForKeyedSubscript("value"), !result.isUndefined, !result.isNull,
      let code = result.toString()
    {

      // If our custom parser can parse the string, then use it.
      // It's significantly faster than Apple's parser.
      if let string = HLJSParser.parse(
        "<pre><code class='hljs'>\(code)</code></pre>",
        styleSheet: theme,
        preferredFont: preferredFont
      ) {
        return processAttributedString(string, preferredFont: preferredFont)
      }
    }

    return nil
  }

  private func normalizeLanguage(_ language: String) -> String {
    // If we want more languages, we have to use the full HighlightJS package which is very tiny anyway
    // I'm only using a subset atm though:

    // let supportedLanguages = [
    //  "bash", "c", "cpp", "csharp", "css", "diff", "go", "graphql",
    //  "ini", "java", "javascript", "json", "kotlin", "less", "lua",
    //  "makefile", "markdown", "objectivec", "perl", "php", "php-template",
    //  "plaintext", "python", "python-repl", "r", "ruby", "rust", "scss",
    //  "shell", "sql", "swift", "typescript", "vbnet", "wasm", "xml", "yaml"]

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

  private func processAttributedString(
    _ string: NSMutableAttributedString,
    preferredFont: UIFont
  ) -> (backgroundColor: Color?, string: AttributedString)? {
    if string.length == 0 {
      return (nil, AttributedString(string))
    }

    let maxRange = NSRange(location: 0, length: string.length)
    let backgroundColor =
      string.attribute(.backgroundColor, at: 0, longestEffectiveRange: nil, in: maxRange)
      as? UIColor

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
  static func parse(
    _ string: String,
    styleSheet: [BasicCSSParser.CSSStyle],
    preferredFont: UIFont
  ) -> NSMutableAttributedString? {
    if string.isEmpty {
      return nil
    }

    do {
      let doc = try HTMLDocument(string: string, encoding: .utf8)

      // Setup Stacks
      var styleAttributes = [String]()
      var nodes: [XMLNode?] = doc.body?.childNodes(ofTypes: [.Text, .Element]).reversed() ?? []

      if nodes.isEmpty {
        return nil
      }

      // Stack based parsing instead of recursion
      let result = NSMutableAttributedString()
      while !nodes.isEmpty {
        // We encountered the nil delimiter
        guard let node = nodes.removeLast() else {

          // Pop the style from the stack
          if !styleAttributes.isEmpty {
            _ = styleAttributes.removeLast()
          }

          continue
        }

        // Text node encountered
        if node.type == XMLNodeType.Text {

          // Style the text
          result.append(
            styleString(
              node.stringValue,
              properties: styleAttributes,
              styleSheet: styleSheet,
              preferredFont: preferredFont
            )
          )
          continue
        }

        // Must be element.type == XMLNodeType.Element due to our node selector
        guard let element = node.toElement() else {
          continue
        }

        // Push the style onto the stack
        if let style = element.attr("class") {
          styleAttributes.append(style)
        }

        // Delimiter to know when to pop the styles of the stack
        nodes.append(nil)

        // Push the nested child nodes
        nodes.append(contentsOf: element.childNodes(ofTypes: [.Text, .Element]).reversed())
      }

      return result
    } catch {
      Logger.module.error("[HLJSParser] - Error Parsing HTML: \(error)")
      return nil
    }
  }

  private static func styleString(
    _ string: String,
    properties: [String],
    styleSheet: [BasicCSSParser.CSSStyle],
    preferredFont: UIFont
  ) -> NSAttributedString {
    let result = NSMutableAttributedString(string: string)
    if string.isEmpty {
      return result
    }

    result.addAttribute(
      .font,
      value: preferredFont,
      range: NSRange(location: 0, length: result.length)
    )

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
          result.addAttribute(
            attribute,
            value: attributeValue,
            range: NSRange(location: 0, length: string.count)
          )
        }
      }
    }

    return result
  }
}

public struct BasicCSSParser {
  public struct CSSStyle: Hashable {
    public let selector: String
    public let declaration: Set<CSSDeclaration>
  }

  public struct CSSDeclaration: Hashable {
    public let property: String
    public let value: String
  }

  public static func parse(_ styleSheet: String) -> [CSSStyle] {
    var result = [CSSStyle]()

    //(?:((?:\.[A-z0-9_-]*(?:[,\s]\s*\.[A-z0-9_-]*)*)*))(?:\s*\{\s*([\s\S]*?)\s*\})
    let pattern =
      "(?:((?:\\.[A-z0-9_-]*(?:[,\\s]\\s*\\.[A-z0-9_-]*)*)*))(?:\\s*\\{\\s*([\\s\\S]*?)\\s*\\})"
    let regex = try! NSRegularExpression(pattern: pattern, options: [])
    let matches = regex.matches(
      in: styleSheet,
      options: [],
      range: NSRange(location: 0, length: styleSheet.utf16.count)
    )

    for match in matches {
      if match.numberOfRanges != 3 {  // Entire match, selector match, declaration match
        continue
      }

      let selectorRange = match.range(at: 1)
      let declarationRange = match.range(at: 2)

      if let selectorRange = Range(selectorRange, in: styleSheet),
        let declarationRange = Range(declarationRange, in: styleSheet)
      {

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

      let selectors = style.selector.components(separatedBy: ",").map({
        $0.trimmingCharacters(in: .whitespacesAndNewlines)
      })
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
