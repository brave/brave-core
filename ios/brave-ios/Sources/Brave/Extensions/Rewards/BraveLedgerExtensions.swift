// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import Shared
import os.log

extension BraveRewardsAPI {

  public var isLedgerTransferExpired: Bool {
    if Locale.current.region?.identifier != "JP" {
      return false
    }
    let now = Date()
    let deadlineComponents = DateComponents(year: 2021, month: 3, day: 13)
    guard
      let deadlineDate = Calendar(identifier: .gregorian).nextDate(
        after: now,
        matching: deadlineComponents,
        matchingPolicy: .strict
      )
    else {
      return true
    }
    return now >= deadlineDate
  }

  // MARK: -

  /// Creates the ledger wallet and fetches wallet properties and balances
  ///
  /// Use this is in UI instead of `createWallet` directly unless required
  public func createWalletAndFetchDetails(_ completion: @escaping (Bool) -> Void) {
    createWallet { [weak self] (error) in
      guard let self = self else { return }

      if let _ = error {
        completion(false)
        return
      }

      self.setMinimumVisitDuration(8)
      self.setMinimumNumberOfVisits(1)

      let group = DispatchGroup()
      var success = true
      group.enter()
      self.getRewardsParameters { details in
        if details == nil {
          success = false
        }
        group.leave()
      }
      group.enter()
      self.fetchBalance { balance in
        if balance == nil {
          success = false
        }
        group.leave()
      }
      group.notify(
        queue: .main,
        execute: {
          completion(success)
        }
      )
    }
  }

  @MainActor private func fetchPaymentId() async -> String? {
    await withCheckedContinuation { c in
      rewardsInternalInfo { info in
        c.resume(returning: info?.paymentId)
      }
    }
  }

  public func setupDeviceCheckEnrollment(
    _ client: DeviceCheckClient,
    completion: @escaping () -> Void
  ) {
    // Enroll in DeviceCheck
    client.generateToken { [weak self] (token, error) in
      guard let self = self else { return }
      if let error = error {
        Logger.module.error("Failed to generate DeviceCheck token: \(error.localizedDescription)")
        completion()
        return
      }
      Task { @MainActor in
        do {
          guard let paymentId = await self.fetchPaymentId(), !paymentId.isEmpty else {
            // No wallet to register
            completion()
            return
          }
          let registration = try client.generateEnrollment(paymentId: paymentId, token: token)
          try await client.registerDevice(enrollment: registration)
        } catch {
          Logger.module.error(
            "Failed to register device with mobile attestation server: \(error.localizedDescription)"
          )
        }
        completion()
      }
    }
  }

  func solveAdaptiveCaptcha(paymentId: String, captchaId: String) async throws {
    let deviceCheck = DeviceCheckClient()
    if !DeviceCheckClient.isDeviceEnrolled() {
      let didEnroll: Bool = await withCheckedContinuation { c in
        setupDeviceCheckEnrollment(deviceCheck) {
          c.resume(returning: DeviceCheckClient.isDeviceEnrolled())
        }
      }
      if !didEnroll {
        Logger.module.error(
          "Cannot solve adaptive captcha as user is not enrolled with the attestation server"
        )
        return
      }
    }

    let attestation = try deviceCheck.generateAttestation(paymentId: paymentId)
    let blob = try await deviceCheck.getAttestation(attestation: attestation)
    let verification = try deviceCheck.generateAttestationVerification(nonce: blob.nonce)
    try await deviceCheck.setAttestation(nonce: blob.nonce, verification: verification)
    try await deviceCheck.solveAdaptiveCaptcha(
      paymentId: paymentId,
      captchaId: captchaId,
      verification: verification
    )
  }
}
