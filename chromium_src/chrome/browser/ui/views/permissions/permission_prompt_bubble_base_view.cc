/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/browser/ui/views/dialog_footnote_utils.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/permissions/permission_lifetime_utils.h"
#include "brave/components/permissions/permission_widevine_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/features.h"
#include "components/permissions/permission_prompt.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/request_type.h"
#include "components/strings/grit/components_strings.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/combobox_model.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/style/typography.h"
#include "ui/views/window/dialog_client_view.h"
#include "ui/views/window/dialog_delegate.h"

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_permission_request.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_WIDEVINE)
class DontAskAgainCheckbox : public views::Checkbox {
  METADATA_HEADER(DontAskAgainCheckbox, views::Checkbox)
 public:
  explicit DontAskAgainCheckbox(WidevinePermissionRequest* request);
  DontAskAgainCheckbox(const DontAskAgainCheckbox&) = delete;
  DontAskAgainCheckbox& operator=(const DontAskAgainCheckbox&) = delete;

 private:
  void ButtonPressed();

  raw_ptr<WidevinePermissionRequest> request_ = nullptr;
};

BEGIN_METADATA(DontAskAgainCheckbox)
END_METADATA

DontAskAgainCheckbox::DontAskAgainCheckbox(WidevinePermissionRequest* request)
    : views::Checkbox(brave_l10n::GetLocalizedResourceUTF16String(
                          IDS_WIDEVINE_DONT_ASK_AGAIN_CHECKBOX),
                      base::BindRepeating(&DontAskAgainCheckbox::ButtonPressed,
                                          base::Unretained(this))),
      request_(request) {}

void DontAskAgainCheckbox::ButtonPressed() {
  request_->set_dont_ask_again(GetChecked());
}

void AddAdditionalWidevineViewControlsIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    const std::vector<raw_ptr<permissions::PermissionRequest,
                              VectorExperimental>>& requests) {
  if (!HasWidevinePermissionRequest(requests)) {
    return;
  }

  auto* widevine_request = static_cast<WidevinePermissionRequest*>(requests[0]);
  views::Label* text = new views::Label(
      widevine_request->GetExplanatoryMessageText(),
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY);
  text->SetMultiLine(true);
  text->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  const int preferred_dialog_width = provider->GetSnappedDialogWidth(
      dialog_delegate_view->GetPreferredSize().width());
  // Resize width. Then, it's height deduced.
  text->SizeToFit(preferred_dialog_width -
                  dialog_delegate_view->margins().width());
  dialog_delegate_view->AddChildView(text);
  dialog_delegate_view->AddChildView(
      new DontAskAgainCheckbox(widevine_request));
}

void AddWidevineFootnoteView(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    Browser* browser) {
  const std::u16string footnote = brave_l10n::GetLocalizedResourceUTF16String(
      IDS_WIDEVINE_PERMISSIONS_BUBBLE_FOOTNOTE_TEXT);
  const std::vector<std::u16string> replacements{
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_WIDEVINE_PERMISSIONS_BUBBLE_LEARN_MORE),
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_PERMISSIONS_BUBBLE_SETTINGS_EXTENSIONS_LINK)};
  const std::vector<GURL> urls{GURL(kWidevineLearnMoreUrl),
                               GURL(kExtensionSettingsURL)};

  dialog_delegate_view->SetFootnoteView(
      views::CreateStyledLabelForDialogFootnote(browser, footnote, replacements,
                                                urls));
}
#else
void AddAdditionalWidevineViewControlsIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    const std::vector<raw_ptr<permissions::PermissionRequest,
                              VectorExperimental>>& requests) {}
#endif

// Custom combobox, shows permission lifetime options and applies selected value
// to all permissions currently visible in the bubble.
class PermissionLifetimeCombobox : public views::Combobox,
                                   public ui::ComboboxModel {
  METADATA_HEADER(PermissionLifetimeCombobox, views::Combobox)
 public:
  explicit PermissionLifetimeCombobox(
      views::BubbleDialogDelegateView& dialog_delegate_view,
      permissions::PermissionPrompt::Delegate& delegate)
      : dialog_delegate_view_(dialog_delegate_view),
        delegate_(delegate),
        lifetime_options_(permissions::CreatePermissionLifetimeOptions()) {
    SetCallback(base::BindRepeating(&PermissionLifetimeCombobox::OnItemSelected,
                                    base::Unretained(this)));
    SetModel(this);
    OnItemSelected();
    SetAccessibleName(brave_l10n::GetLocalizedResourceUTF16String(
        IDS_PERMISSIONS_BUBBLE_LIFETIME_COMBOBOX_LABEL));
  }

  PermissionLifetimeCombobox(const PermissionLifetimeCombobox&) = delete;
  PermissionLifetimeCombobox& operator=(const PermissionLifetimeCombobox&) =
      delete;

  // ui::ComboboxModel:
  size_t GetItemCount() const override { return lifetime_options_.size(); }

  std::u16string GetItemAt(size_t index) const override {
    return lifetime_options_[index].label;
  }

 private:
  void OnItemSelected() {
    SetRequestsLifetime(lifetime_options_, GetSelectedIndex().value(),
                        &*delegate_);
    // Workaround an upstream issue when a ComboBox close prevents any
    // interaction with the Permission bubble for 500ms.
    if (auto* dialog = dialog_delegate_view_->GetDialogClientView()) {
      dialog->IgnoreNextWindowStationaryStateChanged();
    }
  }

  const raw_ref<views::BubbleDialogDelegateView> dialog_delegate_view_;
  const raw_ref<permissions::PermissionPrompt::Delegate> delegate_;
  std::vector<permissions::PermissionLifetimeOption> lifetime_options_;
};

BEGIN_METADATA(PermissionLifetimeCombobox)
END_METADATA

views::View* AddPermissionLifetimeComboboxIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    permissions::PermissionPrompt::Delegate* delegate) {
  if (!ShouldShowLifetimeOptions(delegate)) {
    return nullptr;
  }

  // Create a single line container for a label and a combobox.
  auto container = std::make_unique<views::View>();
  container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      views::LayoutProvider::Get()->GetDistanceMetric(
          views::DISTANCE_RELATED_BUTTON_HORIZONTAL)));

  // Add the label.
  auto label = std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_PERMISSIONS_BUBBLE_LIFETIME_COMBOBOX_LABEL),
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY);
  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  container->AddChildView(std::move(label));

  // Add the combobox.
  DCHECK(delegate);
  auto* combobox =
      container->AddChildView(std::make_unique<PermissionLifetimeCombobox>(
          *dialog_delegate_view, *delegate));
  static_cast<views::BoxLayout*>(container->GetLayoutManager())
      ->SetFlexForView(combobox, 1);

  // Add the container to the view.
  return dialog_delegate_view->AddChildView(std::move(container));
}

void AddFootnoteViewIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    const std::vector<
        raw_ptr<permissions::PermissionRequest, VectorExperimental>>& requests,
    Browser* browser) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  // Widevine permission bubble has custom footnote.
  if (HasWidevinePermissionRequest(requests)) {
    AddWidevineFootnoteView(dialog_delegate_view, browser);
    return;
  }
#endif

  if (!base::FeatureList::IsEnabled(
          permissions::features::kPermissionLifetime)) {
    return;
  }

  const std::u16string footnote = brave_l10n::GetLocalizedResourceUTF16String(
      IDS_PERMISSIONS_BUBBLE_FOOTNOTE_TEXT);
  const std::vector<std::u16string> replacements{
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_PERMISSIONS_BUBBLE_SITE_PERMISSION_LINK),
      brave_l10n::GetLocalizedResourceUTF16String(IDS_LEARN_MORE)};
  const std::vector<GURL> urls{
      chrome::GetSettingsUrl(chrome::kContentSettingsSubPage),
      GURL(kPermissionPromptLearnMoreUrl)};

  dialog_delegate_view->SetFootnoteView(
      views::CreateStyledLabelForDialogFootnote(browser, footnote, replacements,
                                                urls));
}

}  // namespace

#define BRAVE_PERMISSION_PROMPT_BUBBLE_BASE_VIEW                          \
  AddAdditionalWidevineViewControlsIfNeeded(this, delegate_->Requests()); \
  auto* permission_lifetime_view =                                        \
      AddPermissionLifetimeComboboxIfNeeded(this, delegate_.get());       \
  AddFootnoteViewIfNeeded(this, delegate_->Requests(), browser_);         \
  if (permission_lifetime_view) {                                         \
    set_fixed_width(                                                      \
        std::max(GetPreferredSize().width(),                              \
                 permission_lifetime_view->GetPreferredSize().width()) +  \
        margins().width());                                               \
    set_should_ignore_snapping(true);                                     \
  }

#include "src/chrome/browser/ui/views/permissions/permission_prompt_bubble_base_view.cc"
#undef BRAVE_PERMISSION_PROMPT_BUBBLE_BASE_VIEW
