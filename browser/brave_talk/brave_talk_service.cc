/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_talk/brave_talk_service.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/notreached.h"
#include "brave/browser/brave_talk/brave_talk_service_factory.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry_factory.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/confirm_bubble.h"
#include "chrome/browser/ui/confirm_bubble_model.h"
#include "chrome/browser/ui/views/confirm_bubble_views.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/point.h"

namespace brave_talk {

class BraveTalkConfirmBubbleModel : public ConfirmBubbleModel {
 public:
  explicit BraveTalkConfirmBubbleModel(content::WebContents* target_contents)
      : target_contents_(target_contents->GetWeakPtr()) {}

  BraveTalkConfirmBubbleModel(const BraveTalkConfirmBubbleModel&) = delete;
  BraveTalkConfirmBubbleModel& operator=(const BraveTalkConfirmBubbleModel&) =
      delete;

  ~BraveTalkConfirmBubbleModel() override = default;

  std::u16string GetTitle() const override {
    return brave_l10n::GetLocalizedResourceUTF16String(
        IDS_BRAVE_TALK_SHARE_TAB_CONFIRM_PROMPT_TITLE);
  }

  std::u16string GetMessageText() const override {
    return brave_l10n::GetLocalizedResourceUTF16String(
        IDS_BRAVE_TALK_SHARE_TAB_CONFIRM_PROMPT_MESSAGE);
  }

  void Accept() override {
    if (!target_contents_)
      return;

    auto* service = BraveTalkServiceFactory::GetForContext(
        target_contents_->GetBrowserContext());
    service->ShareTab(target_contents_.get());
  }

  void Cancel() override {}

 private:
  base::WeakPtr<content::WebContents> target_contents_;
};

BraveTalkService::BraveTalkService() = default;
BraveTalkService::~BraveTalkService() {
  StopObserving();
  Shutdown();
}

void BraveTalkService::GetDeviceID(
    content::WebContents* contents,
    int owning_process_id,
    int owning_frame_id,
    base::OnceCallback<void(const std::string&)> callback) {
  StartObserving(contents);

  owning_render_frame_id_ = owning_frame_id;
  owning_render_process_id_ = owning_process_id;
  on_received_device_id_ = std::move(callback);

  if (on_get_device_id_requested_for_testing_)
    std::move(on_get_device_id_requested_for_testing_).Run();

  NotifyObservers();
}

void BraveTalkService::PromptShareTab(content::WebContents* target_contents) {
  if (!web_contents() || !target_contents || !is_requesting_tab())
    return;

  auto* rvh = target_contents->GetRenderViewHost()->GetWidget()->GetView();
  auto rect = rvh->GetViewBounds();
  auto anchor = gfx::Point(rect.CenterPoint().x(), rect.y());
  auto confirm_bubble =
      std::make_unique<BraveTalkConfirmBubbleModel>(target_contents);
  chrome::ShowConfirmBubble(target_contents->GetTopLevelNativeWindow(),
                            rvh->GetNativeView(), anchor,
                            std::move(confirm_bubble));
}

void BraveTalkService::ShareTab(content::WebContents* target_contents) {
  if (!web_contents() || !target_contents || !is_requesting_tab())
    return;
  auto* registry = BraveTalkTabCaptureRegistryFactory::GetForContext(
      target_contents->GetBrowserContext());

  auto* owning_render_frame = content::RenderFrameHost::FromID(
      owning_render_process_id_, owning_render_frame_id_);
  std::string device_id =
      owning_render_frame
          ? registry->AddRequest(target_contents, owning_render_frame)
          : "";
  if (on_received_device_id_)
    std::move(on_received_device_id_).Run(device_id);

  NotifyObservers();
}

void BraveTalkService::AddObserver(BraveTalkServiceObserver* observer) {
  observers_.push_back(observer);
}

void BraveTalkService::RemoveObserver(BraveTalkServiceObserver* observer) {
  for (auto it = observers_.begin(); it != observers_.end(); ++it) {
    if (*it == observer) {
      observers_.erase(it);
      return;
    }
  }
  NOTREACHED();
}

void BraveTalkService::DidStartNavigation(content::NavigationHandle* handle) {
  if (!handle->IsInMainFrame())
    return;

  // On any navigation of the main frame stop observing the web contents.
  StopObserving();
}

void BraveTalkService::OnGetDeviceIDRequestedForTesting(
    base::OnceCallback<void()> callback_for_testing) {
  on_get_device_id_requested_for_testing_ = std::move(callback_for_testing);
}

void BraveTalkService::StartObserving(content::WebContents* contents) {
  if (web_contents())
    StopObserving();

  Observe(contents);
}

void BraveTalkService::StopObserving() {
  if (on_received_device_id_)
    std::move(on_received_device_id_).Run("");

  Observe(nullptr);
}

void BraveTalkService::NotifyObservers() {
  for (auto* observer : observers_) {
    observer->OnIsRequestingChanged(is_requesting_tab());
  }
}

}  // namespace brave_talk
