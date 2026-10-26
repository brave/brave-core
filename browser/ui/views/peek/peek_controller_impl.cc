/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/peek/peek_controller_impl.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/ui/peek/peek_utils.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/frame_tree_node_id.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/focus/focus_manager.h"

PeekControllerImpl::PeekControllerImpl(BrowserWindowInterface& browser,
                                       BrowserView& browser_view)
    : PeekController(browser), browser_(browser), browser_view_(browser_view) {
  browser_->GetTabStripModel()->AddObserver(this);
}

PeekControllerImpl::~PeekControllerImpl() {
  Close();
}

void PeekControllerImpl::SetShieldsPanelProvider(
    std::unique_ptr<PeekShieldsPanelProvider> provider) {
  CHECK(!IsShowing());
  shields_panel_provider_ = std::move(provider);
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

  std::unique_ptr<content::WebContents> contents =
      PeekContents::CreateWebContents(browser_->GetProfile(), params.url,
                                      source);
  content::WebContents* raw_contents = contents.get();
  Show(std::move(contents));

  // Keeps the initiator origin, so SameSite cookies and fetch metadata treat
  // this like a link click rather than a browser-initiated navigation.
  content::NavigationController::LoadURLParams load_params(params);
  // The source frame lives in another WebContents.
  load_params.frame_tree_node_id = content::FrameTreeNodeId();
  raw_contents->GetController().LoadURLWithParams(load_params);
  return true;
}

void PeekControllerImpl::OpenAsTab() {
  if (!contents_) {
    return;
  }

  std::unique_ptr<content::WebContents> contents =
      contents_->ReleaseWebContents();
  content::WebContents* raw_contents = contents.get();
  SetShowingContents(raw_contents, false);
  Close();

  TabStripModel* tab_strip_model = browser_->GetTabStripModel();
  tab_strip_model->InsertWebContentsAt(
      tab_strip_model->active_index() + 1, std::move(contents),
      AddTabTypes::ADD_ACTIVE | AddTabTypes::ADD_INHERIT_OPENER);
  PeekContents::OnMovedToTab(*raw_contents);
}

void PeekControllerImpl::Close() {
  contents_area_observation_.Reset();
  browser_widget_observation_.Reset();
  if (peek_view_) {
    peek_view_->SetWebContents(nullptr);
    peek_view_ = nullptr;
  }
  widget_.reset();
  contents_.reset();
}

bool PeekControllerImpl::IsShowing() const {
  return !!widget_;
}

content::WebContents* PeekControllerImpl::GetWebContents() {
  return contents_ ? contents_->web_contents() : nullptr;
}

void PeekControllerImpl::OnPeekCloseRequested() {
  PostTask(&PeekControllerImpl::Close);
}

void PeekControllerImpl::OnPeekNavigationStateChanged() {
  if (peek_view_) {
    peek_view_->UpdateToolbar();
  }
}

void PeekControllerImpl::OnPeekMoveToTabRequired() {
  OpenAsTab();
}

void PeekControllerImpl::OnPeekMoveToTabOrCloseRequested() {
  PostTask(&PeekControllerImpl::OpenAsTabOrClose);
}

bool PeekControllerImpl::OnPeekTakeFocus(bool reverse) {
  // Use the Peek widget's focus manager, not the browser's, so focus cycles
  // between the card's toolbar and the page.
  views::FocusManager* focus_manager =
      peek_view_ ? peek_view_->GetFocusManager() : nullptr;
  if (!focus_manager) {
    return false;
  }
  focus_manager->AdvanceFocus(reverse);
  return true;
}

void PeekControllerImpl::OnPeekViewCloseRequested() {
  PostTask(&PeekControllerImpl::Close);
}

void PeekControllerImpl::OnPeekViewOpenAsTabRequested() {
  PostTask(&PeekControllerImpl::OpenAsTab);
}

void PeekControllerImpl::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    Close();
  }
}

void PeekControllerImpl::OnViewBoundsChanged(views::View* observed_view) {
  UpdateWidgetBounds();
}

void PeekControllerImpl::OnViewIsDeleting(views::View* observed_view) {
  Close();
}

void PeekControllerImpl::OnWidgetBoundsChanged(views::Widget* widget,
                                               const gfx::Rect& new_bounds) {
  UpdateWidgetBounds();
}

void PeekControllerImpl::Show(std::unique_ptr<content::WebContents> contents) {
  Close();

  SetShowingContents(contents.get(), true);
  contents_ =
      std::make_unique<PeekContents>(*browser_, *this, std::move(contents));
  CreateWidget();
  peek_view_->SetWebContents(contents_->web_contents());
  peek_view_->AnimateOpen();
  widget_->Show();
  peek_view_->FocusContents();
}

void PeekControllerImpl::OpenAsTabOrClose() {
  content::WebContents* contents = GetWebContents();
  if (!contents) {
    return;
  }
  // E.g. a link that's a download: there's no page to keep.
  content::NavigationEntry* entry =
      contents->GetController().GetLastCommittedEntry();
  if (!entry || entry->IsInitialEntry()) {
    Close();
    return;
  }
  OpenAsTab();
}

void PeekControllerImpl::CreateWidget() {
  CHECK(shields_panel_provider_);
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.parent = browser_view_->GetWidget()->GetNativeView();
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.activatable = views::Widget::InitParams::Activatable::kYes;
  params.bounds = GetTargetBoundsInScreen();
  params.name = "PeekWidget";

  widget_ = std::make_unique<views::Widget>();
  widget_->Init(std::move(params));
  widget_->MakeCloseSynchronous(base::BindOnce(
      &PeekControllerImpl::OnWidgetCloseRequested, weak_factory_.GetWeakPtr()));
  peek_view_ = widget_->SetContentsView(
      std::make_unique<PeekView>(*this, *browser_, *shields_panel_provider_));

  contents_area_observation_.Observe(browser_view_->contents_container());
  browser_widget_observation_.Observe(browser_view_->GetWidget());
}

void PeekControllerImpl::OnWidgetCloseRequested(
    views::Widget::ClosedReason reason) {
  PostTask(&PeekControllerImpl::Close);
}

void PeekControllerImpl::UpdateWidgetBounds() {
  if (widget_) {
    widget_->SetBounds(GetTargetBoundsInScreen());
  }
}

gfx::Rect PeekControllerImpl::GetTargetBoundsInScreen() const {
  // The contents container starts right below the toolbar (or the bookmarks
  // bar), so the card hangs from the browser UI.
  return browser_view_->contents_container()->GetBoundsInScreen();
}

void PeekControllerImpl::PostTask(void (PeekControllerImpl::*method)()) {
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(method, weak_factory_.GetWeakPtr()));
}
