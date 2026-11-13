/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/bubble/download_toolbar_ui_controller.h"

#include "base/callback_list.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/download/bubble/download_bubble_update_service.h"
#include "chrome/browser/download/bubble/download_bubble_update_service_factory.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/interaction/element_tracker.h"
#include "ui/views/interaction/element_tracker_views.h"

namespace {

SkColor GetIconColor(DownloadDisplay::IconState state,
                     DownloadDisplay::IconActive active,
                     const ui::ColorProvider* color_provider) {
  // Apply active color only when download is completed and user doesn't
  // interact with this button.
  if (state == DownloadDisplay::IconState::kComplete &&
      active == DownloadDisplay::IconActive::kActive) {
    return color_provider->GetColor(kColorDownloadToolbarButtonActive);
  }

  // Otherwise, always use inactive color.
  return color_provider->GetColor(kColorDownloadToolbarButtonInactive);
}

bool HasInsecureDownloads(BrowserView* browser_view) {
  auto* update_service = DownloadBubbleUpdateServiceFactory::GetForProfile(
      browser_view->GetProfile());
  if (!update_service || !update_service->IsInitialized()) {
    return false;
  }

  std::vector<DownloadUIModel::DownloadUIModelPtr> all_models;
  update_service->GetAllModelsToDisplay(all_models, /*web_app_id=*/nullptr,
                                        /*force_backfill_download_items=*/true);

  return std::ranges::any_of(all_models, [](const auto& model) {
    return (model->GetInsecureDownloadStatus() ==
                download::DownloadItem::InsecureDownloadStatus::BLOCK ||
            model->GetInsecureDownloadStatus() ==
                download::DownloadItem::InsecureDownloadStatus::WARN);
  });
}

void UpdateIcon_BraveImpl(BrowserView* browser_view,
                          const gfx::VectorIcon& icon,
                          DownloadDisplay::IconState state,
                          DownloadDisplay::IconActive active,
                          actions::ActionItem* action_item);

void ObserveDownloadButtonShown_BraveImpl(
    base::WeakPtr<DownloadToolbarUIController> controller,
    const raw_ptr<BrowserView>& browser_view,
    base::CallbackListSubscription& subscription);

}  // namespace

#include <chrome/browser/ui/views/download/bubble/download_toolbar_ui_controller.cc>

namespace {

void ObserveDownloadButtonShown_BraveImpl(
    base::WeakPtr<DownloadToolbarUIController> controller,
    const raw_ptr<BrowserView>& browser_view,
    base::CallbackListSubscription& subscription) {
  // The browser view may not be in its widget yet, so match the context on
  // each show. `browser_view` is the controller's own, read only while
  // `controller` is alive, and cleared ahead of the window's destruction.
  subscription =
      ui::ElementTracker::GetElementTracker()
          ->AddElementShownInAnyContextCallback(
              kToolbarDownloadButtonElementId,
              base::BindRepeating(
                  [](base::WeakPtr<DownloadToolbarUIController> controller,
                     const raw_ptr<BrowserView>* browser_view,
                     ui::TrackedElement* element) {
                    if (!controller || !*browser_view ||
                        element->context() !=
                            views::ElementTrackerViews::GetContextForView(
                                *browser_view)) {
                      return;
                    }
                    // The toolbar is still adding the button, which
                    // `UpdateIcon()` wouldn't find yet.
                    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
                        FROM_HERE,
                        base::BindOnce(&DownloadToolbarUIController::UpdateIcon,
                                       controller));
                  },
                  controller, base::Unretained(&browser_view)));
}

void UpdateIcon_BraveImpl(BrowserView* browser_view,
                          const gfx::VectorIcon& icon,
                          DownloadDisplay::IconState state,
                          DownloadDisplay::IconActive active,
                          actions::ActionItem* action_item) {
  // Only a shown button is found, which is why a newly shown one updates the
  // icon again.
  auto* button =
      views::ElementTrackerViews::GetInstance()
          ->GetFirstMatchingViewAs<ToolbarButton>(
              kToolbarDownloadButtonElementId,
              views::ElementTrackerViews::GetContextForView(browser_view));

  // Use an exclamation point icon while there's an insecure download in the
  // download models.
  if (HasInsecureDownloads(browser_view)) {
    auto icon_color = browser_view->GetColorProvider()->GetColor(
        ui::kColorAlertMediumSeverityIcon);
    if (button) {
      button->SetIconEnabledColorsOverride(icon_color);
      button->SetVectorIcon(vector_icons::kNotSecureWarningOldIcon);
    }
    const int icon_size = action_item->GetImage().Size().height();
    action_item->SetImage(ui::ImageModel::FromVectorIcon(
        vector_icons::kNotSecureWarningOldIcon, icon_color, icon_size));
  } else {
    if (button) {
      button->SetIconEnabledColorsOverride(std::nullopt);
    }
    action_item->SetImage(ui::ImageModel::FromVectorIcon(
        icon, GetIconColor(state, active, browser_view->GetColorProvider())));
  }
}

}  // namespace
