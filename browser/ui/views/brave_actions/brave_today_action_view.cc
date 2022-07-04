#include "brave/browser/ui/views/brave_actions/brave_today_action_view.h"
#include <memory>
#include <string>
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/ui/views/brave_actions/brave_action_view.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/browser/brave_news_tab_helper.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/browser_context.h"
#include "extensions/common/constants.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkColor.h"
#include "include/core/SkPath.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep_default.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"

BraveTodayActionView::BraveTodayActionView(Profile* profile,
                                           TabStripModel* tab_strip)
    : views::LabelButton(
          base::BindRepeating(&BraveTodayActionView::ToggleSubscribed,
                              base::Unretained(this))),
      profile_(profile),
      tab_strip_(tab_strip),
      news_controller_(
          brave_news::BraveNewsControllerFactory::GetControllerForContext(
              profile)) {
  DCHECK(profile_);

  SetAccessibleName(u"Brave Today Button");
  SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_CENTER);

  auto* ink_drop = views::InkDrop::Get(this);
  ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
  ink_drop->SetBaseColorCallback(base::BindRepeating(
      [](views::View* host) { return GetToolbarInkDropBaseColor(host); },
      this));
  ink_drop->SetVisibleOpacity(kToolbarInkDropVisibleOpacity);
  SetHasInkDropActionOnClick(true);

  tab_strip_->AddObserver(this);
  news_controller_->publisher_controller()->AddObserver(this);
  news_controller_->GetPublishers(base::DoNothing());
}

BraveTodayActionView::~BraveTodayActionView() {
  tab_strip_->RemoveObserver(this);
  news_controller_->publisher_controller()->RemoveObserver(this);
}

void BraveTodayActionView::Init() {
  auto image =
      gfx::CreateVectorIcon(kBraveTodaySubscribeIcon, 16,
                            color_utils::DeriveDefaultIconColor(SK_ColorBLACK));
  SetImage(ButtonState::STATE_NORMAL, image);
}

void BraveTodayActionView::Update() {
  auto* contents = tab_strip_->GetActiveWebContents();
  if (!contents)
    return;

  current_publisher_ =
      news_controller_->publisher_controller()->GetPublisherForSite(
          contents->GetLastCommittedURL());

  SetVisible(!!current_publisher_);
}

SkPath BraveTodayActionView::GetHighlightPath() const {
  auto highlight_insets = gfx::Insets();
  gfx::Rect rect(GetPreferredSize());
  rect.Inset(highlight_insets);
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, rect.size());
  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(rect), radii, radii);
  return path;
}

std::u16string BraveTodayActionView::GetTooltipText(const gfx::Point& p) const {
  bool enabled = brave_news::IsPublisherEnabled(current_publisher_);

  return enabled ? u"Unsubscribe" : u"Subscribe";
}

std::unique_ptr<views::LabelButtonBorder>
BraveTodayActionView::CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      LabelButton::CreateDefaultBorder();
  border->set_insets(gfx::Insets::TLBR(3, 0, 3, 0));
  return border;
}

void BraveTodayActionView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  Update();
}

void BraveTodayActionView::OnPublishersUpdated(
    brave_news::PublishersController* controller) {
  Update();
}

void BraveTodayActionView::ToggleSubscribed() {
  if (!current_publisher_)
    return;

  bool enabled = brave_news::IsPublisherEnabled(current_publisher_);
  auto status = enabled ? brave_news::mojom::UserEnabled::DISABLED
                        : brave_news::mojom::UserEnabled::ENABLED;
  news_controller_->SetPublisherPref(current_publisher_->publisher_id, status);
}
