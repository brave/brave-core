/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspace/open_workspace_dialog.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/workspace/brave_workspace_service.h"
#include "brave/browser/workspace/brave_workspace_service_factory.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/simple_message_box.h"
#include "components/constrained_window/constrained_window_views.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/color/color_id.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_utils.h"

namespace {

constexpr int kDialogWidth = 420;
constexpr int kListHeight = 240;
constexpr int kPadding = 16;
constexpr int kRowSpacing = 2;
constexpr int kRowHeight = 36;

// A workspace list row that highlights its background on hover and shows a
// darker tint when selected.  SetNotifyEnterExitOnChild propagates mouse
// enter/exit from child buttons up to this view so the whole row responds.
class WorkspaceRowView : public views::View {
  METADATA_HEADER(WorkspaceRowView, views::View)
 public:
  WorkspaceRowView() { SetNotifyEnterExitOnChild(true); }

  void SetSelected(bool selected) {
    if (selected_ == selected) {
      return;
    }
    selected_ = selected;
    UpdateBackground();
  }

  void OnMouseEntered(const ui::MouseEvent& event) override {
    views::View::OnMouseEntered(event);
    hovered_ = true;
    UpdateBackground();
  }

  void OnMouseExited(const ui::MouseEvent& event) override {
    views::View::OnMouseExited(event);
    hovered_ = false;
    UpdateBackground();
  }

 private:
  void UpdateBackground() {
    if (selected_) {
      SetBackground(
          views::CreateSolidBackground(ui::kColorSysOnSurfaceSecondary));
    } else if (hovered_) {
      SetBackground(
          views::CreateSolidBackground(ui::kColorSysStateHoverOnSubtle));
    } else {
      SetBackground(nullptr);
    }
    for (views::View* child : children()) {
      if (auto* btn = views::AsViewClass<views::LabelButton>(child)) {
        btn->SetEnabledTextColors(
            selected_ ? std::optional<ui::ColorVariant>(SK_ColorWHITE)
                      : std::nullopt);
      } else if (auto* img_btn =
                     views::AsViewClass<views::ImageButton>(child)) {
        views::SetImageFromVectorIconWithColor(
            img_btn, kTrashCanIcon,
            selected_
                ? views::IconColors(SK_ColorWHITE, SK_ColorWHITE)
                : views::IconColors(ui::kColorIcon, ui::kColorIconDisabled));
      }
    }
  }

  bool hovered_ = false;
  bool selected_ = false;
};

BEGIN_METADATA(WorkspaceRowView)
END_METADATA

}  // namespace

// static
void OpenWorkspaceDialog::Show(Browser* browser) {
  auto* service =
      BraveWorkspaceServiceFactory::GetForProfile(browser->profile());
  if (!service) {
    return;
  }
  auto* dialog = new OpenWorkspaceDialog(browser, service->ListWorkspaces());
  constrained_window::CreateBrowserModalDialogViews(
      dialog, browser->window()->GetNativeWindow())
      ->Show();
}

OpenWorkspaceDialog::OpenWorkspaceDialog(Browser* browser,
                                         std::vector<WorkspaceInfo> workspaces)
    : browser_(browser), workspaces_(std::move(workspaces)) {
  SetButtonLabel(
      ui::mojom::DialogButton::kOk,
      l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_OPEN_BUTTON));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 l10n_util::GetStringUTF16(IDS_WORKSPACE_DIALOG_CANCEL_BUTTON));

  SetAcceptCallback(
      base::BindOnce(&OpenWorkspaceDialog::OnAccept, base::Unretained(this)));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(kPadding),
      kRowSpacing));

  if (workspaces_.empty()) {
    AddChildView(std::make_unique<views::Label>(
        l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_EMPTY_MESSAGE)));
  } else {
    // Scrollable list of workspaces.  Height is set by BuildWorkspaceList so
    // it fits the content exactly, up to kListHeight.
    auto scroll_view = std::make_unique<views::ScrollView>();

    auto list = std::make_unique<views::View>();
    list->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical, gfx::Insets(), kRowSpacing));
    list_container_ = list.get();

    scroll_view->SetContents(std::move(list));
    scroll_view_ = AddChildView(std::move(scroll_view));

    BuildWorkspaceList();
  }
}

OpenWorkspaceDialog::~OpenWorkspaceDialog() = default;

void OpenWorkspaceDialog::BuildWorkspaceList() {
  if (!list_container_) {
    return;
  }
  list_container_->RemoveAllChildViews();

  const int kRowContentWidth = kDialogWidth - kPadding * 2;

  for (size_t i = 0; i < workspaces_.size(); ++i) {
    const auto& info = workspaces_[i];
    std::u16string ws_stats_text;
    if (info.number_of_windows > 1) {
      ws_stats_text = base::UTF8ToUTF16(absl::StrFormat(
          "%d windows - %d ", info.number_of_windows, info.number_of_tabs));
    } else {
      ws_stats_text =
          base::UTF8ToUTF16(absl::StrFormat("%d ", info.number_of_tabs));
    }
    ws_stats_text +=
        base::UTF8ToUTF16((info.number_of_tabs == 1 ? "tab" : "tabs"));

    // Each row is a horizontal container: name button (flex) + delete button.
    // WorkspaceRowView installs an InkDrop so the row highlights on hover.
    auto row = std::make_unique<WorkspaceRowView>();
    auto row_box = std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 0);
    row_box->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);
    auto* row_layout = row_box.get();
    row->SetLayoutManager(std::move(row_box));
    row->SetPreferredSize(gfx::Size(kRowContentWidth, kRowHeight));

    auto name_btn = std::make_unique<views::LabelButton>(
        base::BindRepeating(&OpenWorkspaceDialog::OnWorkspaceSelected,
                            base::Unretained(this), static_cast<int>(i)),
        base::UTF8ToUTF16(info.name) + u"  " + ws_stats_text);
    name_btn->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    auto* name_btn_raw = row->AddChildView(std::move(name_btn));
    row_layout->SetFlexForView(name_btn_raw, 1);

    auto del_btn = views::CreateVectorImageButtonWithNativeTheme(
        base::BindRepeating(&OpenWorkspaceDialog::OnDeleteClicked,
                            base::Unretained(this), static_cast<int>(i)),
        kTrashCanIcon);
    del_btn->SetTooltipText(
        l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_ROW_DELETE_BUTTON));
    row->AddChildView(std::move(del_btn));

    list_container_->AddChildView(std::move(row));
  }

  // Resize the scroll view to fit the current row count exactly, up to
  // kListHeight (at which point the list becomes scrollable).
  if (scroll_view_) {
    int n = static_cast<int>(workspaces_.size());
    int desired = n > 0 ? n * kRowHeight + (n - 1) * kRowSpacing : 0;
    int height = std::min(desired, kListHeight);
    scroll_view_->ClipHeightTo(height, height);
    InvalidateLayout();
  }

  UpdateRowSelection();
}

void OpenWorkspaceDialog::UpdateRowSelection() {
  if (!list_container_) {
    return;
  }
  const auto& children = list_container_->children();
  for (size_t i = 0; i < children.size(); ++i) {
    static_cast<WorkspaceRowView*>(children[i].get())
        ->SetSelected(static_cast<int>(i) == selected_index_);
  }
}

void OpenWorkspaceDialog::OnWorkspaceSelected(int index) {
  selected_index_ = index;
  UpdateRowSelection();
  DialogModelChanged();
}

ui::mojom::ModalType OpenWorkspaceDialog::GetModalType() const {
  return ui::mojom::ModalType::kWindow;
}

std::u16string OpenWorkspaceDialog::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_TITLE);
}

bool OpenWorkspaceDialog::IsDialogButtonEnabled(
    ui::mojom::DialogButton button) const {
  if (button == ui::mojom::DialogButton::kOk) {
    return selected_index_ >= 0 &&
           selected_index_ < static_cast<int>(workspaces_.size());
  }
  return true;
}

void OpenWorkspaceDialog::OnAccept() {
  if (selected_index_ < 0 ||
      selected_index_ >= static_cast<int>(workspaces_.size())) {
    return;
  }
  brave::RestoreWorkspace(browser_, workspaces_[selected_index_].name);
}

void OpenWorkspaceDialog::OnDeleteClicked(int index) {
  if (index < 0 || index >= static_cast<int>(workspaces_.size())) {
    return;
  }
  std::string name = workspaces_[index].name;
  chrome::ShowQuestionMessageBoxAsync(
      browser_->window()->GetNativeWindow(),
      l10n_util::GetStringUTF16(IDS_WORKSPACE_DELETE_CONFIRM_TITLE),
      l10n_util::GetStringFUTF16(IDS_WORKSPACE_DELETE_CONFIRM_MESSAGE,
                                 base::UTF8ToUTF16(workspaces_[index].name)),
      base::BindOnce(&OpenWorkspaceDialog::OnDeleteConfirmed,
                     weak_factory_.GetWeakPtr(), std::move(name)));
}

void OpenWorkspaceDialog::OnDeleteConfirmed(std::string name,
                                            chrome::MessageBoxResult result) {
  if (result != chrome::MESSAGE_BOX_RESULT_YES) {
    return;
  }
  auto* service =
      BraveWorkspaceServiceFactory::GetForProfile(browser_->profile());
  if (!service) {
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&BraveWorkspaceService::DeleteWorkspace,
                     base::Unretained(service), name),
      base::BindOnce(&OpenWorkspaceDialog::OnDeleteCompleted,
                     weak_factory_.GetWeakPtr(), std::move(name)));
}

void OpenWorkspaceDialog::OnDeleteCompleted(std::string name, bool success) {
  if (!success) {
    return;
  }
  auto* service =
      BraveWorkspaceServiceFactory::GetForProfile(browser_->profile());
  if (service) {
    service->RemoveWorkspaceMetadata(name);
  }
  auto it =
      std::find_if(workspaces_.begin(), workspaces_.end(),
                   [&name](const WorkspaceInfo& w) { return w.name == name; });
  if (it == workspaces_.end()) {
    return;
  }
  if (selected_index_ >= 0 &&
      static_cast<size_t>(selected_index_) ==
          static_cast<size_t>(it - workspaces_.begin())) {
    selected_index_ = -1;
  }
  workspaces_.erase(it);
  BuildWorkspaceList();
  DialogModelChanged();
}

BEGIN_METADATA(OpenWorkspaceDialog)
END_METADATA
