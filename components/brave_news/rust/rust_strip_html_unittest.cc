// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/rust/lib.rs.h"

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

std::string Strip(const std::string& input) {
  return static_cast<std::string>(brave_news::strip_html(input));
}

TEST(RustStripHtml, Noop) {
  EXPECT_EQ("Foo", Strip("Foo"));
}

TEST(RustStripHtml, CanStripTags) {
  EXPECT_EQ("Foo", Strip("<p>Foo</p>"));
}

TEST(RustStripHtml, CanStripNestedTags) {
  EXPECT_EQ("Foo", Strip("<span><p>Foo</p></span>"));
}

TEST(RustStripHtml, AttributesAreStripped) {
  EXPECT_EQ("Foo", Strip("<span style='display: inline'>Foo<img "
                         "onerror='alert(\"haha\")'></img></span>"));
}

TEST(RustStripHtml, CanStripMismatchedTags) {
  auto result = Strip("<span>|<p>Foo</span>|</p>");
  EXPECT_EQ("|Foo|", result);
}

TEST(RustStripHtml, TagsCanCoverNewLines) {
  EXPECT_EQ("Foo", Strip("<dialog\nopen\n>Foo<\n/\ndialog>"));
}

TEST(RustStripHtml, NonsenseIsIgnored) {
  auto result = Strip(R"(<this is 
    a bunch of text>|<pfg somran>Foo</ fwe span>|< fawefaewf>)");
  EXPECT_EQ("|Foo|", result);
}

TEST(RustStripHtml, EvilAttributes) {
  auto result = Strip("<p attr='> <script>alert()</script>'>Foo</p>");
  EXPECT_EQ(" alert()'Foo", result);
}

TEST(RustStripHtml, CommentsAreStripped) {
  EXPECT_EQ("Foo", Strip("<p><!-- Hmm -->Foo</p>"));
}

TEST(RustStripHtml, CommentsCantCloseTag) {
  EXPECT_EQ("alert('hello')",
            Strip("<p><!--<script>-->alert('hello')<!--</script>--></p>"));
}

TEST(RustStripHtml, CantMakeATagWithComments) {
  EXPECT_EQ("foo", Strip("<<!--script-->>foo"));
}

TEST(RustStripHtml, CommentsCanCoverNewLines) {
  EXPECT_EQ("frob", Strip("<!-- foo\nbar\n-->frob"));
}

TEST(RustStripHtml, UnclosedComment) {
  EXPECT_EQ("", Strip("<p><!--\nHello WOrld\n foo </p>"));
}

TEST(RustStripHtml, UnclosedTag) {
  EXPECT_EQ("foo", Strip("foo<p bar \n<!-- thing\n --> stuff"));
}

TEST(RustStripHtml, NonsenseWithComments) {
  EXPECT_EQ("foo -- --  ", Strip(R"(foo<<!
  -- bar>
  --> --> --> </p> <p <!----> <! fawefg! awefg--> <p>
  </n>
  <\n>
  <whor --> <!-- </n> </dialog> <att ='foo'> ga4wet
  AWEGT
  </P>-->
  )"));
}
