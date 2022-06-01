// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import WebKit
import Shared
import Data
import BraveShared
import Combine

private let log = Logger.browserLogger

protocol ContentBlocker: AnyObject, Hashable {
  // Make constant `let
  var filename: String { get }
  var rule: WKContentRuleList? { get set }
  var fileVersionPref: Preferences.Option<String?>? { get }
  var fileVersion: String? { get }
}

extension ContentBlocker {
  func buildRule(ruleStore: WKContentRuleListStore) -> AnyPublisher<Void, Error> {
    needsCompiling(ruleStore: ruleStore)
      .subscribe(on: DispatchQueue.global(qos: .userInitiated))
      .receive(on: DispatchQueue.main)
      .flatMap { needsCompiling -> AnyPublisher<String, Error> in
        if !needsCompiling {
          return Empty(outputType: String.self, failureType: Error.self).eraseToAnyPublisher()
        }
        
        return BlocklistName.loadJsonFromBundle(forResource: self.filename)
      }
      .map { jsonString in
        if jsonString.isEmpty {
          return
        }
        
        ruleStore.compileContentRuleList(forIdentifier: self.filename, encodedContentRuleList: jsonString) { rule, error in
          if let error = error {
            // TODO #382: Potential telemetry location
            log.error("Content blocker '\(self.filename)' errored: \(error.localizedDescription)")
            assert(false)
            return
          }
          assert(rule != nil)
          
          self.rule = rule
          self.fileVersionPref?.value = self.fileVersion
        }
      }.eraseToAnyPublisher()
  }

  private func needsCompiling(ruleStore: WKContentRuleListStore) -> AnyPublisher<Bool, Never> {
    return Future { [weak self] completion in
      guard let self = self else {
        completion(.success(true))
        return
      }
      
      if self.fileVersionPref?.value != self.fileVersion {
        // New file, so we must update the lists, no need to check the store
        completion(.success(true))
        return
      }
      
      ruleStore.lookUpContentRuleList(forIdentifier: self.filename) { rule, error in
        if let error = error {
          log.error(error)
          completion(.success(true))
          return
        }
        
        self.rule = rule
        completion(.success(false))
      }
    }.eraseToAnyPublisher()
  }

  private static func loadJsonFromBundle(forResource file: String) -> AnyPublisher<String, Error> {
    Combine.Deferred {
      Future { completion in
        do {
          guard let path = Bundle.current.path(forResource: file, ofType: "json") else {
            assert(false)
            completion(.failure("Failed to Load JSON From Bundle - Resource: \(file)"))
            return
          }
          
          let source = try String(contentsOfFile: path, encoding: .utf8)
          completion(.success(source))
        } catch {
          completion(.failure(error))
        }
      }
    }.eraseToAnyPublisher()
  }

  public static func == (lhs: Self, rhs: Self) -> Bool {
    return lhs.filename == rhs.filename
  }

  public func hash(into hasher: inout Hasher) {
    hasher.combine(filename)
  }
}
