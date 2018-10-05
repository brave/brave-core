#include "chrome/browser/ui/cocoa/translate/translate_bubble_bridge_views.h"
#define ShowTranslateBubbleViews ShowTranslateBubbleViews_ChromiumImpl
#include "../../../../../../chrome/browser/ui/cocoa/translate/translate_bubble_bridge_views.mm"
#undef ShowTranslateBubbleViews

void ShowTranslateBubbleViews(NSWindow* parent_window,
                              LocationBarViewMac* location_bar,
                              content::WebContents* web_contents,
                              translate::TranslateStep step,
                              translate::TranslateErrors::Type error_type,
                              bool is_user_gesture) {
}
