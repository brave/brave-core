#include "brave/browser/ui/views/toolbar/share_tab_button.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/l10n/common/locale_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/vector_icons.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/paint_vector_icon.h"

namespace share_tab_button {

ShareTabButton::~ShareTabButton() = default;
ShareTabButton::ShareTabButton(PressedCallback callback)
    : ToolbarButton(callback) {}

void ShareTabButton::UpdateImageAndText() {
  const ui::ThemeProvider* tp = GetThemeProvider();

  SkColor icon_color = tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
  const gfx::VectorIcon& icon = omnibox::kShareIcon;
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(icon, icon_color));

  int tooltip_id = IDS_ACCESS_CODE_CAST_CONNECT;
  SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(tooltip_id));
}

}  // namespace share_tab_button