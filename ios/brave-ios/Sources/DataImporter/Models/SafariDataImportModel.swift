// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Growth

@Observable
public class SafariDataImportModel: SafariDataImportClientDelegate {
  enum State {
    case initial
    case failure
    case importing
    case passwordConflict
    case success
  }

  var state: State = .initial

  @ObservationIgnored
  private var importingFilePath: URL?

  @ObservationIgnored
  private(set) var conflictedPasswordIDs: [NSNumber]?

  private let coordinator: any SafariDataImporterCoordinator

  public init(
    coordinator: any SafariDataImporterCoordinator
  ) {
    self.coordinator = coordinator
    self.coordinator.delegate = self
  }

  deinit {
    if state == .importing {
      coordinator.importer.cancelImport()
    }
    coordinator.delegate = nil
    importingFilePath?.stopAccessingSecurityScopedResource()
  }

  func beginImport(for fileURL: URL) {
    if fileURL.startAccessingSecurityScopedResource() {
      importingFilePath = fileURL
      state = .importing
      coordinator.importer.prepareImportForFile(atPath: fileURL.path)
    }
  }

  enum ConflictResolutionOption {
    case keepBravePasswords
    case keepSafariPasswords
  }

  func resolveConflicts(_ option: ConflictResolutionOption) {
    guard state == .passwordConflict, let conflictedPasswordIDs else {
      return
    }
    switch option {
    case .keepBravePasswords:
      coordinator.importer.completeImport(withSelectedPasswords: nil)
    case .keepSafariPasswords:
      coordinator.importer.completeImport(withSelectedPasswords: conflictedPasswordIDs)
    }
  }

  func reset() {
    conflictedPasswordIDs = nil
    importingFilePath?.stopAccessingSecurityScopedResource()
    importingFilePath = nil
    state = .initial
  }

  private enum ImportType: CaseIterable {
    case bookmarks
    case history
    case passwords
    case paymentCards
  }

  @ObservationIgnored
  private var readyTypes: Set<ImportType> = [] {
    didSet {
      if readyTypes.count == ImportType.allCases.count {
        if case .passwordConflict = state {
          // Conflict resolution required prior to continuation
          return
        }
        // Continue to import
        coordinator.importer.completeImport(withSelectedPasswords: nil)
      }
    }
  }

  @ObservationIgnored
  private var importedTypes: Set<ImportType> = [] {
    didSet {
      if importedTypes.count == ImportType.allCases.count {
        state = .success
        importingFilePath?.stopAccessingSecurityScopedResource()
        importingFilePath = nil
        recordP3A(answer: .importedFromSafari)
      }
    }
  }

  // MARK: - P3A

  private enum Answer: Int, CaseIterable {
    case didNotImport = 1
    case importedFromBrave = 2
    case importedFromChrome = 3
    case importedFromFirefox = 4
    case importedFromBookmarksHTML = 5
    case importedFromSafari = 6
  }

  private func recordP3A(answer: Answer) {
    UmaHistogramEnumeration("Brave.Importer.ImporterSource.2", sample: answer)
  }

  // MARK: - SafariDataImportClientDelegate

  public func onTotalFailure() {
    state = .failure
    recordP3A(answer: .didNotImport)
  }

  public func onBookmarksReady(_ count: Int) {
    readyTypes.insert(.bookmarks)
  }

  public func onHistoryReady(_ estimatedCount: Int) {
    readyTypes.insert(.history)
  }

  public func onPasswordsReady(_ conflictedPasswordIDs: [NSNumber]) {
    if !conflictedPasswordIDs.isEmpty {
      self.conflictedPasswordIDs = conflictedPasswordIDs
      state = .passwordConflict
    }
    readyTypes.insert(.passwords)
  }

  public func onPaymentCardsReady(_ count: Int) {
    readyTypes.insert(.paymentCards)
  }

  public func onBookmarksImported(_ count: Int) {
    importedTypes.insert(.bookmarks)
  }

  public func onHistoryImported(_ count: Int) {
    importedTypes.insert(.history)
  }

  public func onPasswordsImported(_ count: Int) {
    importedTypes.insert(.passwords)
  }

  public func onPaymentCardsImported(_ count: Int) {
    importedTypes.insert(.paymentCards)
  }
}
