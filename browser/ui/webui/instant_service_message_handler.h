// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_INSTANT_SERVICE_MESSAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_INSTANT_SERVICE_MESSAGE_HANDLER_H_

#include "base/values.h"
#include "chrome/browser/search/instant_service_observer.h"
#include "chrome/common/search/instant_types.h"
#include "content/public/browser/web_ui_message_handler.h"

class InstantService;
class Profile;
namespace content {
class WebUIDataSource;
}

// Handles messages from InstantService (used for top sites)
class InstantServiceMessageHandler : public content::WebUIMessageHandler,
                                     public InstantServiceObserver {
 public:
  explicit InstantServiceMessageHandler(Profile* profile,
      InstantService* instant_service);
  ~InstantServiceMessageHandler() override;

  static InstantServiceMessageHandler* Create(
      content::WebUIDataSource* html_source, Profile* profile,
      InstantService* instant_service);

 private:
  // WebUIMessageHandler:
  void RegisterMessages() override;

  // InstantServiceObserver:
  void MostVisitedInfoChanged(const InstantMostVisitedInfo& info) override;

  // handlers
  void HandleGetMostVisitedInfo(const base::ListValue* args);
  void HandleDeleteMostVisitedTile(const base::ListValue* args);
  void HandleReorderMostVisitedTile(const base::ListValue* args);
  void HandleRestoreMostVisitedDefaults(const base::ListValue* args);
  void HandleUndoMostVisitedTileAction(const base::ListValue* args);
  void HandleSetMostVisitedSettings(const base::ListValue* args);

  GURL last_blacklisted_;
  // Weak pointer.
  Profile* profile_;
  InstantService* instant_service_;
  base::Value top_site_tiles_;

  DISALLOW_COPY_AND_ASSIGN(InstantServiceMessageHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_INSTANT_SERVICE_MESSAGE_HANDLER_H_
