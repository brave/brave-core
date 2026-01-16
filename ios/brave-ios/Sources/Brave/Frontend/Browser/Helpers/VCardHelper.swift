// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Contacts
import ContactsUI
import Foundation
import UIKit
import os.log

/// Protocol for VCard presentation delegate
protocol VCardHelperDelegate: AnyObject {
  /// Called to present the VCard contact view controller
  func presentVCardViewController(_ viewController: UIViewController)
  /// Called when the VCard view controller should be dismissed
  func dismissVCardViewController()
}

/// Helper class to handle VCard (.vcf) file presentation using CNContactViewController
///
/// This class parses VCard data and presents it in a native iOS contact modal,
/// matching the behavior of Safari and Chrome on iOS.
class VCardHelper: NSObject {
  enum VCardError: Error {
    case parseFailed
  }
  weak var delegate: VCardHelperDelegate?

  /// Opens a VCard from the provided data and presents it in a contact view controller
  /// - Parameter data: The raw VCard data (typically from a .vcf file download)
  @MainActor
  func openVCard(from data: Data) throws {
    let contacts = try CNContactVCardSerialization.contacts(with: data)
    // Only display the first contact for now, ignoring any additional contacts in the VCard.
    guard let contact = contacts.first else {
      throw VCardError.parseFailed
    }

    let contactViewController = CNContactViewController(forUnknownContact: contact)
    contactViewController.allowsEditing = true
    contactViewController.contactStore = CNContactStore()

    let navigationController = UINavigationController(rootViewController: contactViewController)

    let doneButton = UIBarButtonItem(
      barButtonSystemItem: .done,
      target: self,
      action: #selector(dismissButtonTapped)
    )
    contactViewController.navigationItem.leftBarButtonItem = doneButton

    delegate?.presentVCardViewController(navigationController)
  }

  @objc private func dismissButtonTapped() {
    delegate?.dismissVCardViewController()
  }
}
