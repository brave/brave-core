/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import CoreGraphics
import pop

// MARK: - Reusable Animations

/// Defines a protocol that may be used to obtain a property animator
public protocol ExpressibleByPropertyAnimator {
  
  /// The pop animatable property
  var animatableProperty: POPAnimatableProperty { get }
}

extension POPAnimatableProperty: ExpressibleByPropertyAnimator {
  public var animatableProperty: POPAnimatableProperty {
    return self
  }
}

extension String: ExpressibleByPropertyAnimator {
  public var animatableProperty: POPAnimatableProperty {
    return POPAnimatableProperty.property(withName: self) as! POPAnimatableProperty // swiftlint:disable:this force_cast
  }
}

extension NSObject {
  
  private func animate<T>(type: T.Type,
                          property: ExpressibleByPropertyAnimator,
                          key: String,
                          _ animation: (T, Bool) -> Void) -> T where T: POPPropertyAnimation {
    if let running = pop_animation(forKey: key) as? T {
      animation(running, true)
      return running
    }
    
    let newAnimation = T()
    newAnimation.removedOnCompletion = true
    newAnimation.property = property.animatableProperty
    animation(newAnimation, false)
    pop_add(newAnimation, forKey: key)
    return newAnimation
  }
  
  /// Create a spring animation to animate a given property.
  ///
  /// - parameters:
  ///   - property: The property to animate. See ExpressibleByPropertyAnimator
  ///   - key: The unique key for this animation.
  ///   - animation: The setup for this animation.
  ///   - _ The animatable object. Set the appropriate damping, and such.
  ///   - inProgress: Whether or not the spring animation was already running for this property
  ///
  /// - returns: A running POPSpringAnimation
  @discardableResult
  public func springAnimate(property: ExpressibleByPropertyAnimator, key: String, _ animation: (_: POPSpringAnimation, _ inProgress: Bool) -> Void) -> POPSpringAnimation {
    return animate(type: POPSpringAnimation.self, property: property, key: key, animation)
  }
  
  /// Create a basic animation to animate a given property.
  ///
  /// - parameters:
  ///   - property: The property to animate. See ExpressibleByPropertyAnimator
  ///   - key: The unique key for this animation.
  ///   - animation: The setup for this animation.
  ///   - _ The animatable object. Set the appropriate duration, and such.
  ///   - inProgress: Whether or not the spring animation was already running for this property
  ///
  /// - returns: A running POPBasicAnimation
  @discardableResult
  public func basicAnimate(property: ExpressibleByPropertyAnimator, key: String, _ animation: (_: POPBasicAnimation, _ inProgress: Bool) -> Void) -> POPBasicAnimation {
    return animate(type: POPBasicAnimation.self, property: property, key: key, animation)
  }
}

// MARK: - Custom Property Animators

public protocol CustomPropertyAnimator {}

extension CustomPropertyAnimator {
  
  /// Creates an animatable property without dealing with C pointers from the pop library
  ///
  /// - parameters:
  ///   - name: The name of the property. Use Swift KeyPaths.
  ///   - reading: The reading block. Reads the value from the parent.
  ///   - writing: The writing block. Writes the value to the parent.
  ///   - threshold: The stride between values while animating.  Defaults to 0.01
  ///   - parent: The parent of this property
  ///   - values: The values to write to the parent.
  ///
  /// - returns: An animatable property object to be used in pop animations.
  public static func animatableProperty(
    name: String,
    reading: @escaping (_ parent: Self) -> [CGFloat],
    writing: @escaping (_ parent: Self, _ values: [CGFloat]) -> Void,
    threshold: CGFloat = 0.01
    ) -> POPAnimatableProperty {
    return POPAnimatableProperty.property(withName: name, initializer: { property in
      guard let property = property else { return }
      
      property.readBlock = {
        $1?[0] = reading($0 as! Self)[0] // swiftlint:disable:this force_cast
      }
      property.writeBlock = {
        writing($0 as! Self, [$1!.pointee]) // swiftlint:disable:this force_cast
      }
      
      property.threshold = threshold
    }) as! POPAnimatableProperty // swiftlint:disable:this force_cast
  }
}

extension NSObject: CustomPropertyAnimator {}
