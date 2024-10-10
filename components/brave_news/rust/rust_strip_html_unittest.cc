// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/rust/lib.rs.h"
#include "testing/gtest/include/gtest/gtest.h"

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
  auto result = Strip(
      R"(<this is a bunch of text>|<pfg somran>Foo</ fwe span>|< fawefaewf>)");
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

TEST(RustStripHtml, CantCloseATagFromInsideAComment) {
  EXPECT_EQ("Content",
            Strip("<dialog <!-- sneaky close tag > --> open=false>Content"));
}

TEST(RustStripHtml, CDataIsStripped) {
  EXPECT_EQ("Some", Strip("Some<![CDATA[<p>foo</p>]]"));
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

TEST(RustStripHtml, HtmlEntities) {
  EXPECT_EQ("&lt;pThis is paragraph an HTML entity.",
            Strip("&lt;p>This is paragraph an HTML entity.</p>"));
}

// ---------------------------
// Some test data from the voca_rs project
// https://github.com/a-merezhanyi/voca_rs/blob/master/tests/unit/strip.rs
//
// MIT License
//
// Copyright (c) 2018-2022 A. Merezhanyi
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// 1. The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// 2. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 3. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ## Acknowledgements:
// - Voca.js https://github.com/panzerdp/voca `Copyright (c) 2017 Dmitri
// Pavlutin`
// - Unidecode https://github.com/chowdhurya/rust-unidecode `Copyright (c) 2015,
// Amit Chowdhury`
// - heck https://github.com/withoutboats/heck `Copyright (c) 2018, Saoirse
// Shipwreckt`
// - Inflector https://github.com/whatisinternet/inflector `Copyright (c) 2019,
// Josh Teeter`
// - Graphite Helpers https://github.com/GrafiteInc/Helpers `Copyright (c) 2020,
// Matt Lantz`
// ---------------------------

TEST(RustStripHtml, VocaGeneral) {
  EXPECT_EQ(Strip(""), "");
  EXPECT_EQ(Strip("Hello world!"), "Hello world!");
  EXPECT_EQ(Strip("  "), "  ");
  // should strip tags
  EXPECT_EQ(Strip("<span><a href=\"#\">Summer</a> is nice</span>"),
            "Summer is nice");
  EXPECT_EQ(Strip("<b>Hello world!</b>"), "Hello world!");
  EXPECT_EQ(Strip("<span class=\"italic\"><b>Hello world!</b></span>"),
            "Hello world!");
  EXPECT_EQ(Strip("<span class='<italic>'>Hello world!</span>"),
            "Hello world!");
  EXPECT_EQ(Strip("<script language=\"PHP\"> echo hello </script>"),
            " echo hello ");
  // should strip tags which attributes contain < or >
  EXPECT_EQ(Strip("hello <img title='>_<'> world"), "hello _ world");
  EXPECT_EQ(Strip("hello <img title=\"<\"> world"), "hello ");
  EXPECT_EQ(Strip("hello <img title=\"<foo/> <'bar'\"> world"), "hello ");
  // should strip tags on multiple lines
  EXPECT_EQ(
      Strip("This's a string with quotes:</html>\n\"strings in double "
            "quote\";\n'strings in single quote\';\n<html>this\\line is single "
            "quoted /with\\slashes"),
      "This\'s a string with quotes:\n\"strings in double quote\";\n\'strings "
      "in single quote\';\nthis\\line is single quoted /with\\slashes");
  // should strip comments and doctype
  EXPECT_EQ(Strip("<html><!-- COMMENT --></html>"), "");
  EXPECT_EQ(Strip("<b>Hello world!</b><!-- Just some information -->"),
            "Hello world!");
  EXPECT_EQ(Strip("<span class=\"italic\">Hello world!<!-- Just some "
                  "information --></span>"),
            "Hello world!");
  EXPECT_EQ(
      Strip("<!-- Small<>comment --><span class=\"italic\"><!-- Just some "
            "information --><b>Hello world!</b></span>"),
      "Hello world!");
  EXPECT_EQ(Strip("<!doctype html><span class=\"italic\"><!-- Just some "
                  "information --><b>Hello world!</b></span>"),
            "Hello world!");
}

TEST(RustStripHtml, VocaUser) {
  EXPECT_EQ(
      Strip("<span style=\"color: rgb(51, 51, 51); font-family: \" "
            "microsoft=\"\" yahei=\"\" stheiti=\"\" wenquanyi=\"\" micro=\"\" "
            "hei=\"\" simsun=\"\" sans-serif=\"\" font-size:=\"\" "
            "16px=\"\">】มีมี่’ เด็กสาวที่นอนไม่ค่อยหลับเนื่องจากกลัวผี ขี้เหงา และอะไรหลายๆ "
            "อย่างทำให้เธอมึนได้โล่เพราะไม่ค่อยได้นอน การที่เธอ นอนไม่หลับทำให้เธอได้เจอกับ "
            "‘ดีเจไททัน’ แห่งคลื่น 99.99 MHzเขาจัดรายการในช่วง Midnight Fantasy "
            "ตีสามถึงตีห้า "
            "และมีมี่ก็เป็นผู้ฟังเพียงคนเดียวของเขาจากที่ตอนแรกเธอฟังดีเจไททันเพื่อช่วยปลอบประโลม"
            "การที่เธอต้องมาอยู่หอเพียงลำพัง แต่ไปๆ "
            "มาๆกลับกลายเป็นว่าเธออยู่รอฟังเขาทุกคืนทำให้เธอไปเรียนแบบมึนๆ "
            "จนบังเอิญไปนอนหลับซบ ‘ธรรม’ผู้ชายจอมกวนที่บังเอิญมานอนให้เธอซบ! "
            "จนอาจารย์สั่งให้ไปทำรายงานคู่กัน "
            "และนั่นก็เป็นที่มาของการที่เธอเริ่มไม่แน่ใจแล้วว่าเธอปลื้มดีเจไททัน "
            "หรือแอบหวั่นไหวกับนายจอมกวนคนนี้กันแน่</span><br />"),
      "】มีมี่’ เด็กสาวที่นอนไม่ค่อยหลับเนื่องจากกลัวผี ขี้เหงา และอะไรหลายๆ "
      "อย่างทำให้เธอมึนได้โล่เพราะไม่ค่อยได้นอน การที่เธอ นอนไม่หลับทำให้เธอได้เจอกับ "
      "‘ดีเจไททัน’ แห่งคลื่น 99.99 MHzเขาจัดรายการในช่วง Midnight Fantasy ตีสามถึงตีห้า "
      "และมีมี่ก็เป็นผู้ฟังเพียงคนเดียวของเขาจากที่ตอนแรกเธอฟังดีเจไททันเพื่อช่วยปลอบประโลมการที่เธ"
      "อต้องมาอยู่หอเพียงลำพัง แต่ไปๆ "
      "มาๆกลับกลายเป็นว่าเธออยู่รอฟังเขาทุกคืนทำให้เธอไปเรียนแบบมึนๆ จนบังเอิญไปนอนหลับซบ "
      "‘ธรรม’ผู้ชายจอมกวนที่บังเอิญมานอนให้เธอซบ! จนอาจารย์สั่งให้ไปทำรายงานคู่กัน "
      "และนั่นก็เป็นที่มาของการที่เธอเริ่มไม่แน่ใจแล้วว่าเธอปลื้มดีเจไททัน "
      "หรือแอบหวั่นไหวกับนายจอมกวนคนนี้กันแน่");
}

TEST(RustStripHtml, VocaSpecial) {
  EXPECT_EQ(Strip("< html >"), "");
  EXPECT_EQ(Strip("<<>>"), "");
  EXPECT_EQ(Strip("<a.>HtMl text</.a>"), "HtMl text");
  EXPECT_EQ(Strip("<abc>hello</abc> \t\tworld... <ppp>strip_tags_test</ppp>"),
            "hello \t\tworld... strip_tags_test");
  EXPECT_EQ(Strip("<html><b>hello</b><p>world</p></html>"), "helloworld");
  EXPECT_EQ(Strip("<span class=\"italic\"><b>He>llo</b> < world!</span>"),
            "Hello ");
  // should handle unicode
  EXPECT_EQ(Strip("<SCRIPT>Ω≈ç≈≈Ω</SCRIPT>"), "Ω≈ç≈≈Ω");
  EXPECT_EQ(Strip("<SCRIPT a=\"blah\">片仮名平仮名</SCRIPT>"), "片仮名平仮名");
  EXPECT_EQ(Strip("<!-- testing --><a>text here</a>"), "text here");
}

TEST(RustStripHtml, VocaXSSTests) {
  EXPECT_EQ(Strip("<img "
                  "src=\"data:image/gif;base64,R0lGODlhAQABAIAAAP///"
                  "wAAACwAAAAAA‌\u{200B}QABAAACAkQBADs=\"onload=\"$."
                  "getScript('evil.js');1<2>3\">"),
            "");
  EXPECT_EQ(Strip("<script>evil();</script>"), "evil();");
  EXPECT_EQ(Strip("<SCRIPT SRC=http://xss.rocks/xss.js></SCRIPT>"), "");
  EXPECT_EQ(Strip("<IMG \"\"\"><SCRIPT>alert(\"XSS\")</SCRIPT>\">"),
            "alert(\"XSS\")\"");
  EXPECT_EQ(Strip("<SCRIPT/XSS SRC=\"http://xss.rocks/xss.js\"></SCRIPT>"), "");
  EXPECT_EQ(Strip("<BODY onload!#$%&()*~+-_.,:;?@[/|\\]^`=alert(\"XSS\")>"),
            "");
  EXPECT_EQ(Strip("<SCRIPT/SRC=\"http://xss.rocks/xss.js\"></SCRIPT>"), "");
  EXPECT_EQ(Strip("<<SCRIPT>alert(\"XSS\");//<</SCRIPT>"), "");
  EXPECT_EQ(Strip("<SCRIPT SRC=http://xss.rocks/xss.js?< B >"), "");
  EXPECT_EQ(Strip("<SCRIPT SRC=//xss.rocks/.j>"), "");
  EXPECT_EQ(Strip("<IMG SRC=\"javascript:alert(\'XSS\')\""), "");
  EXPECT_EQ(Strip("<SCRIPT a=\">\" SRC=\"httx://xss.rocks/xss.js\"></SCRIPT>"),
            "\" SRC=\"httx://xss.rocks/xss.js\"");
  EXPECT_EQ(Strip("<SCRIPT =\">\" SRC=\"httx://xss.rocks/xss.js\"></SCRIPT>"),
            "\" SRC=\"httx://xss.rocks/xss.js\"");
  EXPECT_EQ(
      Strip("<SCRIPT a=\">\" \'\' SRC=\"httx://xss.rocks/xss.js\"></SCRIPT>"),
      "\" '' SRC=\"httx://xss.rocks/xss.js\"");
  EXPECT_EQ(
      Strip("<SCRIPT \"a=\'>\'\" SRC=\"httx://xss.rocks/xss.js\"></SCRIPT>"),
      "'\" SRC=\"httx://xss.rocks/xss.js\"");
  EXPECT_EQ(Strip("<SCRIPT a=`>` SRC=\"httx://xss.rocks/xss.js\"></SCRIPT>"),
            "` SRC=\"httx://xss.rocks/xss.js\"");
  EXPECT_EQ(
      Strip("<SCRIPT a=\">\'>\" SRC=\"httx://xss.rocks/xss.js\"></SCRIPT>"),
      "'\" SRC=\"httx://xss.rocks/xss.js\"");
  EXPECT_EQ(Strip("<SCRIPT>document.write(\"<SCRI\");</SCRIPT>PT "
                  "SRC=\"httx://xss.rocks/xss.js\"></SCRIPT>"),
            "document.write(\"");
}

TEST(RustStripHtml, VocaStripTags) {
  EXPECT_EQ(Strip("<span><a href=\"#\">Summer</a> is nice</span>"),
            "Summer is nice");
}

TEST(RustStripHtml, VocaPartialDirective) {
  EXPECT_EQ(Strip("<"), "");
  EXPECT_EQ(Strip("<t"), "");
  EXPECT_EQ(Strip("</"), "");
  EXPECT_EQ(Strip("</a"), "");
  EXPECT_EQ(Strip("<!"), "");
  EXPECT_EQ(Strip("<!-"), "");
  EXPECT_EQ(Strip("á<!"), "á");
  EXPECT_EQ(Strip(">天地不仁<"), "天地不仁");
  EXPECT_EQ(Strip("\u{00a0}<!"), "\u{a0}");
}
