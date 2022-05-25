#include "brave_talk_share_tab_action_view.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "brave/browser/brave_talk/brave_talk_service.h"
#include "brave/browser/brave_talk/brave_talk_service_factory.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/extensions/icon_with_badge_image_source.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/vector_icons/vector_icons.h"
#include "extensions/common/constants.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkColor.h"
#include "include/core/SkPath.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"

namespace {
class BraveTalkShareTabActionHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  BraveTalkShareTabActionHighlightPathGenerator() = default;
  BraveTalkShareTabActionHighlightPathGenerator(
      const BraveTalkShareTabActionHighlightPathGenerator&) = delete;
  BraveTalkShareTabActionHighlightPathGenerator& operator=(
      const BraveTalkShareTabActionHighlightPathGenerator&) = delete;

  // HighlightPathGenerator
  SkPath GetHighlightPath(const views::View* view) override {
    return static_cast<const BraveShieldsActionView*>(view)->GetHighlightPath();
  }
};
}  // namespace

namespace brave_talk {

BraveTalkShareTabActionView::BraveTalkShareTabActionView(
    Profile* profile,
    TabStripModel* tab_strip_model)
    : views::LabelButton(
          base::BindRepeating(&BraveTalkShareTabActionView::ButtonPressed,
                              base::Unretained(this)),
          std::u16string()),
      profile_(profile),
      tab_strip_model_(tab_strip_model),
      brave_talk_service_(BraveTalkServiceFactory::GetForContext(profile)) {
  auto* ink_drop = views::InkDrop::Get(this);
  ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
  ink_drop->SetBaseColorCallback(base::BindRepeating(
      [](views::View* host) { return GetToolbarInkDropBaseColor(host); },
      this));
  SetAccessibleName(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_TALK_SHARE_TAB_BUTTON_TOOLTIP));
  SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_TALK_SHARE_TAB_BUTTON_TOOLTIP));
  SetHasInkDropActionOnClick(true);
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  ink_drop->SetVisibleOpacity(kToolbarInkDropVisibleOpacity);

  SetImage(
      ButtonState::STATE_NORMAL,
      gfx::CreateVectorIcon(vector_icons::kScreenShareIcon, 20, SK_ColorWHITE));
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveTalkShareTabActionHighlightPathGenerator>());

  SetVisible(brave_talk_service_->is_requesting_tab());
  brave_talk_service_->AddObserver(this);
}

BraveTalkShareTabActionView::~BraveTalkShareTabActionView() {
  brave_talk_service_->RemoveObserver(this);
}

void BraveTalkShareTabActionView::OnIsRequestingChanged(bool requesting) {
  SetVisible(requesting);
}

void BraveTalkShareTabActionView::ButtonPressed() {
  brave_talk_service_->PromptShareTab(tab_strip_model_->GetActiveWebContents());
}

std::unique_ptr<views::LabelButtonBorder>
BraveTalkShareTabActionView::CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      views::LabelButton::CreateDefaultBorder();
  border->set_insets(gfx::Insets(0, 0));
  return border;
}
}  // namespace brave_talk
