//
//  AccessibilityEmbedInScrollViewViewModifier.swift
//  NativeWalletUI
//
//  Created by Kyle Hickinson on 2021-05-07.
//

import Foundation
import SwiftUI

extension ContentSizeCategory {
  /// A `Bool` value indicating whether the content size category is one that
  /// is associated with accessibility.
  ///
  /// - note: This should function identically to `isAccessibilityCategory`, however is available
  ///         prior to iOS 13.4. Can be removed when target is greater than 13.4
  @available(iOS, introduced: 13.0, obsoleted: 13.4)
  @_disfavoredOverload
  var isAccessibilityCategory: Bool {
    switch self {
    case .accessibilityExtraExtraExtraLarge,
         .accessibilityExtraExtraLarge,
         .accessibilityExtraLarge,
         .accessibilityLarge,
         .accessibilityMedium:
      return true
    default:
      return false
    }
  }
}

struct AccessibilityEmbedInScrollViewViewModifier: ViewModifier {
  @Environment(\.sizeCategory) private var sizeCategory
  
  // Note: Remove @ViewBuilder when CI is Xcode 12.5+
  @ViewBuilder func body(content: Content) -> some View {
    if sizeCategory.isAccessibilityCategory {
      ScrollView(.vertical) {
        content
          .padding(.vertical)
      }
    } else {
      content
    }
  }
}

extension View {
  /// Embeds the view inside a vertical axis `ScrollView` if the current size category is one that
  /// is associated with accessibility.
  ///
  /// - note: If a user transitions between an accessibility size category and normal size category,
  ///         this View will be removed and re-added to the view hierarchy and thus cause possible
  ///         `transition`, `onAppear`, and `onDisappear` executions.         
  /// - warning: Use this only when you know your View can fit inside all device sizes except when
  ///            the size category is using accessibility sizes.
  /// - warning: Embedding certain View's inside a ScrollView can cause them to layout differently,
  ///            such as `Spacer` and other views where a `.frame(maxHeight: .infinity)` modifier
  ///            is used. Ensure that your View renders correctly when an accessibility size
  ///            category is in use.
  public func accessibilityEmbedInScrollView() -> some View {
    modifier(AccessibilityEmbedInScrollViewViewModifier())
  }
}
