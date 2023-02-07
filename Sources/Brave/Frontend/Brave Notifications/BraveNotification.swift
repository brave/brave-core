// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

struct BraveNotificationPriority: Comparable {
  var value: Int
  static let low: Self = .init(value: 0)
  static let high: Self = .init(value: 1)
  
  public static func < (lhs: BraveNotificationPriority, rhs: BraveNotificationPriority) -> Bool {
    lhs.value < rhs.value
  }
}

enum DismissPolicy {
  case automatic(after: TimeInterval = 30)
  case explicit
}

enum PresentationOrigin {
  case top
  case bottom
}

protocol BraveNotification: AnyObject {
  var priority: BraveNotificationPriority { get }
  var view: UIView { get }
  var dismissAction: (() -> Void)? { get set }
  var dismissPolicy: DismissPolicy { get }
  var id: String { get }
  var presentationOrigin: PresentationOrigin { get }
  
  func willDismiss(timedOut: Bool)
}

extension BraveNotification {
  public var dismissPolicy: DismissPolicy { .automatic() }
  public var priority: BraveNotificationPriority { .high }
  public var presentationOrigin: PresentationOrigin { .top }
}
