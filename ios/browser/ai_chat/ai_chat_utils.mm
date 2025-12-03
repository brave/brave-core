// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ai_chat/ai_chat_utils.h"

#include "base/apple/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/api/profile/profile_bridge.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "brave/ios/components/prefs/pref_service_bridge_impl.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "net/base/apple/url_conversions.h"

@implementation AIChatUtils

+ (BOOL)isAIChatEnabledForPrefService:(id<PrefServiceBridge>)prefService {
  PrefServiceBridgeImpl* holder =
      base::apple::ObjCCastStrict<PrefServiceBridgeImpl>(prefService);
  return ai_chat::IsAIChatEnabled(holder.prefService);
}

+ (NSURL*)openLeoURLWithQuerySubmitted:(NSString*)query
                               profile:(id<ProfileBridge>)profileBridge {
  // There is no way to currently bridge `ai_chat::ConversationUrl` nor a way
  // to get a `ConversationHandler` from mojo APIs, so this method will use the
  // public methods directly exposed on `AIChatService`
  ProfileIOS* profile =
      base::apple::ObjCCastStrict<ProfileBridgeImpl>(profileBridge).profile;
  ai_chat::AIChatService* service =
      ai_chat::AIChatServiceFactory::GetForProfile(profile);
  if (!service) {
    return nil;
  }
  ai_chat::ConversationHandler* handler = service->CreateConversation();
  ai_chat::mojom::ConversationTurnPtr turn =
      ai_chat::mojom::ConversationTurn::New(
          std::nullopt, ai_chat::mojom::CharacterType::HUMAN,
          ai_chat::mojom::ActionType::QUERY,
          base::SysNSStringToUTF8(query) /* text */, std::nullopt /* prompt */,
          std::nullopt /* selected_text */, std::nullopt /* events */,
          base::Time::Now(), std::nullopt /* edits */,
          std::nullopt /* uploaded images */, nullptr /* skill */,
          false /* from_brave_search_SERP */, std::nullopt /* model_key */,
          nullptr /* near_verification_status */);
  handler->SubmitHumanConversationEntry(std::move(turn));
  return net::NSURLWithGURL(
      ai_chat::ConversationUrl(handler->get_conversation_uuid()));
}

@end
