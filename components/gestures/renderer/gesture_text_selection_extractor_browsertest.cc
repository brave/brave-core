// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/renderer/gesture_text_selection_extractor.h"

#include <memory>
#include <string>
#include <string_view>

#include "base/strings/strcat.h"
#include "base/test/test_future.h"
#include "brave/components/gestures/core/common/mojom/gesture_text_selection_extractor.mojom.h"
#include "content/public/test/render_view_test.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class GestureTextSelectionExtractorRenderViewTest
    : public content::RenderViewTest {
 public:
  GestureTextSelectionExtractorRenderViewTest() = default;
  ~GestureTextSelectionExtractorRenderViewTest() override = default;

  void SetUp() override {
    content::RenderViewTest::SetUp();
    binder_registry_ = std::make_unique<service_manager::BinderRegistry>();
    gesture_text_selection_extractor_ =
        std::make_unique<GestureTextSelectionExtractor>(GetMainRenderFrame(),
                                                        binder_registry_.get());
  }

  void TearDown() override {
    gesture_text_selection_extractor_.reset();
    binder_registry_.reset();
    content::RenderViewTest::TearDown();
  }

 protected:
  void SelectAllText(std::string_view body_html) {
    LoadHTML(base::StrCat({"<body>", body_html, "</body>"}));
    ExecuteJavaScriptForTests("document.execCommand('selectAll')");
  }

  std::string GetSelectedText() {
    base::test::TestFuture<const std::string&> test_future;
    gesture_text_selection_extractor_->GetSelectedText(
        test_future.GetCallback());
    return test_future.Take();
  }

  // Connects to the extractor through the registry, as the browser does.
  mojo::Remote<mojom::GestureTextSelectionExtractor> BindExtractor() {
    mojo::Remote<mojom::GestureTextSelectionExtractor>
        gesture_text_selection_extractor_remote;
    mojo::ScopedMessagePipeHandle pipe =
        gesture_text_selection_extractor_remote.BindNewPipeAndPassReceiver()
            .PassPipe();
    EXPECT_TRUE(binder_registry_->TryBindInterface(
        mojom::GestureTextSelectionExtractor::Name_, &pipe));
    return gesture_text_selection_extractor_remote;
  }

 private:
  std::unique_ptr<service_manager::BinderRegistry> binder_registry_;
  std::unique_ptr<GestureTextSelectionExtractor>
      gesture_text_selection_extractor_;
};

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsSelectedTextFromFrame) {
  SelectAllText("foo bar");

  EXPECT_EQ("foo bar", GetSelectedText());
}

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsEmptyStringWhenNothingIsSelected) {
  LoadHTML("<body>foo bar</body>");

  EXPECT_THAT(GetSelectedText(), testing::IsEmpty());
}

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsEmptyStringOnABlankPage) {
  LoadHTML("<body></body>");

  EXPECT_THAT(GetSelectedText(), testing::IsEmpty());
}

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsOnlyTheSelectedSubstring) {
  LoadHTML("<body><span id='a'>foo</span> <span id='b'>bar</span></body>");
  ExecuteJavaScriptForTests(
      "const range = document.createRange();"
      "range.selectNode(document.getElementById('a'));"
      "window.getSelection().removeAllRanges();"
      "window.getSelection().addRange(range);");

  EXPECT_EQ("foo", GetSelectedText());
}

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsEmptyStringAfterSelectionIsCleared) {
  SelectAllText("foo bar");
  ASSERT_EQ("foo bar", GetSelectedText());

  ExecuteJavaScriptForTests("window.getSelection().removeAllRanges()");

  EXPECT_THAT(GetSelectedText(), testing::IsEmpty());
}

// Opening another connection must not drop a reply still pending on an
// existing one.
TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ContinuesExtractingTextAfterAnotherConnection) {
  SelectAllText("foo bar");

  auto extractor = BindExtractor();
  base::test::TestFuture<const std::string&> test_future;
  extractor->GetSelectedText(test_future.GetCallback());

  auto other_extractor = BindExtractor();

  EXPECT_EQ("foo bar", test_future.Take());
  EXPECT_TRUE(extractor.is_connected());
}

}  // namespace gestures
