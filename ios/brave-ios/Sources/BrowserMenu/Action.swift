// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import Foundation
import Strings
import SwiftUI

public struct Action: Hashable, Identifiable, Sendable {

  public enum HandlerResult {
    case none
    case updateAction(Action)
  }

  public struct Attributes: OptionSet, Hashable, Sendable {
    public var rawValue: Int = 0
    public init(rawValue: Int) {
      self.rawValue = rawValue
    }

    public static let disabled: Self = .init(rawValue: 1 << 0)
  }

  public struct Traits: Hashable, Sendable {
    public var badgeColor: UIColor? = nil

    public init(badgeColor: UIColor? = nil) {
      self.badgeColor = badgeColor
    }
  }

  public enum Visibility: Hashable, Sendable {
    case visible
    case hidden
  }

  public struct Identifier: Hashable, Sendable {
    public var id: String
    public var defaultTitle: String
    public var defaultImage: String
    public var defaultRank: Int
    public var defaultVisibility: Visibility

    public init(
      id: String,
      title: String,
      braveSystemImage: String,
      defaultRank: Int,
      defaultVisibility: Visibility
    ) {
      self.id = id
      self.defaultTitle = title
      self.defaultImage = braveSystemImage
      self.defaultRank = defaultRank
      self.defaultVisibility = defaultVisibility
    }
  }

  public var id: Identifier
  public var title: String
  public var image: String
  public var attributes: Attributes
  public var traits: Traits
  public var state: Bool?
  public var handler: @Sendable @MainActor (Action) async -> HandlerResult

  public func hash(into hasher: inout Hasher) {
    hasher.combine(id)
    hasher.combine(title)
    hasher.combine(image)
    hasher.combine(attributes)
    hasher.combine(traits)
    hasher.combine(state)
  }

  public static func == (lhs: Self, rhs: Self) -> Bool {
    lhs.id == rhs.id && lhs.title == rhs.title && lhs.image == rhs.image
      && lhs.attributes == rhs.attributes && lhs.traits == rhs.traits && lhs.state == rhs.state
  }

  public init(
    id: Identifier,
    title: String? = nil,
    image: String? = nil,
    attributes: Attributes = [],
    traits: Traits = .init(),
    state: Bool? = nil,
    handler: (@Sendable (Action) async -> HandlerResult)? = nil
  ) {
    self.id = id
    self.title = title ?? id.defaultTitle
    self.image = image ?? id.defaultImage
    self.attributes = attributes
    self.traits = traits
    self.state = state
    self.handler = handler ?? { _ in .none }
  }
}
