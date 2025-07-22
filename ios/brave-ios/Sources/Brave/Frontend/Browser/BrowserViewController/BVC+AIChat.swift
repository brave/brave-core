// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension BrowserViewController: AIChatCommunicationProtocol {
  public func handleVoiceRecognition(
    _ controller: AIChatCommunicationController,
    withConversationId conversationId: String
  ) {
    print("[AIChat] - HandleVoiceRecognition")
  }

  public func fetchImage(forChatUpload controller: AIChatCommunicationController) async -> URL? {
    print("[AIChat] - Fetch Image")
    return nil
  }

  public func openSettings(_ controller: AIChatCommunicationController) {
    print("[AIChat] - Open Settings")
  }

  public func openConversationFullPage(
    _ controller: AIChatCommunicationController,
    conversationId: String
  ) {
    print("[AIChat] - Open Conversation Full Screen")
  }

  public func openURL(_ controller: AIChatCommunicationController, url: URL) {
    print("[AIChat] - Open URL")
  }

  public func goPremium(_ controller: AIChatCommunicationController) {
    print("[AIChat] - Go Premium")
  }

  public func managePremium(_ controller: AIChatCommunicationController) {
    print("[AIChat] - Manage Premium")
  }
}
