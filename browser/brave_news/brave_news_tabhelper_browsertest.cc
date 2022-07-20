
#include <vector>
#include "absl/types/optional.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"

class WaitForFeedsChanged : public BraveNewsTabHelper::PageFeedsObserver {
 public:
  WaitForFeedsChanged(BraveNewsTabHelper* tab_helper)
      : tab_helper_(tab_helper) {
    tab_helper_->AddObserver(this);
  }

  ~WaitForFeedsChanged() { tab_helper_->RemoveObserver(this); }

  std::vector<BraveNewsTabHelper::FeedDetails> WaitForChange() {
    if (!last_feeds_)
      loop_.Run();
    return last_feeds_.value();
  }

 private:
  void OnAvailableFeedsChanged(
      const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) override {
    last_feeds_ = feeds;
    loop_.Quit();
  }

  base::RunLoop loop_;
  raw_ptr<BraveNewsTabHelper> tab_helper_;
  absl::optional<std::vector<BraveNewsTabHelper::FeedDetails>> last_feeds_ =
      absl::nullopt;
};

class BraveNewsTabHelperTest : public InProcessBrowserTest {
 public:
  BraveNewsTabHelperTest() {
    features_.InitWithFeatures(
        {brave_today::features::kBraveNewsSubscribeButtonFeature}, {});
  }

  void SetUp() override { InProcessBrowserTest::SetUp(); }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  base::test::ScopedFeatureList features_;
};

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest, TabHelperIsCreated) {
  EXPECT_NE(nullptr, BraveNewsTabHelper::FromWebContents(contents()));
}

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest,
                       TabHelperNotifiesObserversWhenFoundFeeds) {
  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());
  WaitForFeedsChanged waiter(tab_helper);

  std::vector<brave_news::mojom::FeedSearchResultItemPtr> feeds;
  feeds.emplace_back(brave_news::mojom::FeedSearchResultItem::New());
  feeds.emplace_back(brave_news::mojom::FeedSearchResultItem::New());
  tab_helper->OnFoundFeeds(contents()->GetLastCommittedURL(),
                           std::move(feeds));

  auto result = waiter.WaitForChange();
  EXPECT_EQ(2u, result.size());
}
