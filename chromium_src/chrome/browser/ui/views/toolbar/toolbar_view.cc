#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button.h"
#include "chrome/browser/ui/views/translate/translate_bubble_view.h"

class BraveTranslateBubbleView : public TranslateBubbleView {
public:
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   views::Button* highlighted_button,
                                   const gfx::Point& anchor_point,
                                   content::WebContents* web_contents,
                                   translate::TranslateStep step,
                                   translate::TranslateErrors::Type error_type,
                                   DisplayReason reason) {
    return nullptr;
  }
  DISALLOW_COPY_AND_ASSIGN(BraveTranslateBubbleView);
};

#define LocationBarView BraveLocationBarView
#define AvatarToolbarButton BraveAvatarToolbarButton
#define TranslateBubbleView BraveTranslateBubbleView
#include "../../../../../../../chrome/browser/ui/views/toolbar/toolbar_view.cc"
#undef LocationBarView
#undef AvatarToolbarButton
#undef TranslateBubbleView
