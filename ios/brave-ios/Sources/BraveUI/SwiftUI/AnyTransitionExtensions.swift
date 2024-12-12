// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

extension AnyTransition {
  /// Configuration properties for a transition.
  public enum BlurReplaceTransitionConfiguration: Hashable, Sendable {
    /// A configuration that requests a transition that scales the
    /// view down while removing it and up while inserting it.
    case downUp

    /// A configuration that requests a transition that scales the
    /// view up while both removing and inserting it.
    case upUp

    @available(iOS 17.0, *)
    fileprivate var configuration: BlurReplaceTransition.Configuration {
      switch self {
      case .downUp: return .downUp
      case .upUp: return .upUp
      }
    }
  }
  /// A blur replace transition or a fallback for iOS 16 users
  public static func blurReplace(
    configuration: BlurReplaceTransitionConfiguration = .downUp,
    fallback: AnyTransition = .opacity
  ) -> AnyTransition {
    if #available(iOS 17.0, *) {
      return AnyTransition(.blurReplace(configuration.configuration))
    }
    return fallback
  }
}
