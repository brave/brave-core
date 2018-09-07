#include "../../../../../../../chrome/browser/ui/views/profiles/profile_chooser_view.cc"

void ProfileChooserView::AddTorButton(views::GridLayout* layout) {
    if (!browser_->profile()->IsTorProfile()) {
      tor_profile_button_ = new HoverButton(
          this,
          gfx::CreateVectorIcon(kLaunchIcon, kIconSize,
                                gfx::kChromeIconGrey),
          l10n_util::GetStringUTF16(IDS_PROFILES_OPEN_TOR_PROFILE_BUTTON));
      layout->StartRow(1.0, 0);
      layout->AddView(tor_profile_button_);
    }
}
