/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_SPEECH_TO_TEXT_NETWORK_DELEGATE_HELPER_H_
#define BRAVE_BROWSER_NET_BRAVE_SPEECH_TO_TEXT_NETWORK_DELEGATE_HELPER_H_

namespace network {
struct ResourceRequest;
}

namespace stt {

void OnBeforeURLRequest_SpoofSpeechToText(network::ResourceRequest* request);

}

#endif  // BRAVE_BROWSER_NET_BRAVE_SPEECH_TO_TEXT_NETWORK_DELEGATE_HELPER_H_
