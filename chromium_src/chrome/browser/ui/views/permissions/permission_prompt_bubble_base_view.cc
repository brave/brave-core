/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/permissions/permission_prompt_bubble_base_view.h"

#include <vector>

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/browser/ui/geolocation/brave_geolocation_permission_tab_helper.h"
#include "brave/browser/ui/geolocation/geolocation_utils.h"
#include "brave/browser/ui/views/dialog_footnote_utils.h"
#include "brave/browser/ui/views/infobars/custom_styled_label.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/permissions/permission_lifetime_utils.h"
#include "brave/components/permissions/permission_widevine_utils.h"
#include "brave/components/vector_icons/vector_icons.h"
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
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
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

constexpr char kGeolocationPermissionLearnMoreURL[] =
#if BUILDFLAG(IS_WIN)
    "https://support.microsoft.com/en-us/windows/"
    "windows-location-service-and-privacy-3a8eee0a-5b0b-dc07-eede-"
    "2a5ca1c49088";
#elif BUILDFLAG(IS_MAC)
    "https://support.apple.com/guide/mac-help/"
    "allow-apps-to-detect-the-location-of-your-mac-mh35873/mac";
#else
    // Not used now. Set proper link when detailed bubble is enabled on linux.
    "https://www.brave.com/";
#endif

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

std::unique_ptr<views::View> CreateGeolocationDescLabel(
    Browser* browser,
    bool enable_high_accuracy,
    bool location_service_is_on) {
  // The text shown in dialog is different depending on if person has location
  // services enabled or disabled. This code finds which placeholders should
  // show.
  int string_id = IDS_GEOLOCATION_PERMISSION_BUBBLE_LOW_ACCURACY_LABEL;
  size_t expected_offset_size = 4;
  if (enable_high_accuracy && location_service_is_on) {
    string_id =
        IDS_GEOLOCATION_PERMISSION_BUBBLE_HIGH_ACCURACY_WITH_LOCATION_SERVICE_LABEL;
  } else if (enable_high_accuracy) {
    string_id =
        IDS_GEOLOCATION_PERMISSION_BUBBLE_HIGH_ACCURACY_WITHOUT_LOCATION_SERVICE_LABEL;
    expected_offset_size = 8;
  }

  // This code will get the actual strings so we know the length/offset of each
  // styles (bold, italics, etc) can be applied with a range (begin offset / end
  // offset).
  std::vector<size_t> offsets;
  const std::u16string contents_text =
      l10n_util::GetStringFUTF16(string_id, {u""}, &offsets);
  CHECK(!contents_text.empty());
  CHECK_EQ(expected_offset_size, offsets.size());

  // Insert the placeholder text with styles.
  auto contents_label = std::make_unique<views::StyledLabel>();
  contents_label->SetTextContext(views::style::CONTEXT_LABEL);
  contents_label->SetDefaultTextStyle(views::style::STYLE_PRIMARY);
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  contents_label->SetText(contents_text);
  views::StyledLabel::RangeStyleInfo part_style;
  part_style.text_style = views::style::STYLE_EMPHASIZED;
  const int part_count_except_learn_more = offsets.size() / 2 - 1;
  for (int i = 0; i < part_count_except_learn_more; ++i) {
    // Each part has start/end offset pair.
    const int part_start_offset = i * 2;
    contents_label->AddStyleRange(
        gfx::Range(offsets[part_start_offset], offsets[part_start_offset + 1]),
        part_style);
  }

  // It's ok to use |browser| in the link's callback as bubbble is tied with
  // that |browser| and bubble is destroyed earlier than browser.
  views::StyledLabel::RangeStyleInfo learn_more_style =
      views::StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
          [](Browser* browser) {
            chrome::AddSelectedTabWithURL(
                browser, GURL(kGeolocationPermissionLearnMoreURL),
                ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
          },
          browser));

  // Learn more is last part.
  const int learn_more_offset = offsets.size() - 2;
  contents_label->AddStyleRange(
      gfx::Range(offsets[learn_more_offset], offsets[learn_more_offset + 1]),
      learn_more_style);
  return contents_label;
}

std::unique_ptr<views::View> CreateGeolocationDescIcon(
    bool enable_high_accuracy,
    bool location_service_is_on) {
  const bool use_high_accuracy = enable_high_accuracy && location_service_is_on;
  constexpr int kIconSize = 16;
  auto icon_view =
      std::make_unique<views::ImageView>(ui::ImageModel::FromVectorIcon(
          use_high_accuracy ? kLeoWarningTriangleOutlineIcon
                            : kLeoInfoOutlineIcon,
          ui::ColorIds::kColorMenuIcon, kIconSize));
  // To align with text more precisely.
  icon_view->SetBorder(views::CreateEmptyBorder(gfx::Insets::TLBR(2, 0, 0, 0)));
  return icon_view;
}

void AddGeolocationDescription(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    Browser* browser,
    bool enable_high_accuracy,
    bool location_service_is_on) {
  auto container = std::make_unique<views::View>();
  constexpr int kPadding = 12;
  constexpr int kChildSpacing = 6;
  container
      ->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::TLBR(kPadding, 0, 0, 0), kChildSpacing))
      ->set_cross_axis_alignment(views::BoxLayout::CrossAxisAlignment::kStart);

  container->AddChildView(
      CreateGeolocationDescIcon(enable_high_accuracy, location_service_is_on));
  container->AddChildView(CreateGeolocationDescLabel(
      browser, enable_high_accuracy, location_service_is_on));
  dialog_delegate_view->AddChildView(std::move(container));
}

void AddGeolocationDescriptionIfNeeded(
    PermissionPromptBubbleBaseView* bubble_base_view,
    permissions::PermissionPrompt::Delegate* delegate,
    Browser* browser) {
  if (!geolocation::CanGiveDetailedGeolocationRequestInfo()) {
    return;
  }

  // Could be nullptr in unit test.
  if (!browser) {
    return;
  }

  auto requests = delegate->Requests();

  // Geolocation permission is not grouped with others.
  if (requests.empty() ||
      requests[0]->request_type() != permissions::RequestType::kGeolocation) {
    return;
  }

  bool enable_high_accuracy = false;
  if (auto* web_contents = delegate->GetAssociatedWebContents()) {
    if (auto* geolocation_tab_helper =
            BraveGeolocationPermissionTabHelper::FromWebContents(
                web_contents)) {
      enable_high_accuracy = geolocation_tab_helper->enable_high_accuracy();
    }
  }

  geolocation::IsSystemLocationSettingEnabled(base::BindOnce(
      [](base::WeakPtr<views::WidgetDelegate> widget_delegate,
         base::WeakPtr<Browser> browser, bool enable_high_accuracy,
         bool settings_enabled) {
        // Browser or Bubble is already gone.
        if (!browser || !widget_delegate) {
          return;
        }
        PermissionPromptBubbleBaseView* bubble_base_view =
            static_cast<PermissionPromptBubbleBaseView*>(widget_delegate.get());
        AddGeolocationDescription(
            bubble_base_view, browser.get(),
            /*enable_high_accuracy*/ enable_high_accuracy,
            /*use_exact_location_label*/ settings_enabled);

        // To update widget layout after adding another child view.
        bubble_base_view->UpdateAnchorPosition();
      },
      bubble_base_view->GetWeakPtr(), browser->AsWeakPtr(),
      enable_high_accuracy));
}

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
  AddFootnoteViewIfNeeded(this, delegate_->Requests(), browser());        \
  if (permission_lifetime_view) {                                         \
    set_fixed_width(                                                      \
        std::max(GetPreferredSize().width(),                              \
                 permission_lifetime_view->GetPreferredSize().width()) +  \
        margins().width());                                               \
    set_should_ignore_snapping(true);                                     \
  }                                                                       \
  AddGeolocationDescriptionIfNeeded(this, delegate_.get(), browser());

#include "src/chrome/browser/ui/views/permissions/permission_prompt_bubble_base_view.cc"
#undef BRAVE_PERMISSION_PROMPT_BUBBLE_BASE_VIEW
