/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/peek/peek_controller_impl.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/ui/peek/peek_utils.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"
#include "brave/browser/ui/views/peek/peek_contents.h"
#include "brave/browser/ui/views/peek/peek_view.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/frame_tree_node_id.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget.h"

// One Peek: its page, and the widget and card that show it over its source
// tab. Owned by the controller.
class PeekControllerImpl::Peek : public PeekContents::Delegate,
                                 public PeekView::Delegate {
 public:
  Peek(PeekControllerImpl& controller,
       tabs::TabInterface& source_tab,
       std::unique_ptr<content::WebContents> contents)
      : controller_(controller), source_tab_(source_tab) {
    controller_->SetShowingContents(contents.get(), true);
    contents_ = std::make_unique<PeekContents>(*controller_->browser_, *this,
                                               std::move(contents));
    CreateWidget();
    view_->SetWebContents(contents_->web_contents());
  }
  Peek(const Peek&) = delete;
  Peek& operator=(const Peek&) = delete;

  ~Peek() override {
    // Nothing posted for this Peek may run after this.
    weak_factory_.InvalidateWeakPtrs();
    view_->SetWebContents(nullptr);
    view_ = nullptr;
    // `contents_` must outlive `widget_`, which shows it.
    widget_.reset();
    paint_as_active_lock_.reset();
    contents_.reset();
  }

  tabs::TabInterface& source_tab() const { return *source_tab_; }
  content::WebContents* web_contents() const {
    return contents_->web_contents();
  }
  PeekView* view() const { return view_; }
  views::Widget* widget() const { return widget_.get(); }
  bool IsVisible() const { return widget_->IsVisible(); }

  void Show(bool animate) {
    if (IsVisible()) {
      return;
    }
    // Peek is part of the window, so the window keeps looking active while
    // Peek has focus.
    paint_as_active_lock_ =
        controller_->browser_view_->GetWidget()->LockPaintAsActive();
    UpdateBounds();
    if (animate) {
      view_->AnimateOpen();
    }
    widget_->Show();
    view_->FocusContents();
  }

  void UpdateBounds() {
    widget_->SetBounds(controller_->GetTargetBoundsInScreen());
  }

  // Runs `task` with this Peek, unless it has been destroyed since.
  void PostTask(base::OnceCallback<void(Peek&)> task) {
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&Peek::RunTask, weak_factory_.GetWeakPtr(),
                                  std::move(task)));
  }

  // Posts `method` on the controller for this Peek. The controller outlives
  // its Peeks.
  void PostControllerTask(void (PeekControllerImpl::*method)(Peek&)) {
    PostTask(base::BindOnce(method, base::Unretained(&*controller_)));
  }

  // PeekContents::Delegate:
  void OnPeekCloseRequested() override {
    PostControllerTask(&PeekControllerImpl::ClosePeek);
  }

  void OnPeekNavigationStateChanged() override { view_->UpdateToolbar(); }

  // PeekView::Delegate:
  void OnPeekViewCloseRequested() override {
    PostControllerTask(&PeekControllerImpl::ClosePeek);
  }

 private:
  void CreateWidget() {
    views::Widget::InitParams params(
        views::Widget::InitParams::CLIENT_OWNS_WIDGET,
        views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
    params.parent = controller_->browser_view_->GetWidget()->GetNativeView();
    params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
    params.activatable = views::Widget::InitParams::Activatable::kYes;
    params.bounds = controller_->GetTargetBoundsInScreen();
    params.name = "PeekWidget";

    widget_ = std::make_unique<views::Widget>();
    widget_->Init(std::move(params));
    widget_->MakeCloseSynchronous(
        base::BindOnce(&Peek::OnWidgetClosed, weak_factory_.GetWeakPtr()));
    view_ = widget_->SetContentsView(
        std::make_unique<PeekView>(*this, *controller_->browser_));
  }

  void RunTask(base::OnceCallback<void(Peek&)> task) {
    std::move(task).Run(*this);
  }

  void OnWidgetClosed(views::Widget::ClosedReason reason) {
    PostControllerTask(&PeekControllerImpl::ClosePeek);
  }

  const raw_ref<PeekControllerImpl> controller_;
  const raw_ref<tabs::TabInterface> source_tab_;
  std::unique_ptr<PeekContents> contents_;
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<PeekView> view_ = nullptr;
  std::unique_ptr<views::Widget::PaintAsActiveLock> paint_as_active_lock_;
  base::WeakPtrFactory<Peek> weak_factory_{this};
};

PeekControllerImpl::PeekControllerImpl(BrowserWindowInterface& browser,
                                       BrowserView& browser_view)
    : PeekController(browser),
      browser_(browser),
      browser_view_(browser_view) {
  browser_->GetTabStripModel()->AddObserver(this);
  contents_area_observation_.Observe(browser_view_->contents_container());
  browser_widget_observation_.Observe(browser_view_->GetWidget());
}

PeekControllerImpl::~PeekControllerImpl() {
  peeks_.clear();
}

PeekView* PeekControllerImpl::peek_view_for_testing() {
  Peek* peek = GetActivePeek();
  return peek ? peek->view() : nullptr;
}

views::Widget* PeekControllerImpl::widget_for_testing() {
  Peek* peek = GetActivePeek();
  return peek ? peek->widget() : nullptr;
}

bool PeekControllerImpl::CanOpenFrom(content::WebContents* source) {
  // This also rules out the Peek contents itself, so Peeks don't nest.
  tabs::TabInterface* tab = tabs::TabInterface::MaybeGetFromContents(source);
  if (!tab || tab->GetBrowserWindowInterface() != &*browser_) {
    return false;
  }

  // Sidebar web panels are tabs too, but shown outside the tab's area.
  if (auto* sidebar_controller = sidebar::SidebarController::From(&*browser_);
      sidebar_controller && sidebar::IsWebPanelFeatureEnabled()) {
    if (auto* web_panel_controller =
            sidebar_controller->GetWebPanelController();
        web_panel_controller &&
        web_panel_controller->panel_contents() == source) {
      return false;
    }
  }
  return true;
}

bool PeekControllerImpl::Open(content::WebContents& source,
                              const content::OpenURLParams& params) {
  if (!CanOpenFrom(&source) || !peek::CanOpenInPeek(params)) {
    return false;
  }
  tabs::TabInterface& source_tab =
      CHECK_DEREF(tabs::TabInterface::GetFromContents(&source));

  std::unique_ptr<content::WebContents> contents =
      PeekContents::CreateWebContents(browser_->GetProfile(), params.url,
                                      source);
  if (!contents) {
    return false;
  }
  content::WebContents* raw_contents = contents.get();

  // One Peek at a time, so opening another replaces it.
  peeks_.clear();
  Peek& peek = *peeks_.emplace_back(
      std::make_unique<Peek>(*this, source_tab, std::move(contents)));
  if (source_tab.IsActivated()) {
    peek.Show(/*animate=*/true);
  }

  // Keeps the initiator origin, so SameSite cookies and fetch metadata treat
  // this like a link click rather than a browser-initiated navigation.
  content::NavigationController::LoadURLParams load_params(params);
  // The source frame lives in another WebContents.
  load_params.frame_tree_node_id = content::FrameTreeNodeId();
  raw_contents->GetController().LoadURLWithParams(load_params);
  return true;
}

void PeekControllerImpl::Close() {
  if (Peek* peek = GetActivePeek()) {
    ClosePeek(*peek);
  }
}

bool PeekControllerImpl::IsShowing() const {
  return !!GetActivePeek();
}

content::WebContents* PeekControllerImpl::GetWebContents() {
  Peek* peek = GetActivePeek();
  return peek ? peek->web_contents() : nullptr;
}

void PeekControllerImpl::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    // A Peek goes when its tab is deactivated.
    peeks_.clear();
  }
}

void PeekControllerImpl::OnViewBoundsChanged(views::View* observed_view) {
  if (Peek* peek = GetActivePeek()) {
    peek->UpdateBounds();
  }
}

void PeekControllerImpl::OnViewIsDeleting(views::View* observed_view) {
  contents_area_observation_.Reset();
  peeks_.clear();
}

void PeekControllerImpl::OnWidgetBoundsChanged(views::Widget* widget,
                                               const gfx::Rect& new_bounds) {
  if (Peek* peek = GetActivePeek()) {
    peek->UpdateBounds();
  }
}

PeekControllerImpl::Peek* PeekControllerImpl::GetActivePeek() const {
  tabs::TabInterface* active_tab = browser_->GetTabStripModel()->GetActiveTab();
  return active_tab ? FindPeek(*active_tab) : nullptr;
}

PeekControllerImpl::Peek* PeekControllerImpl::FindPeek(
    const tabs::TabInterface& source_tab) const {
  for (const auto& peek : peeks_) {
    if (&peek->source_tab() == &source_tab) {
      return peek.get();
    }
  }
  return nullptr;
}

PeekControllerImpl::Peek* PeekControllerImpl::FindPeek(
    const content::WebContents& peek_contents) const {
  for (const auto& peek : peeks_) {
    if (peek->web_contents() == &peek_contents) {
      return peek.get();
    }
  }
  return nullptr;
}

void PeekControllerImpl::ClosePeek(Peek& peek) {
  std::erase_if(peeks_, [&](const auto& p) { return p.get() == &peek; });
}

gfx::Rect PeekControllerImpl::GetTargetBoundsInScreen() const {
  // The contents container starts right below the toolbar (or the bookmarks
  // bar), so the card hangs from the browser UI.
  return browser_view_->contents_container()->GetBoundsInScreen();
}
