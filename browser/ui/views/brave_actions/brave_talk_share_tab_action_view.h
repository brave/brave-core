#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_TALK_SHARE_TAB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_TALK_SHARE_TAB_VIEW_H_

#include <memory>
#include <string>

#include "brave/browser/brave_talk/brave_talk_service.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/view.h"

class Profile;
class TabStripModel;

namespace brave_talk {

class BraveTalkShareTabActionView : public views::LabelButton, public BraveTalkService::BraveTalkServiceObserver {
 public:
  explicit BraveTalkShareTabActionView(Profile* profile, TabStripModel* tab_strip_model);
  BraveTalkShareTabActionView(const BraveTalkShareTabActionView&) = delete;
  BraveTalkShareTabActionView& operator=(const BraveTalkShareTabActionView&) = delete;
  ~BraveTalkShareTabActionView() override;

  // BraveTalkService::BraveTalkServiceObserver:
  void OnIsRequestingChanged(bool requesting) override;

  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override;

 private:
  void ButtonPressed();

  raw_ptr<Profile> profile_;
  raw_ptr<TabStripModel> tab_strip_model_;
  raw_ptr<BraveTalkService> brave_talk_service_;
};
}  // namespace brave_talk

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_TALK_SHARE_TAB_VIEW_H_
