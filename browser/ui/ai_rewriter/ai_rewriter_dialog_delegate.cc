// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_rewriter/ai_rewriter_dialog_delegate.h"

#include <cstddef>
#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/process/kill.h"
#include "brave/browser/ui/webui/ai_rewriter/ai_rewriter_ui.h"
#include "brave/components/ai_rewriter/common/features.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/tab_contents/web_contents_collection.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
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
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
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

// static
AIRewriterDialogDelegate* AIRewriterDialogDelegate::Show(
    WebContents* contents) {
  DCHECK(features::IsAIRewriterEnabled());
  AIRewriterDialogDelegate* dialog = new AIRewriterDialogDelegate(contents);
  dialog->ShowDialog();
  return dialog;
}

AIRewriterDialogDelegate::AIRewriterDialogDelegate(
    content::WebContents* contents)
    : content::WebContentsObserver(contents),
      target_contents_(contents->GetWeakPtr()) {
  set_can_close(true);
  set_dialog_modal_type(ui::MODAL_TYPE_WINDOW);
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

void AIRewriterDialogDelegate::ShowDialog() {
  ConstrainedWebDialogDelegate* dialog_delegate =
      ShowConstrainedWebDialog(target_contents_->GetBrowserContext(),
                               base::WrapUnique(this), &*target_contents_);

  DCHECK(dialog_delegate);
  content::WebContents* dialog_contents = dialog_delegate->GetWebContents();
  DCHECK(dialog_contents);
  dialog_observer_ =
      std::make_unique<DialogContentsObserver>(dialog_contents, this);

  auto* widget = views::Widget::GetWidgetForNativeWindow(
      dialog_delegate->GetNativeDialog());

  mojo::Remote<mojom::AIRewriterAgent> agent;
  auto* frame = target_contents_->GetFocusedFrame();
  frame->GetRemoteInterfaces()->GetInterface(
      agent.BindNewPipeAndPassReceiver());
  auto* raw_agent = agent.get();
  raw_agent->GetFocusBounds(base::BindOnce(
      [](mojo::Remote<mojom::AIRewriterAgent> agent,
         base::WeakPtr<views::Widget> widget,
         content::WeakDocumentPtr document_pointer,
         const gfx::RectF& focus_rect) {
        auto* host = document_pointer.AsRenderFrameHostIfValid();
        if (!widget || !host) {
          return;
        }

        auto transformed = TransformFrameRectToView(host, focus_rect);
        auto widget_bounds = widget->GetWindowBoundsInScreen();

        widget_bounds.set_x(transformed.CenterPoint().x() -
                            widget_bounds.width() / 2);
        widget_bounds.set_y(transformed.bottom());
        widget->SetBounds(widget_bounds);
      },
      std::move(agent), widget->GetWeakPtr(), frame->GetWeakDocumentPtr()));
}

void AIRewriterDialogDelegate::CloseDialog() {
  auto* dialog_contents = GetDialogWebContents();
  if (!dialog_contents) {
    return;
  }

  auto* webui = dialog_contents->GetWebUI();
  if (!webui) {
    return;
  }

  auto* ui = webui->GetController()->GetAs<AIRewriterUI>();
  if (!ui) {
    return;
  }

  ui->Close();
}

content::WebContents* AIRewriterDialogDelegate::GetDialogWebContents() {
  return dialog_observer_.get() ? dialog_observer_->web_contents() : nullptr;
}

void AIRewriterDialogDelegate::ResetDialogObserver() {
  dialog_observer_.reset();
}

}  // namespace ai_rewriter
