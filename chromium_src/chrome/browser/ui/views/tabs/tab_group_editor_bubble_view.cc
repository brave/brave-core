/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_group_editor_bubble_view.h"

#include <algorithm>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "brave/components/local_ai/browser/text_embedder.h"
#include "brave/components/local_ai/common/features.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/bubble_menu_item_factory.h"
#include "chrome/browser/ui/views/data_sharing/data_sharing_bubble_controller.h"
#include "chrome/browser/user_education/user_education_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/base/mojom/ui_base_types.mojom.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "url/gurl.h"

namespace {

// If a "learn more" footer has been added to the bubble view, remove it and
// ensure that the view's interior margins are correct.
void MaybeRemoveFooter(TabGroupEditorBubbleView* bubble_view,
                       views::View* footer) {
  if (!footer) {
    return;
  }

  auto footer_holder = bubble_view->RemoveChildViewT(footer);

  auto* layout =
      static_cast<views::FlexLayout*>(bubble_view->GetLayoutManager());
  CHECK(layout);
  gfx::Insets margin = layout->interior_margin();
  margin.set_bottom(margin.top());
  layout->SetInteriorMargin(margin);
}

}  // namespace

// Dialog for showing suggested tabs to the user
class TabSuggestionDialog : public views::DialogDelegate {
 public:
  TabSuggestionDialog(Browser* browser,
                      std::vector<int> suggested_tab_indices,
                      tab_groups::TabGroupId group)
      : browser_(browser),
        suggested_tab_indices_(std::move(suggested_tab_indices)),
        group_(group) {}

  static void Show(Browser* browser,
                   std::vector<int> suggested_tab_indices,
                   tab_groups::TabGroupId group,
                   gfx::NativeWindow parent_window) {
    auto* dialog = new TabSuggestionDialog(
        browser, std::move(suggested_tab_indices), group);
    dialog->CreateAndShow(parent_window);
  }

  void CloseDialog() {
    if (GetWidget()) {
      GetWidget()->Close();
    }
  }

  // views::DialogDelegate overrides:
  std::u16string GetWindowTitle() const override {
    return u"Add Suggested Tabs";
  }

  views::View* GetContentsView() override {
    if (!contents_view_) {
      CreateContentsView();
    }
    return contents_view_;
  }

  bool CanResize() const override { return false; }

  ui::mojom::ModalType GetModalType() const override {
    return ui::mojom::ModalType::kWindow;
  }

  bool Accept() override {
    // Collect selected tab indices
    std::vector<int> selected_indices;
    for (size_t i = 0;
         i < checkboxes_.size() && i < suggested_tab_indices_.size(); ++i) {
      if (checkboxes_[i]->GetChecked()) {
        selected_indices.push_back(suggested_tab_indices_[i]);
      }
    }

    // Perform the actual tab operations directly in the dialog
    if (!selected_indices.empty()) {
      AddTabsToGroup(std::move(selected_indices));
    }

    return true;  // Close dialog
  }

  bool Cancel() override {
    return true;  // Close dialog
  }

 private:
  void AddTabsToGroup(std::vector<int> selected_tab_indices) {
    // Validate that browser is still valid
    if (!browser_) {
      return;
    }

    // Get tab strip model with validation
    TabStripModel* tab_strip_model = browser_->tab_strip_model();
    if (!tab_strip_model) {
      return;
    }

    // Validate that all selected tab indices are still valid
    std::vector<int> valid_indices;
    for (int tab_index : selected_tab_indices) {
      if (tab_index >= 0 && tab_index < tab_strip_model->count()) {
        content::WebContents* web_contents =
            tab_strip_model->GetWebContentsAt(tab_index);
        if (web_contents) {
          // Ensure the tab is not already in any group
          std::optional<tab_groups::TabGroupId> existing_group =
              tab_strip_model->GetTabGroupForTab(tab_index);
          if (!existing_group.has_value()) {
            valid_indices.push_back(tab_index);
          }
        }
      }
    }

    // Add valid selected tabs to the group
    if (!valid_indices.empty()) {
      // Sort indices in ascending order as required by
      // TabStripModel::AddToExistingGroup
      std::ranges::sort(valid_indices);
      tab_strip_model->AddToExistingGroup(valid_indices, group_);
    }
  }

  void CreateAndShow(gfx::NativeWindow parent_window) {
    views::Widget::InitParams params(
        views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET,
        views::Widget::InitParams::TYPE_WINDOW);
    params.delegate = this;
    params.context = parent_window;

    auto* widget = new views::Widget();
    widget->Init(std::move(params));
    widget->CenterWindow(gfx::Size(450, 250));
    widget->Show();
  }

  void CreateContentsView() {
    // Create the content view
    contents_view_ = new views::View();
    contents_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical, gfx::Insets(10), 5));

    // Add description
    auto* description = contents_view_->AddChildView(
        std::make_unique<views::Label>(u"Select tabs to add to your group:"));
    description->SetMultiLine(true);
    description->SetHorizontalAlignment(gfx::ALIGN_LEFT);

    // Create scroll view for tab list
    auto scroll_view = std::make_unique<views::ScrollView>();
    auto* scroll_content =
        scroll_view->SetContents(std::make_unique<views::View>());
    scroll_content->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical, gfx::Insets(), 2));

    TabStripModel* tab_strip_model = browser_->tab_strip_model();
    checkboxes_.reserve(suggested_tab_indices_.size());

    for (int tab_index : suggested_tab_indices_) {
      content::WebContents* web_contents =
          tab_strip_model->GetWebContentsAt(tab_index);
      if (!web_contents) {
        continue;
      }

      std::u16string title = web_contents->GetTitle();
      if (title.empty()) {
        title = u"Untitled";
      }

      auto checkbox = std::make_unique<views::Checkbox>(title);
      checkbox->SetChecked(true);  // Default to checked
      checkboxes_.push_back(checkbox.get());
      scroll_content->AddChildView(std::move(checkbox));
    }

    scroll_view->SetPreferredSize(gfx::Size(400, 150));
    contents_view_->AddChildView(std::move(scroll_view));
    contents_view_->SetPreferredSize(gfx::Size(450, 250));
  }

  raw_ptr<Browser> browser_ = nullptr;
  std::vector<int> suggested_tab_indices_;
  tab_groups::TabGroupId group_;
  std::vector<raw_ptr<views::Checkbox>> checkboxes_;
  raw_ptr<views::View> contents_view_ = nullptr;
};

#define CreateBubble(bubble_view)                                           \
  CreateBubble(bubble_view);                                                \
  bubble_view->MaybeAddSuggestedTabsButton();                               \
  MaybeRemoveFooter(bubble_view, bubble_view->footer_.ExtractAsDangling()); \
  bubble_view->footer_ = nullptr

#define kUngroupRefreshIcon \
  kUngroupRefreshIcon, ui::kColorMenuIcon, kDefaultIconSize

#include <chrome/browser/ui/views/tabs/tab_group_editor_bubble_view.cc>

#undef kUngroupRefreshIcon

#undef CreateBubble

std::unique_ptr<views::LabelButton>
TabGroupEditorBubbleView::BuildSuggestedTabInGroupButton() {
  return CreateMenuItem(
      TAB_GROUP_HEADER_CXMENU_ADD_SUGGESTED_TABS,
      l10n_util::GetStringUTF16(IDS_TAB_GROUP_HEADER_CXMENU_ADD_SUGGESTED_TABS),
      base::BindRepeating(&TabGroupEditorBubbleView::SuggestedTabsPressed,
                          base::Unretained(this)),
      ui::ImageModel::FromVectorIcon(kNewTabInGroupRefreshIcon,
                                     ui::kColorMenuIcon, 20));
}

void TabGroupEditorBubbleView::SuggestedTabsPressed() {
  // Check if already processing a suggestion
  if (suggestion_in_progress_) {
    return;  // Already processing, ignore
  }

  // Get the tab strip model from the browser
  TabStripModel* tab_strip_model = browser_->tab_strip_model();
  if (!tab_strip_model) {
    GetWidget()->Close();
    return;
  }

  // Check if the text embedder model is available
  auto* state = local_ai::LocalModelsUpdaterState::GetInstance();
  if (state->GetInstallDir().empty()) {
    GetWidget()->Close();
    return;
  }

  suggestion_in_progress_ = true;

  // Create TextEmbedder if needed
  if (!text_embedder_) {
    auto embedder_task_runner = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_BLOCKING});
    text_embedder_ = local_ai::TextEmbedder::Create(
        state->GetUniversalQAModel(), embedder_task_runner);

    if (!text_embedder_) {
      CleanupAndClose();
      return;
    }
  }

  // Initialize if needed, or process immediately
  if (!text_embedder_->IsInitialized()) {
    text_embedder_->Initialize(
        base::BindOnce(&TabGroupEditorBubbleView::OnTextEmbedderInitialized,
                       base::Unretained(this)));
  } else {
    ProcessTabSuggestion();
  }
}

void TabGroupEditorBubbleView::MaybeAddSuggestedTabsButton() {
  // Only add the button if the Local AI Tab Grouping feature is enabled
  if (!base::FeatureList::IsEnabled(local_ai::features::kLocalAITabGrouping)) {
    return;
  }

  // Find the position after "New tab in group" button and insert our button
  for (size_t i = 0; i < simple_menu_items_.size(); ++i) {
    if (simple_menu_items_[i]->GetID() ==
        TAB_GROUP_HEADER_CXMENU_NEW_TAB_IN_GROUP) {
      // Create our button
      auto suggested_button = BuildSuggestedTabInGroupButton();
      auto* suggested_button_ptr = suggested_button.get();

      // Find the NEW_TAB_IN_GROUP button in the view hierarchy and insert after
      // it
      views::LabelButton* new_tab_view = simple_menu_items_[i];
      auto view_index_opt = GetIndexOf(new_tab_view);
      if (!view_index_opt.has_value()) {
        AddChildView(std::move(suggested_button));  // Fallback to end
      } else {
        int view_index = static_cast<int>(view_index_opt.value());
        // Insert our button at the next position in the view hierarchy
        AddChildViewAt(std::move(suggested_button), view_index + 1);
      }

      // Also insert into simple_menu_items_ at the correct position
      simple_menu_items_.insert(simple_menu_items_.begin() + i + 1,
                                suggested_button_ptr);
      break;
    }
  }
}

void TabGroupEditorBubbleView::OnTextEmbedderInitialized(bool initialized) {
  if (!initialized) {
    // Initialization failed
    CleanupAndClose();
    return;
  }

  // Initialization successful, process the suggestion
  ProcessTabSuggestion();
}

void TabGroupEditorBubbleView::ProcessTabSuggestion() {
  if (!text_embedder_) {
    CleanupAndClose();
    return;
  }

  TabStripModel* tab_strip_model = browser_->tab_strip_model();
  if (!tab_strip_model) {
    CleanupAndClose();
    return;
  }

  // Collect tabs in the current group and candidate tabs
  std::vector<std::string> group_tabs;
  std::vector<std::pair<int, std::string>> candidate_tabs;

  for (int i = 0; i < tab_strip_model->count(); ++i) {
    content::WebContents* web_contents = tab_strip_model->GetWebContentsAt(i);
    if (!web_contents) {
      continue;
    }

    // Get tab title and origin
    std::u16string title = web_contents->GetTitle();
    GURL url = web_contents->GetVisibleURL();
    std::string tab_description =
        base::StrCat({base::UTF16ToUTF8(title), " | ", url.spec()});

    // Check if this tab is in any group
    std::optional<tab_groups::TabGroupId> tab_group =
        tab_strip_model->GetTabGroupForTab(i);

    if (tab_group.has_value() && tab_group.value() == group_) {
      // Tab is in the current group - only need the description
      group_tabs.push_back(tab_description);
    } else if (!tab_group.has_value()) {
      // Tab is not in any group - candidate for addition
      candidate_tabs.emplace_back(i, std::move(tab_description));
    }
  }

  // If there are no candidate tabs or no group tabs, just close
  if (candidate_tabs.empty() || group_tabs.empty()) {
    CleanupAndClose();
    return;
  }

  text_embedder_->SuggestTabsForGroup(
      std::move(group_tabs), std::move(candidate_tabs),
      base::BindOnce(&TabGroupEditorBubbleView::OnTabSuggestionResult,
                     base::Unretained(this)));
}

void TabGroupEditorBubbleView::OnTabSuggestionResult(
    absl::StatusOr<std::vector<int>> result) {
  if (!result.ok()) {
    // Error occurred
    CleanupAndClose();
    return;
  }

  std::vector<int> suggested_tab_indices = result.value();
  if (suggested_tab_indices.empty()) {
    CleanupAndClose();
    return;
  }

  // Show dialog with suggested tabs for user confirmation
  ShowSuggestionDialog(std::move(suggested_tab_indices));
}

void TabGroupEditorBubbleView::ShowSuggestionDialog(
    std::vector<int> suggested_tab_indices) {
  // Validate that we have a valid browser and browser window
  if (!browser_ || !browser_->window()) {
    CleanupAndClose();
    return;
  }

  // Get the parent window from the browser instead of this widget
  gfx::NativeWindow parent_window = browser_->window()->GetNativeWindow();
  if (!parent_window) {
    CleanupAndClose();
    return;
  }

  // Use the static Show method to create and show the dialog
  TabSuggestionDialog::Show(browser_, std::move(suggested_tab_indices), group_,
                            parent_window);

  // Close the bubble view after showing the dialog
  CleanupAndClose();
}

void TabGroupEditorBubbleView::CleanupTextEmbedder() {
  if (text_embedder_) {
    text_embedder_->CancelAllTasks();
    text_embedder_.reset();
  }
  suggestion_in_progress_ = false;
}

void TabGroupEditorBubbleView::CleanupAndClose() {
  CleanupTextEmbedder();
  GetWidget()->Close();
}
