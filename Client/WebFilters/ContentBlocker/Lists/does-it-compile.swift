#!/usr/bin/swift

import WebKit
import Foundation

enum CompileResult {
  case success
  case failed(String)
}

/// Compile a content blocker list synchronously, must be run on background thread
func synchronousCompileList(_ list: String, to ruleStore: WKContentRuleListStore = .default()!) -> CompileResult {
  let group = DispatchGroup()
  var result: CompileResult!
  
  group.enter()
  ruleStore.compileContentRuleList(forIdentifier: "list", encodedContentRuleList: list, completionHandler: { list, error in
    if let error = error {
      if let errorString = (error as NSError).userInfo["NSHelpAnchor"] as? String {
        result = .failed(errorString)
      } else {
        result = .failed(String(describing: error))
      }
    } else {
      result = .success
    }
    group.leave()
  })
  group.wait()
  return result
}

let compileQueue = DispatchQueue(label: "compile")
var anyFailed = false

for path in CommandLine.arguments.dropFirst().map({ NSString(string: $0) }) {
  guard let list = try? String(contentsOfFile: path.expandingTildeInPath) else {
    print("Failed to load \"\(path)\" to compile")
    continue
  }
  compileQueue.async {
    let timeStart = Date()
    print("Compiling: \(path.lastPathComponent)", terminator: "")
    let result = synchronousCompileList(list)
    let performance = String(format: "(%.02fs)", Date().timeIntervalSince(timeStart))
    switch result {
    case .success:
      print("... ✓ \(performance)")
    case .failed(let errorDescription):
      print("... 𐄂 \(performance)")
      print("  → \(errorDescription)")
      anyFailed = true
    }
  }
}

compileQueue.async {
  exit(anyFailed ? EXIT_FAILURE : EXIT_SUCCESS)
}

RunLoop.main.run()
