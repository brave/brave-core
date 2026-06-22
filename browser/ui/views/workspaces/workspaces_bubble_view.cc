/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/workspaces_bubble_view.h"

#include <memory>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/ui/views/workspaces/save_workspace_dialog.h"
#include "brave/browser/ui/views/workspaces/workspace_row_view.h"
#include "brave/browser/workspaces/workspace_service.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/dialogs/browser_dialogs.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/dialog_model.h"
#include "ui/base/models/dialog_model_field.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/gfx/font.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

WorkspacesBubbleView::~WorkspacesBubbleView() = default;

// static
void WorkspacesBubbleView::Show(views::View* anchor_view, Browser* browser) {
  // NATIVE_WIDGET_OWNS_WIDGET keeps the fire-and-forget ownership the bubble
  // had under BubbleDialogDelegateView: the widget owns and deletes the
  // delegate when it closes, so there is no lifetime to manage here.
  views::Widget* widget = views::BubbleDialogDelegate::CreateBubbleDeprecated(
      std::make_unique<WorkspacesBubbleView>(anchor_view, browser),
      views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET);
  widget->Show();
}

WorkspacesBubbleView::WorkspacesBubbleView(views::View* anchor_view,
                                           Browser* browser)
    : BubbleDialogDelegate(anchor_view, views::BubbleBorder::TOP_LEFT),
      browser_(*browser) {
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  SetShowCloseButton(false);
  set_margins(gfx::Insets());
  set_fixed_width(263);

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      /*inside_border_insets=*/gfx::Insets(),
      /*between_child_spacing=*/0));

  auto* service = WorkspaceServiceFactory::GetForProfile(browser->profile());
  CHECK(service);

  std::vector<WorkspaceMetadata> workspaces = service->ListWorkspaces();
  auto* workspaces_container = AddChildView(std::make_unique<views::View>());
  workspaces_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  workspaces_container->SetProperty(views::kMarginsKey, gfx::Insets(2));
  if (workspaces.empty()) {
    auto* empty_title =
        workspaces_container->AddChildView(std::make_unique<views::Label>(
            l10n_util::GetStringUTF16(IDS_WORKSPACES_BUBBLE_EMPTY_TITLE)));
    empty_title->SetFontList(empty_title->font_list()
                                 .DeriveWithSizeDelta(kTitleFontSizeDelta)
                                 .DeriveWithWeight(gfx::Font::Weight::BOLD));
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
      workspaces_container->AddChildView(std::make_unique<WorkspaceRowView>(
          workspace,
          base::BindRepeating(&WorkspacesBubbleView::OnWorkspaceSelected,
                              weak_factory_.GetWeakPtr(), workspace.name),
          base::BindRepeating(&WorkspacesBubbleView::OnDeleteClicked,
                              weak_factory_.GetWeakPtr(), workspace.name)));
    }
  }

  AddChildView(std::make_unique<views::Separator>());

  // Bottom action row: Settings + Save.
  auto* button_row = AddChildView(std::make_unique<views::View>());
  auto* button_row_layout =
      button_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          /*inside_border_insets=*/gfx::Insets(),
          /*between_child_spacing=*/0));
  button_row->SetProperty(views::kMarginsKey, gfx::Insets(8));

  auto* settings_button =
      button_row->AddChildView(std::make_unique<views::MdTextButton>(
          views::Button::PressedCallback(),
          l10n_util::GetStringUTF16(IDS_WORKSPACES_BUBBLE_SETTINGS_BUTTON)));
  settings_button->SetEnabled(false);
  settings_button->SetProperty(views::kMarginsKey, gfx::Insets(4));
  button_row_layout->SetFlexForView(settings_button, 1);

  auto* new_space_button =
      button_row->AddChildView(std::make_unique<views::MdTextButton>(
          base::BindRepeating(&WorkspacesBubbleView::OnSaveClicked,
                              weak_factory_.GetWeakPtr()),
          l10n_util::GetStringUTF16(IDS_WORKSPACES_BUBBLE_SAVE_BUTTON)));
  new_space_button->SetStyle(ui::ButtonStyle::kProminent);
  new_space_button->SetProperty(views::kMarginsKey, gfx::Insets(4));
  button_row_layout->SetFlexForView(new_space_button, 1);
}

views::View* WorkspacesBubbleView::GetContentsView() {
  return this;
}

views::Widget* WorkspacesBubbleView::GetWidget() {
  return View::GetWidget();
}

const views::Widget* WorkspacesBubbleView::GetWidget() const {
  return View::GetWidget();
}

void WorkspacesBubbleView::OnSaveClicked() {
  SaveWorkspaceDialog::Show(base::to_address(browser_));
  GetWidget()->Close();
}

void WorkspacesBubbleView::OnWorkspaceSelected(const std::string& name) {
  auto* service = WorkspaceServiceFactory::GetForProfile(browser_->profile());
  CHECK(service);
  service->RestoreWorkspace(name);
  GetWidget()->Close();
}

void WorkspacesBubbleView::OnDeleteClicked(const std::string& name) {
  auto* service = WorkspaceServiceFactory::GetForProfile(browser_->profile());
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
                                      service->GetWeakPtr(), name),
                       ui::DialogModel::Button::Params()
                           .SetLabel(l10n_util::GetStringUTF16(
                               IDS_WORKSPACE_DELETE_CONFIRM_DELETE_BUTTON))
                           .SetStyle(ui::ButtonStyle::kProminent))
          .AddCancelButton(base::DoNothing(),
                           ui::DialogModel::Button::Params().SetLabel(
                               l10n_util::GetStringUTF16(
                                   IDS_WORKSPACE_DIALOG_CANCEL_BUTTON)))
          .Build();

  chrome::ShowBrowserModal(base::to_address(browser_), std::move(dialog));
  GetWidget()->Close();
}

BEGIN_METADATA(WorkspacesBubbleView)
END_METADATA
