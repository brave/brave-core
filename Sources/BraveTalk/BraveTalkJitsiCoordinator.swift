// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import JitsiMeetSDK

/// Handles coordinating when to use the Jitsi SDK for better Brave Talk integration
///
/// Currently relies on AppConstants.buildChannel
@MainActor public class BraveTalkJitsiCoordinator: Sendable {
  /// Whether or not the jitsi SDK integration is enabled
  static var isIntegrationEnabled: Bool {
    // If needed to disable for certain build channels, check AppConstants.buildChannel here
    return true
  }
  
  public init() {
    if !AppConstants.buildChannel.isPublic {
      JitsiMeetLogger.add(BraveTalkJitsiLogHandler())
    }
  }
  
  public enum AppLifetimeEvent {
    case didFinishLaunching(options: [UIApplication.LaunchOptionsKey: Any] = [:])
    case continueUserActivity(NSUserActivity, restorationHandler: (([UIUserActivityRestoring]) -> Void)? = nil)
    case openURL(URL, options: [UIApplication.OpenURLOptionsKey: Any] = [:])
  }
  
  @discardableResult
  public static func sendAppLifetimeEvent(_ event: AppLifetimeEvent) -> Bool {
    guard Self.isIntegrationEnabled else { return false }
    let application = UIApplication.shared
    // Warning: Grabbing this shared instance automatically creates a React Native bridge
    let meet = JitsiMeet.sharedInstance()
    meet.destroyReactNativeBridge()
    // --
    switch event {
    case .didFinishLaunching(let options):
      return meet.application(application, didFinishLaunchingWithOptions: options)
    case .continueUserActivity(let activity, let restorationHandler):
      return meet.application(application, continue: activity, restorationHandler: restorationHandler)
    case .openURL(let url, let options):
      return meet.application(application, open: url, options: options)
    }
  }
  
  private var pipViewCoordinator: PiPViewCoordinator?
  private var jitsiMeetView: JitsiMeetView?
  private var isBraveTalkInPiPMode: Bool = false
  private var delegate: JitsiDelegate?
  private var isMuted: Bool = false
  public private(set) var isCallActive: Bool = false
  
  public func toggleMute() {
    guard Self.isIntegrationEnabled else { return }
    isMuted.toggle() // The SDK doesn't seem to call `audioMutedChanged` when we call setAudioMuted below…
    jitsiMeetView?.setAudioMuted(isMuted)
  }
  
  public enum KeyboardPressPhase {
    case began, changed, ended, cancelled
  }
  
  public func handleResponderPresses(presses: Set<UIPress>, phase: KeyboardPressPhase) {
    guard Self.isIntegrationEnabled, isCallActive else { return }
    let isSpacebarPressed = presses.contains(where: { $0.key?.keyCode == .keyboardSpacebar })
    switch phase {
    case .began:
      if isSpacebarPressed {
        isMuted = false
      }
    case .changed:
      if !isMuted && !isSpacebarPressed {
        isMuted = true
      }
    case .cancelled, .ended:
      if isSpacebarPressed {
        isMuted = true
      }
    }
    jitsiMeetView?.setAudioMuted(isMuted)
  }
  
  private func dismissJitsiMeetView(_ completion: @escaping () -> Void) {
    self.pipViewCoordinator?.hide() { _ in
      self.jitsiMeetView?.removeFromSuperview()
      self.jitsiMeetView = nil
      self.pipViewCoordinator = nil
      self.isCallActive = false
      // Destroy the bridge after they're done
      JitsiMeet.sharedInstance().destroyReactNativeBridge()
      completion()
    }
  }
  
  public func launchNativeBraveTalk(
    for room: String,
    token: String,
    onEnterCall: @escaping () -> Void,
    onExitCall: @escaping () -> Void
  ) {
    guard Self.isIntegrationEnabled else { return }

    // Only create the RN bridge when the user joins a call
    JitsiMeet.sharedInstance().instantiateReactNativeBridge()
    
    // Call this right away instead of waiting for the conference to join so that we can stop the page load
    // faster.
    onEnterCall()
    delegate = JitsiDelegate(
      conferenceWillJoin: { [weak self] in
        // Its possible for a permission prompt for mic to appear on the web page after joining but the
        // alert gets placed on top of the JitsiMeetView and eats touches. Weirdly it isn't actually visible
        // on top of the JitsiMeetView so the user can't even dismiss it.
        guard let jmv = self?.jitsiMeetView else { return }
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
          jmv.window?.bringSubviewToFront(jmv)
        }
      },
      conferenceJoined: { [weak self] in
        self?.isCallActive = true
      },
      conferenceTerminated: { [weak self] in
        guard let self = self else { return }
        self.dismissJitsiMeetView {
          onExitCall()
        }
      },
      enterPictureInPicture: { [weak self] in
        self?.pipViewCoordinator?.enterPictureInPicture()
      },
      audioIsMuted: { [weak self] isMuted in
        self?.isMuted = isMuted
      },
      readyToClose: { [weak self] in
        guard let self = self else { return }
        if !self.isCallActive {
          // Trying to leave the join screen
          self.dismissJitsiMeetView {
            onExitCall()
          }
        }
      }
    )
    
    jitsiMeetView = JitsiMeetView()
    jitsiMeetView?.delegate = delegate
    
    pipViewCoordinator = PiPViewCoordinator(withView: jitsiMeetView!)
    pipViewCoordinator?.configureAsStickyView()
    
    jitsiMeetView?.join(.braveTalkOptions(room: room, token: token))
    jitsiMeetView?.alpha = 0
    
    pipViewCoordinator?.show()
  }
  
  public func resetPictureInPictureBounds(_ bounds: CGRect) {
    guard Self.isIntegrationEnabled, let pip = pipViewCoordinator else { return }
    pip.resetBounds(bounds: bounds)
  }
}

private class JitsiDelegate: NSObject, JitsiMeetViewDelegate {
  var conferenceWillJoin: () -> Void
  var conferenceJoined: () -> Void
  var conferenceTerminated: () -> Void
  var enterPiP: () -> Void
  var audioIsMuted: (Bool) -> Void
  var readyToClose: () -> Void
  
  init(
    conferenceWillJoin: @escaping () -> Void,
    conferenceJoined: @escaping () -> Void,
    conferenceTerminated: @escaping () -> Void,
    enterPictureInPicture: @escaping () -> Void,
    audioIsMuted: @escaping (Bool) -> Void,
    readyToClose: @escaping () -> Void
  ) {
    self.conferenceWillJoin = conferenceWillJoin
    self.conferenceJoined = conferenceJoined
    self.conferenceTerminated = conferenceTerminated
    self.enterPiP = enterPictureInPicture
    self.audioIsMuted = audioIsMuted
    self.readyToClose = readyToClose
  }
  
  func conferenceJoined(_ data: [AnyHashable: Any]!) {
    if let isMuted = data?["isAudioMuted"] as? Bool {
      audioIsMuted(isMuted)
    }
    conferenceJoined()
  }
  
  func conferenceWillJoin(_ data: [AnyHashable: Any]!) {
    conferenceWillJoin()
  }
  
  func conferenceTerminated(_ data: [AnyHashable: Any]!) {
    conferenceTerminated()
  }
  
  func enterPicture(inPicture data: [AnyHashable: Any]!) {
    enterPiP()
  }
  
  func audioMutedChanged(_ data: [AnyHashable: Any]!) {
    guard let isMuted = data?["muted"] as? Bool else { return }
    audioIsMuted(isMuted)
  }
  
  func ready(toClose data: [AnyHashable: Any]!) {
    readyToClose()
  }
}
