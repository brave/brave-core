// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_AI_CHAT_CONVERSATION_DRIVER_IOS_H_
#define BRAVE_IOS_BROWSER_API_AI_CHAT_CONVERSATION_DRIVER_IOS_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/ai_chat/core/browser/conversation_driver.h"
#include "brave/components/ai_chat/core/browser/leo_local_models_updater.h"
#include "brave/ios/browser/skus/skus_service_factory.h"

@protocol AIChatDelegate;
class ChromeBrowserState;
class PrefService;

namespace ai_chat {
class AIChatMetrics;
class LeoLocalModelsUpdater;

class ConversationDriverIOS : public ConversationDriver,
                              ConversationDriver::Observer {
 public:
  ConversationDriverIOS(
      PrefService* profile_prefs,
      PrefService* local_state_prefs,
      ModelService* model_service,
      AIChatMetrics* ai_chat_metrics,
      LeoLocalModelsUpdater* leo_local_models_updater,
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::string_view channel_string,
      id<AIChatDelegate> delegate);
  ConversationDriverIOS(
      PrefService* profile_prefs,
      PrefService* local_state_prefs,
      ModelService* model_service,
      AIChatMetrics* ai_chat_metrics,
      LeoLocalModelsUpdater* leo_local_models_updater,
      std::unique_ptr<AIChatCredentialManager> credential_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::string_view channel_string,
      id<AIChatDelegate> delegate);
  ~ConversationDriverIOS() override;

 protected:
  std::u16string GetPageTitle() const override;
  GURL GetPageURL() const override;
  void GetPageContent(ConversationDriver::GetPageContentCallback callback,
                      std::string_view invalidation_token) override;
  void PrintPreviewFallback(
      ConversationDriver::GetPageContentCallback callback) override;

  // Observer
  void OnHistoryUpdate() override;
  void OnAPIRequestInProgress(bool in_progress) override;
  void OnAPIResponseError(ai_chat::mojom::APIError error) override;
  void OnModelDataChanged(
      const std::string& model_key,
      const std::vector<ai_chat::mojom::ModelPtr>& model_list) override;
  void OnSuggestedQuestionsChanged(
      std::vector<std::string> questions,
      ai_chat::mojom::SuggestionGenerationStatus status) override;
  void OnPageHasContent(ai_chat::mojom::SiteInfoPtr site_info) override;

 private:
  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
  GetSkusService(ChromeBrowserState* browser_state);

  __weak id<AIChatDelegate> bridge_;
  base::ScopedObservation<ConversationDriverIOS,
                          ConversationDriverIOS::Observer>
      chat_driver_observation_{this};
  base::WeakPtrFactory<ConversationDriverIOS> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_CONVERSATION_DRIVER_IOS_H_
