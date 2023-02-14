/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_CHAT_UI_CHAT_UI_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_CHAT_UI_CHAT_UI_SIDE_PANEL_COORDINATOR_H_

#include <memory>

#include "base/scoped_observation.h"
#include "chrome/browser/ui/browser_user_data.h"
#include "chrome/browser/ui/views/bubble/bubble_contents_wrapper.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class AIChatUI;
class Browser;
class SidePanelRegistry;

class ChatUISidePanelCoordinator
    : public BrowserUserData<ChatUISidePanelCoordinator>,
      public views::ViewObserver {
 public:
  explicit ChatUISidePanelCoordinator(Browser* browser);
  ChatUISidePanelCoordinator(const ChatUISidePanelCoordinator&) = delete;
  ChatUISidePanelCoordinator& operator=(const ChatUISidePanelCoordinator&) =
      delete;
  ~ChatUISidePanelCoordinator() override;

  void CreateAndRegisterEntry(SidePanelRegistry* global_registry);

  // views::ViewObserver:
  void OnViewIsDeleting(views::View* view) override;

 private:
  friend class BrowserUserData<ChatUISidePanelCoordinator>;

  void DestroyWebContentsIfNeeded();

  std::unique_ptr<views::View> CreateWebView();

  std::unique_ptr<BubbleContentsWrapperT<AIChatUI>> contents_wrapper_;

  base::ScopedObservation<views::View, views::ViewObserver> view_observation_{
      this};

  BROWSER_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_CHAT_UI_CHAT_UI_SIDE_PANEL_COORDINATOR_H_
