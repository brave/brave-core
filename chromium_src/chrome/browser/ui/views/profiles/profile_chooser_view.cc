#include "../../../../../../../chrome/browser/ui/views/profiles/profile_chooser_view.cc"

#include "brave/common/tor/pref_names.h"

void ProfileChooserView::AddTorButton(views::GridLayout* layout) {
    PrefService* service = browser_->profile()->GetPrefs();
    DCHECK(service);
    if (!service->GetBoolean(tor::prefs::kProfileUsingTor)) {
      tor_profile_button_ = new HoverButton(
          this,
          gfx::CreateVectorIcon(kUserMenuGuestIcon, kIconSize,
                                gfx::kChromeIconGrey),
          l10n_util::GetStringUTF16(IDS_PROFILES_OPEN_TOR_PROFILE_BUTTON));
      layout->StartRow(1.0, 0);
      layout->AddView(tor_profile_button_);
    }
}
