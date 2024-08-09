// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_rewriter/ai_rewriter_dialog_delegate.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/process/kill.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/webui/ai_rewriter/ai_rewriter_ui.h"
#include "brave/components/ai_rewriter/common/features.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/tab_contents/web_contents_collection.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/web_modal/modal_dialog_host.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/focused_node_details.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/weak_document_ptr.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

using content::WebContents;

namespace ai_rewriter {

namespace {

gfx::RectF TransformFrameRectToView(content::RenderFrameHost* host,
                                    const gfx::RectF& bounding_box) {
  auto* view = host->GetView();
  if (!view) {
    return bounding_box;
  }

  gfx::PointF orig_point(bounding_box.x(), bounding_box.y());

  gfx::PointF transformed_point =
      view->TransformPointToRootCoordSpaceF(orig_point);
  return gfx::RectF(transformed_point.x(), transformed_point.y(),
                    bounding_box.width(), bounding_box.height());
}

}  // namespace

class AIRewriterDialogDelegate::DialogContentsObserver
    : public content::WebContentsObserver {
 public:
  DialogContentsObserver(content::WebContents* contents,
                         AIRewriterDialogDelegate* dialog)
      : content::WebContentsObserver(contents), dialog_(dialog) {}
  DialogContentsObserver(const DialogContentsObserver&) = delete;
  DialogContentsObserver& operator=(const DialogContentsObserver&) = delete;
  ~DialogContentsObserver() override = default;

 private:
  void WebContentsDestroyed() override { dialog_->ResetDialogObserver(); }

  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override {
    dialog_->CloseDialog();
  }

  const raw_ptr<AIRewriterDialogDelegate> dialog_;
};

class AIRewriterDialogDelegate::DialogPositioner
    : public views::WidgetObserver,
      public web_modal::ModalDialogHostObserver {
 public:
  DialogPositioner(content::WebContents* target_contents,
                   web_modal::WebContentsModalDialogHost* host,
                   views::Widget* dialog_widget)
      : top_(host->GetDialogPosition({}).y()),
        target_contents_(target_contents->GetWeakPtr()),
        dialog_widget_(dialog_widget->GetWeakPtr()) {
    // TODO(fallaciousreasoning): In a follow up PR we should handle reparenting
    // |target_contents_| into another browser. For now, we just assume it
    // remains in the same widget (which it won't necessarily).

    auto* browser = chrome::FindBrowserWithTab(target_contents);
    CHECK(browser);

    auto* host_widget = views::Widget::GetWidgetForNativeWindow(
        browser->window()->GetNativeWindow());
    CHECK(host_widget);

    host_widget_ = host_widget->GetWeakPtr();

    host_widget_observation_.Observe(host_widget);
    dialog_widget_observation_.Observe(dialog_widget);
    host_observation_.Observe(host);

    UpdatePosition(target_contents->GetFocusedFrame());
  }

  DialogPositioner(const DialogPositioner&) = delete;
  DialogPositioner& operator=(const DialogPositioner&) = delete;
  ~DialogPositioner() override = default;

  // web_modal::ModalDialogHostObserver:
  void OnPositionRequiresUpdate() override {
    if (!target_contents_) {
      return;
    }

    UpdatePosition(target_contents_->GetFocusedFrame());
    UpdateFocusedBounds();
  }

  void OnHostDestroying() override {
    host_widget_observation_.Reset();
    host_observation_.Reset();
    dialog_widget_observation_.Reset();
  }

  // views::WidgetObserver
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override {
    OnPositionRequiresUpdate();
  }

 private:
  void UpdateFocusedBounds() {
    if (!target_contents_) {
      return;
    }
    auto* browser = chrome::FindBrowserWithTab(target_contents_.get());
    if (!browser) {
      return;
    }

    mojo::Remote<mojom::AIRewriterAgent> agent;
    auto* frame = target_contents_->GetFocusedFrame();
    frame->GetRemoteInterfaces()->GetInterface(
        agent.BindNewPipeAndPassReceiver());
    auto* raw_agent = agent.get();
    raw_agent->GetFocusBounds(base::BindOnce(
        [](mojo::Remote<mojom::AIRewriterAgent> agent,
           base::WeakPtr<AIRewriterDialogDelegate::DialogPositioner> positioner,
           content::WeakDocumentPtr document_pointer,
           const gfx::RectF& focus_rect) {
          if (!positioner) {
            return;
          }
          positioner->last_bounds_ = focus_rect;
          positioner->UpdatePosition(
              document_pointer.AsRenderFrameHostIfValid());
        },
        std::move(agent), weak_ptr_factory_.GetWeakPtr(),
        frame->GetWeakDocumentPtr()));
  }

  void UpdatePosition(content::RenderFrameHost* rfh) {
    if (!rfh || !last_bounds_ || !dialog_widget_ || !host_widget_) {
      return;
    }

    auto transformed = TransformFrameRectToView(rfh, last_bounds_.value());

    auto dialog_bounds = dialog_widget_->GetWindowBoundsInScreen();
    dialog_bounds.set_x(transformed.CenterPoint().x() -
                        dialog_bounds.width() / 2);
    dialog_bounds.set_y(transformed.bottom() + top_);

    auto host_bounds = host_widget_->GetWindowBoundsInScreen();
    host_bounds.set_origin(gfx::Point());

    if (constrained_window::PlatformClipsChildrenToViewport() &&
        !host_bounds.Contains(dialog_bounds)) {
      dialog_bounds.AdjustToFit(host_bounds);
    }

    dialog_widget_->SetBounds(dialog_bounds);
  }

  // The offset for the top Chrome - this shouldn't change while the dialog is
  // open.
  int top_ = 0;

  // The last bounds we received from the |target_contents_| for the location of
  // the focused element.
  std::optional<gfx::RectF> last_bounds_;

  base::WeakPtr<content::WebContents> target_contents_;
  base::WeakPtr<views::Widget> dialog_widget_;
  base::WeakPtr<views::Widget> host_widget_;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      host_widget_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      dialog_widget_observation_{this};
  base::ScopedObservation<web_modal::ModalDialogHost,
                          web_modal::ModalDialogHostObserver>
      host_observation_{this};
  base::WeakPtrFactory<AIRewriterDialogDelegate::DialogPositioner>
      weak_ptr_factory_{this};
};

// static
AIRewriterDialogDelegate* AIRewriterDialogDelegate::Show(
    WebContents* contents,
    std::string initial_text) {
  DCHECK(features::IsAIRewriterEnabled());
  AIRewriterDialogDelegate* dialog = new AIRewriterDialogDelegate(contents);
  dialog->ShowDialog();

  if (auto* ui = dialog->GetRewriterUI()) {
    ui->set_initial_text(initial_text);
  }
  return dialog;
}

AIRewriterDialogDelegate::AIRewriterDialogDelegate(
    content::WebContents* contents)
    : content::WebContentsObserver(contents),
      target_contents_(contents->GetWeakPtr()) {
  set_can_close(true);
  set_dialog_modal_type(ui::mojom::ModalType::kWindow);
  set_dialog_content_url(GURL(kRewriterUIURL));
  set_dialog_size(gfx::Size(600, 550));
  set_dialog_args(*base::WriteJson(base::Value::Dict()));
  set_show_dialog_title(false);
  set_delete_on_close(false);
}

AIRewriterDialogDelegate::~AIRewriterDialogDelegate() = default;

void AIRewriterDialogDelegate::DidFinishNavigation(
    content::NavigationHandle* handle) {
  if (!handle->IsInPrimaryMainFrame() || !handle->HasCommitted()) {
    return;
  }

  CloseDialog();
}

void AIRewriterDialogDelegate::OnFocusChangedInPage(
    content::FocusedNodeDetails* focused_node) {
  CloseDialog();
}
void AIRewriterDialogDelegate::ShowDialog() {
  const gfx::Size min_size(600, 550);
  const gfx::Size max_size(600, 2000);
  ConstrainedWebDialogDelegate* dialog_delegate =
      ShowConstrainedWebDialogWithAutoResize(
          target_contents_->GetBrowserContext(), base::WrapUnique(this),
          &*target_contents_, min_size, max_size);

  DCHECK(dialog_delegate);
  content::WebContents* dialog_contents = dialog_delegate->GetWebContents();
  DCHECK(dialog_contents);
  dialog_observer_ =
      std::make_unique<DialogContentsObserver>(dialog_contents, this);

  auto* widget = views::Widget::GetWidgetForNativeWindow(
      dialog_delegate->GetNativeDialog());

  auto* manager = web_modal::WebContentsModalDialogManager::FromWebContents(
      target_contents_.get());
  CHECK(manager);
  auto* dialog_host = manager->delegate()->GetWebContentsModalDialogHost();
  CHECK(dialog_host);
  positioner_ = std::make_unique<DialogPositioner>(target_contents_.get(),
                                                   dialog_host, widget);

  widget_for_testing_ = widget;
}

void AIRewriterDialogDelegate::CloseDialog() {
  if (auto* ui = GetRewriterUI()) {
    ui->Close();
  }
}

content::WebContents* AIRewriterDialogDelegate::GetDialogWebContents() {
  return dialog_observer_.get() ? dialog_observer_->web_contents() : nullptr;
}

void AIRewriterDialogDelegate::ResetDialogObserver() {
  dialog_observer_.reset();
}

AIRewriterUI* AIRewriterDialogDelegate::GetRewriterUIForTesting() {
  return GetRewriterUI();
}

AIRewriterUI* AIRewriterDialogDelegate::GetRewriterUI() {
  auto* dialog_contents = GetDialogWebContents();
  if (!dialog_contents) {
    return nullptr;
  }

  auto* webui = dialog_contents->GetWebUI();
  if (!webui) {
    return nullptr;
  }

  return webui->GetController()->GetAs<AIRewriterUI>();
}

}  // namespace ai_rewriter
