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
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class GestureTextSelectionExtractorRenderViewTest
    : public content::RenderViewTest {
 public:
  GestureTextSelectionExtractorRenderViewTest() = default;
  ~GestureTextSelectionExtractorRenderViewTest() override = default;

  void SetUp() override {
    content::RenderViewTest::SetUp();
    registry_ = std::make_unique<service_manager::BinderRegistry>();
    extractor_ = std::make_unique<GestureTextSelectionExtractor>(
        GetMainRenderFrame(), registry_.get());
  }

  void TearDown() override {
    extractor_.reset();
    registry_.reset();
    content::RenderViewTest::TearDown();
  }

 protected:
  void SelectAllText(std::string_view body_html) {
    LoadHTML(base::StrCat({"<body>", body_html, "</body>"}));
    ExecuteJavaScriptForTests("document.execCommand('selectAll')");
  }

  std::string GetSelectedText() {
    base::test::TestFuture<const std::string&> future;
    extractor_->GetSelectedText(future.GetCallback());
    return future.Take();
  }

  // Connects to the extractor through the registry, as the browser does.
  mojo::Remote<mojom::GestureTextSelectionExtractor> BindExtractor() {
    mojo::Remote<mojom::GestureTextSelectionExtractor> remote;
    auto pipe = remote.BindNewPipeAndPassReceiver().PassPipe();
    EXPECT_TRUE(registry_->TryBindInterface(
        mojom::GestureTextSelectionExtractor::Name_, &pipe));
    return remote;
  }

 private:
  std::unique_ptr<service_manager::BinderRegistry> registry_;
  std::unique_ptr<GestureTextSelectionExtractor> extractor_;
};

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsSelectedTextFromFrame) {
  SelectAllText("Hello gestures");

  EXPECT_EQ("Hello gestures", GetSelectedText());
}

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsEmptyStringWhenNothingIsSelected) {
  LoadHTML("<body>Hello gestures</body>");

  EXPECT_EQ("", GetSelectedText());
}

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsEmptyStringOnABlankPage) {
  LoadHTML("<body></body>");

  EXPECT_EQ("", GetSelectedText());
}

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsOnlyTheSelectedSubstring) {
  LoadHTML(
      "<body><span id='a'>Hello</span> <span id='b'>gestures</span></body>");
  ExecuteJavaScriptForTests(
      "const range = document.createRange();"
      "range.selectNode(document.getElementById('a'));"
      "window.getSelection().removeAllRanges();"
      "window.getSelection().addRange(range);");

  EXPECT_EQ("Hello", GetSelectedText());
}

TEST_F(GestureTextSelectionExtractorRenderViewTest,
       ReturnsEmptyStringAfterSelectionIsCleared) {
  SelectAllText("Hello gestures");
  ASSERT_EQ("Hello gestures", GetSelectedText());

  ExecuteJavaScriptForTests("window.getSelection().removeAllRanges()");

  EXPECT_EQ("", GetSelectedText());
}

// Opening another connection must not drop a reply still pending on an
// existing one.
TEST_F(GestureTextSelectionExtractorRenderViewTest, SurvivesAnotherConnection) {
  SelectAllText("Hello gestures");

  auto extractor = BindExtractor();
  base::test::TestFuture<const std::string&> future;
  extractor->GetSelectedText(future.GetCallback());

  auto other_extractor = BindExtractor();

  EXPECT_EQ("Hello gestures", future.Take());
  EXPECT_TRUE(extractor.is_connected());
}

}  // namespace gestures
