#include "brave/browser/ui/views/brave_actions/brave_today_action_view.h"
#include <memory>
#include <string>
#include <vector>
#include "absl/types/optional.h"
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/callback_helpers.h"
#include "base/task/task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
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
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"

namespace {
SkColor selectedColor = SkColorSetRGB(30, 33, 82);
}

BraveTodayActionView::BraveTodayActionView(Profile* profile,
                                           TabStripModel* tab_strip)
    : views::LabelButton(
          base::BindRepeating(&BraveTodayActionView::ToggleSubscribed,
                              base::Unretained(this))),
      profile_(profile),
      tab_strip_(tab_strip) {
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
}

BraveTodayActionView::~BraveTodayActionView() {
  tab_strip_->RemoveObserver(this);
}

void BraveTodayActionView::Init() {
  Update();
}

void BraveTodayActionView::Update() {
  auto* contents = tab_strip_->GetActiveWebContents();
  bool subscribed = false;
  absl::optional<BraveNewsTabHelper::FeedDetails> feed = absl::nullopt;

  if (contents) {
    auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents);
    if (!tab_helper->available_feeds().empty()) {
      feed = tab_helper->available_feeds()[0];
      subscribed = tab_helper->is_subscribed();
    }
  }

  auto background =
      subscribed ? views::CreateRoundedRectBackground(
                       selectedColor,
                       ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
                           views::Emphasis::kMaximum, GetPreferredSize()))
                 : nullptr;
  auto image =
      gfx::CreateVectorIcon(kBraveTodaySubscribeIcon, 16,
                            color_utils::DeriveDefaultIconColor(
                                subscribed ? SK_ColorWHITE : SK_ColorBLACK));
  SetImage(ButtonState::STATE_NORMAL, image);
  SetBackground(std::move(background));
  SetVisible(!!feed);
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
  bool enabled = false;

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
  if (selection.old_contents != selection.new_contents) {
    if (selection.old_contents) {
      BraveNewsTabHelper::FromWebContents(selection.old_contents)
          ->RemoveObserver(this);
    }

    if (selection.new_contents) {
      BraveNewsTabHelper::FromWebContents(selection.new_contents)
          ->AddObserver(this);
    }
  }

  Update();
}

void BraveTodayActionView::OnAvailableFeedsChanged(
    const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) {
  Update();
}

void BraveTodayActionView::ToggleSubscribed() {
  if (!tab_strip_->GetActiveWebContents())
    return;

  auto* tab_helper =
      BraveNewsTabHelper::FromWebContents(tab_strip_->GetActiveWebContents());
  if (tab_helper->available_feeds().empty())
    return;

  auto default_feed = tab_helper->available_feeds()[0];
  tab_helper->ToggleSubscription(default_feed);
}
