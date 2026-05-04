/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/web_apps/frame_toolbar/web_app_toolbar_button_container.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/page_info/features.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_toolbar_button.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/extensions/extensions_toolbar_desktop.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "chrome/browser/ui/views/toolbar/pinned_toolbar_actions_container.h"
#include "chrome/browser/ui/views/toolbar/pinned_toolbar_button_status_indicator.h"
#include "chrome/browser/ui/views/toolbar/toolbar_action_view.h"
#include "chrome/browser/ui/views/web_apps/frame_toolbar/web_app_frame_toolbar_utils.h"
#include "chrome/browser/ui/views/web_apps/frame_toolbar/web_app_frame_toolbar_view.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
#include "ui/base/hit_test.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"
#include "ui/views/window/hit_test_utils.h"

namespace {

void MaybeAddPwaShieldsToolbarButton(WebAppToolbarButtonContainer* container) {
  CHECK(container);
  views::Widget* widget = container->GetWidget();
  if (!widget || !widget->GetNativeWindow()) {
    return;
  }

  BrowserView* base_browser_view =
      BrowserView::GetBrowserViewForNativeWindow(widget->GetNativeWindow());
  if (!base_browser_view) {
    return;
  }

  Browser* browser = base_browser_view->browser();
  if (!browser) {
    return;
  }

  CHECK(web_app::AppBrowserController::IsWebApp(browser));

  if (page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    // Page Info owns Shields; do not add a duplicate title-bar control.
    return;
  }
  if (BraveBrowserView::From(base_browser_view)->GetPwaShieldsToolbarButton()) {
    return;
  }

  auto* frame_toolbar =
      views::AsViewClass<WebAppFrameToolbarView>(container->parent());
  CHECK(frame_toolbar);

  size_t insert_index = 0;
  if (ExtensionsToolbarDesktop* ext = container->extensions_container()) {
    for (size_t i = 0; i < container->children().size(); ++i) {
      if (container->children()[i].get() == ext) {
        insert_index = i;
        break;
      }
    }
  } else if (PinnedToolbarActionsContainer* pinned =
                 container->pinned_toolbar_actions_container()) {
    for (size_t i = 0; i < container->children().size(); ++i) {
      if (container->children()[i].get() == pinned) {
        insert_index = i;
        break;
      }
    }
  }

  auto button = std::make_unique<BraveShieldsToolbarButton>(
      static_cast<BrowserWindowInterface*>(browser),
      base::BindRepeating(&WebUIBubbleManager::Create<ShieldsPanelUI>));
  ConfigureWebAppToolbarButton(button.get(), frame_toolbar);

  raw_ptr<BraveShieldsToolbarButton> ptr = button.get();
  container->AddChildViewAt(std::move(button), insert_index);
  views::SetHitTestComponent(ptr, static_cast<int>(HTCLIENT));
  ptr->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kPreferredSnapToZero)
          .WithWeight(0));
}

}  // namespace

#define AddedToWidget AddedToWidget_ChromiumImpl

#include <chrome/browser/ui/views/web_apps/frame_toolbar/web_app_toolbar_button_container.cc>

#undef AddedToWidget

void WebAppToolbarButtonContainer::AddedToWidget() {
  MaybeAddPwaShieldsToolbarButton(this);

  AddedToWidget_ChromiumImpl();
}
