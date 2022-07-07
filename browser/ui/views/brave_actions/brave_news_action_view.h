#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_TODAY_ACTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_TODAY_ACTION_VIEW_H_

#include <string>
#include <vector>
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/browser/brave_news_tab_helper.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/prefs/pref_member.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/label_button.h"

class Profile;
class TabStripModel;

class BraveTodayActionView : public views::LabelButton,
                             public TabStripModelObserver,
                             public BraveNewsTabHelper::PageFeedsObserver {
 public:
  BraveTodayActionView(Profile* profile, TabStripModel* tab_strip);
  ~BraveTodayActionView() override;
  BraveTodayActionView(const BraveTodayActionView&) = delete;
  BraveTodayActionView& operator=(const BraveTodayActionView&) = delete;

  void Init();
  void Update();

  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;
  SkPath GetHighlightPath() const;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // BraveNewsTabHelper::PageFeedsObserver:
  void OnAvailableFeedsChanged(
      const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) override;

 private:
  void ShowBubble();

  BooleanPrefMember is_hidden_;

  Profile* profile_;
  TabStripModel* tab_strip_;
  base::WeakPtr<views::Widget> bubble_widget_;

};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_TODAY_ACTION_VIEW_H_
