/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/workspaces_bubble_view.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/ui/views/workspaces/workspace_row_view.h"
#include "brave/browser/workspaces/workspace_service.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/dialog_model.h"
#include "ui/base/models/dialog_model_field.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

WorkspacesBubbleView::~WorkspacesBubbleView() = default;

WorkspacesBubbleView::WorkspacesBubbleView(views::View* anchor_view,
                                           BrowserWindowInterface* browser,
                                           ShowDialogCallback on_show_dialog)
    : BubbleDialogDelegate(anchor_view, views::BubbleBorder::TOP_LEFT),
      browser_(browser),
      on_show_dialog_(std::move(on_show_dialog)) {
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  SetShowCloseButton(false);
  set_margins(gfx::Insets());
  set_fixed_width(263);
  set_internal_name(kWidgetName);

  auto contents = std::make_unique<views::View>();
  contents->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      /*inside_border_insets=*/gfx::Insets(),
      /*between_child_spacing=*/0));

  auto* service = WorkspaceServiceFactory::GetForProfile(browser->GetProfile());
  CHECK(service);

  const SessionID window_id = browser->GetSessionID();
  showing_workspace_ = service->GetWorkspaceForWindow(window_id);
  const std::optional<base::Uuid>& showing = showing_workspace_;

  auto* workspaces_container =
      contents->AddChildView(std::make_unique<views::View>());
  workspaces_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  workspaces_container->SetProperty(views::kMarginsKey, gfx::Insets(2));

  // The window a person started in leads the list, separated from the
  // workspaces the whole profile shares. It is hidden rather than emptied when
  // they switch away, so its tab count comes straight off its tab strip.
  BrowserWindowInterface* unmanaged_window = browser;
  if (std::optional<SessionID> origin =
          service->GetUnmanagedWindowFor(window_id)) {
    unmanaged_window = BrowserWindowInterface::FromSessionID(*origin);
  }

  if (unmanaged_window) {
    WorkspaceMetadata unmanaged_space{
        .name = l10n_util::GetStringUTF8(IDS_WORKSPACE_UNMANAGED_SPACE_NAME),
        .number_of_tabs = unmanaged_window->GetTabStripModel()->count()};
    auto* unmanaged_row =
        workspaces_container->AddChildView(std::make_unique<WorkspaceRowView>(
            unmanaged_space,
            base::BindRepeating(&WorkspacesBubbleView::OnUnmanagedSelected,
                                weak_factory_.GetWeakPtr()),
            // The unmanaged space is implicit: no menu to delete it.
            base::RepeatingClosure()));
    unmanaged_row->SetSelected(!showing.has_value());

    workspaces_container->AddChildView(std::make_unique<views::Separator>());
  }

  std::vector<WorkspaceMetadata> workspaces = service->ListWorkspaces();
  if (workspaces.empty()) {
    auto* empty_title =
        workspaces_container->AddChildView(std::make_unique<views::Label>(
            l10n_util::GetStringUTF16(IDS_WORKSPACES_BUBBLE_EMPTY_TITLE)));
    const gfx::FontList& base_font = empty_title->font_list();
    empty_title->SetFontList(
        base_font.Derive(kTitleFontSize - base_font.GetFontSize(),
                         base_font.GetFontStyle(), gfx::Font::Weight::BOLD));
    empty_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    empty_title->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(8, 8, 4, 8));

    auto* empty_body =
        workspaces_container->AddChildView(std::make_unique<views::Label>(
            l10n_util::GetStringUTF16(IDS_WORKSPACES_BUBBLE_EMPTY_BODY)));
    empty_body->SetMultiLine(true);
    empty_body->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    empty_body->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 8, 8, 8));
  } else {
    for (const auto& workspace : workspaces) {
      auto* row =
          workspaces_container->AddChildView(std::make_unique<WorkspaceRowView>(
              workspace,
              base::BindRepeating(&WorkspacesBubbleView::OnWorkspaceSelected,
                                  weak_factory_.GetWeakPtr(), workspace.id),
              base::BindRepeating(&WorkspacesBubbleView::OnDeleteClicked,
                                  weak_factory_.GetWeakPtr(), workspace.id)));
      row->SetSelected(showing == workspace.id);

      // Only a workspace someone is currently looking at is out of reach.
      // One that is open but hidden is picked up by bringing its window
      // forward, so switching away from a workspace releases it.
      row->SetAvailable(
          !service->IsWorkspaceOnScreenElsewhere(workspace.id, window_id));
    }
  }

  contents->AddChildView(std::make_unique<views::Separator>());

  // Bottom action row: Settings + New Space.
  auto* button_row = contents->AddChildView(std::make_unique<views::View>());
  auto* button_row_layout =
      button_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          /*inside_border_insets=*/gfx::Insets(),
          /*between_child_spacing=*/0));
  button_row->SetProperty(views::kMarginsKey, gfx::Insets(8));

  auto* settings_button =
      button_row->AddChildView(std::make_unique<views::MdTextButton>(
          base::BindRepeating(&WorkspacesBubbleView::OnSettingsClicked,
                              weak_factory_.GetWeakPtr()),
          l10n_util::GetStringUTF16(IDS_WORKSPACES_BUBBLE_SETTINGS_BUTTON)));
  // Settings edits the workspace this window is showing, so it has nothing to
  // act on while the window is unmanaged.
  settings_button->SetEnabled(showing_workspace_.has_value());
  settings_button->SetProperty(views::kMarginsKey, gfx::Insets(4));
  button_row_layout->SetFlexForView(settings_button, 1);

  auto* new_space_button =
      button_row->AddChildView(std::make_unique<views::MdTextButton>(
          base::BindRepeating(&WorkspacesBubbleView::OnNewSpaceClicked,
                              weak_factory_.GetWeakPtr()),
          l10n_util::GetStringUTF16(IDS_WORKSPACES_BUBBLE_NEW_SPACE_BUTTON)));
  new_space_button->SetStyle(ui::ButtonStyle::kProminent);
  new_space_button->SetProperty(views::kMarginsKey, gfx::Insets(4));
  button_row_layout->SetFlexForView(new_space_button, 1);

  SetContentsView(std::move(contents));
}

void WorkspacesBubbleView::OnNewSpaceClicked() {
  if (on_show_dialog_) {
    on_show_dialog_.Run(std::nullopt);
  }
  GetWidget()->Close();
}

void WorkspacesBubbleView::OnSettingsClicked() {
  // The button is disabled while the window is unmanaged.
  CHECK(showing_workspace_.has_value());
  if (on_show_dialog_) {
    on_show_dialog_.Run(showing_workspace_);
  }
  GetWidget()->Close();
}

void WorkspacesBubbleView::OnUnmanagedSelected() {
  auto* service =
      WorkspaceServiceFactory::GetForProfile(browser_->GetProfile());
  CHECK(service);
  service->SwitchWindowToUnmanaged(browser_);
  GetWidget()->Close();
}

void WorkspacesBubbleView::OnWorkspaceSelected(const base::Uuid& id) {
  auto* service =
      WorkspaceServiceFactory::GetForProfile(browser_->GetProfile());
  CHECK(service);
  service->SwitchWindowToWorkspace(id, browser_);
  GetWidget()->Close();
}

void WorkspacesBubbleView::OnDeleteClicked(const base::Uuid& id) {
  auto* service =
      WorkspaceServiceFactory::GetForProfile(browser_->GetProfile());
  CHECK(service);

  // The bubble closes once the modal confirmation appears; binding the
  // service's WeakPtr directly to the member function makes the delete
  // independent of the bubble's lifetime and gets auto-cancellation for free.
  auto dialog =
      ui::DialogModel::Builder()
          .SetTitle(
              l10n_util::GetStringUTF16(IDS_WORKSPACE_DELETE_CONFIRM_TITLE))
          .AddParagraph(ui::DialogModelLabel(
              l10n_util::GetStringUTF16(IDS_WORKSPACE_DELETE_CONFIRM_BODY)))
          .AddOkButton(base::BindOnce(&WorkspaceService::DeleteWorkspace,
                                      service->GetWeakPtr(), id),
                       ui::DialogModel::Button::Params()
                           .SetLabel(l10n_util::GetStringUTF16(
                               IDS_WORKSPACE_DELETE_CONFIRM_DELETE_BUTTON))
                           .SetStyle(ui::ButtonStyle::kProminent))
          .AddCancelButton(base::DoNothing(),
                           ui::DialogModel::Button::Params().SetLabel(
                               l10n_util::GetStringUTF16(
                                   IDS_WORKSPACE_DIALOG_CANCEL_BUTTON)))
          .Build();

  constrained_window::ShowBrowserModal(
      std::move(dialog), GetAnchorView()->GetWidget()->GetNativeWindow());
  GetWidget()->Close();
}
