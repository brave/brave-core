/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import Combine

/// A view which embeds some SwiftUI within a `UIScrollView`. This scroll view takes care of
/// adjusting the `contentInset` based on keyboard events.
///
/// On iOS 14, this view automatically ignores the safe area, since the underlying `UIScrollView`
/// will handles keyboard observation itself.
public struct UIKitScrollView<Content: View>: View {
  public var axis: Axis.Set
  public var content: Content
  
  public init(axis: Axis.Set, @ViewBuilder content: () -> Content) {
    self.axis = axis
    self.content = content()
  }
  
  private var scrollView: some View {
    _UIKitScrollView(axis: axis, content: content)
  }
  
  public var body: some View {
    if #available(iOS 14.0, *) {
      scrollView
        .ignoresSafeArea()
    } else {
      scrollView
    }
  }
  
  struct _UIKitScrollView<Content: View>: UIViewControllerRepresentable {
    typealias UIViewControllerType = ScrollingHostingController<Content>
    var axis: Axis.Set
    var content: Content
    func makeUIViewController(context: Context) -> UIViewControllerType {
      ScrollingHostingController(rootView: content, axis: axis)
    }
    func updateUIViewController(_ uiViewController: UIViewControllerType, context: Context) {
    }
  }
}

/// A controller which embeds some SwiftUI within a `UIScrollView`. This scroll view takes care of
/// adjusting the `contentInset` based on keyboard events.
public class ScrollingHostingController<Content: View>: UIViewController {
  private let hostingController: UIHostingController<Content>
  private let axis: Axis.Set
  private var keyboardObserver: AnyCancellable?
  
  let scrollView: UIScrollView = .init()
  
  init(rootView: Content, axis: Axis.Set = []) {
    hostingController = UIHostingController(rootView: rootView)
    self.axis = axis
    super.init(nibName: nil, bundle: nil)
    
    keyboardObserver = NotificationCenter.default.publisher(for: UIApplication.keyboardWillShowNotification)
      .merge(with: NotificationCenter.default.publisher(for: UIApplication.keyboardWillHideNotification))
      .map { notification -> CGRect in
        guard let frame = notification.userInfo?[UIApplication.keyboardFrameEndUserInfoKey] as? CGRect else {
          return .zero
        }
        return frame
      }
      .sink(receiveValue: { [weak self] rect in
        self?.handleKeyboardFrameChange(rect)
      })
  }
  
  private func handleKeyboardFrameChange(_ rect: CGRect) {
    let keyboardViewEndFrame = view.convert(rect, from: view.window)
    let scrollViewIntersection = scrollView.frame.intersection(keyboardViewEndFrame).height
    let insets = UIEdgeInsets(top: 0.0, left: 0.0, bottom: max(0, scrollViewIntersection - view.safeAreaInsets.bottom), right: 0.0)
    scrollView.contentInset = insets
    scrollView.scrollIndicatorInsets = insets
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
  
    view.backgroundColor = .clear
    view.addSubview(scrollView)
    addChild(hostingController)
    hostingController.didMove(toParent: self)
    scrollView.addSubview(hostingController.view)
    
    scrollView.translatesAutoresizingMaskIntoConstraints = false
    hostingController.view.translatesAutoresizingMaskIntoConstraints = false
    
    NSLayoutConstraint.activate([
      scrollView.topAnchor.constraint(equalTo: view.topAnchor),
      scrollView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      scrollView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      scrollView.trailingAnchor.constraint(equalTo: view.trailingAnchor)
    ])
    
    if (axis.contains(.vertical) && axis.contains(.horizontal)) || axis.isEmpty {
      NSLayoutConstraint.activate([
        scrollView.contentLayoutGuide.leadingAnchor.constraint(equalTo: hostingController.view.leadingAnchor),
        scrollView.contentLayoutGuide.trailingAnchor.constraint(equalTo: hostingController.view.trailingAnchor),
        scrollView.contentLayoutGuide.topAnchor.constraint(equalTo: hostingController.view.topAnchor),
        scrollView.contentLayoutGuide.bottomAnchor.constraint(equalTo: hostingController.view.bottomAnchor),
      ])
    } else if axis.contains(.vertical) {
      NSLayoutConstraint.activate([
        scrollView.contentLayoutGuide.widthAnchor.constraint(equalTo: view.widthAnchor),
        scrollView.contentLayoutGuide.topAnchor.constraint(equalTo: hostingController.view.topAnchor),
        scrollView.contentLayoutGuide.bottomAnchor.constraint(equalTo: hostingController.view.bottomAnchor),
        hostingController.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
        hostingController.view.trailingAnchor.constraint(equalTo: view.trailingAnchor)
      ])
    } else if axis.contains(.horizontal) {
      NSLayoutConstraint.activate([
        scrollView.contentLayoutGuide.heightAnchor.constraint(equalTo: view.heightAnchor),
        scrollView.contentLayoutGuide.leadingAnchor.constraint(equalTo: hostingController.view.leadingAnchor),
        scrollView.contentLayoutGuide.trailingAnchor.constraint(equalTo: hostingController.view.trailingAnchor),
        hostingController.view.topAnchor.constraint(equalTo: view.topAnchor),
        hostingController.view.bottomAnchor.constraint(equalTo: view.bottomAnchor)
      ])
    }
  }
}

struct ScrollingHostingController_Previews: PreviewProvider {
  struct UIKitControllerPreview: UIViewControllerRepresentable {
    var make: () -> UIViewController
    func makeUIViewController(context: Context) -> UIViewController {
      make()
    }
    func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
    }
    init(_ make: @autoclosure @escaping () -> UIViewController) {
      self.make = make
    }
  }
  
  static var previews: some View {
    Group {
      UIKitControllerPreview(
        ScrollingHostingController(
          rootView: Color.red.frame(height: 300),
          axis: .vertical
        )
      )
      .previewDisplayName("Vertical")
      UIKitControllerPreview(
        ScrollingHostingController(
          rootView: Color.red.frame(width: 200),
          axis: .horizontal)
      )
      .previewDisplayName("Horizontal")
      UIKitControllerPreview(
        ScrollingHostingController(
          rootView: LinearGradient(
            gradient: Gradient(colors: [Color.red, Color.blue]),
            startPoint: .leading,
            endPoint: .trailing
          ).frame(width: 500, height: 400)
        )
      )
      .previewDisplayName("Both")
    }
  }
}
