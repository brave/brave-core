/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <cstring>
#include <iostream>
#include "wrapper.h"

size_t num_passed = 0;
size_t num_failed = 0;

const unsigned char ad_banner_dat_buffer[] = {
    31,  139, 8,   0,   0,   0,   0,   0,   0,   255, 1,   68,  0,
    187, 255, 155, 145, 128, 145, 128, 145, 128, 145, 128, 145, 128,
    145, 129, 207, 202, 167, 36,  217, 43,  56,  97,  176, 145, 158,
    145, 206, 0,   3,   31,  255, 146, 1,   145, 169, 97,  100, 45,
    98,  97,  110, 110, 101, 114, 192, 192, 192, 192, 192, 192, 192,
    192, 207, 186, 136, 69,  13,  115, 187, 170, 226, 192, 192, 192,
    144, 194, 195, 194, 195, 207, 77,  26,  78,  68,  0,   0,   0};

const unsigned char ad_banner_with_tag_abc_dat_buffer[] = {
    31,  139, 8,   0,   0,   0,   0,   0,   0,   255, 149, 139, 49,  14,
    64,  48,  24,  70,  137, 131, 88,  108, 98,  148, 184, 135, 19,  252,
    197, 218, 132, 3,   8,   139, 85,  126, 171, 132, 193, 32,  54,  71,
    104, 218, 205, 160, 139, 197, 105, 218, 166, 233, 5,   250, 125, 219,
    203, 123, 43,  14,  238, 163, 124, 206, 228, 79,  11,  184, 113, 195,
    55,  136, 98,  181, 132, 120, 65,  157, 17,  160, 180, 233, 152, 221,
    1,   164, 98,  178, 255, 242, 178, 221, 231, 201, 0,   19,  122, 216,
    92,  112, 161, 1,   58,  213, 199, 143, 114, 0,   0,   0};

const unsigned char ad_banner_with_resources_abc_dat_buffer[] = {
    31,  139, 8,   0,   0,   0,   0,   0,   0,   255, 61,  139, 189, 10,  64,
    80,  28,  197, 201, 46,  229, 1,   44,  54,  201, 234, 117, 174, 143, 65,
    233, 18,  6,   35,  118, 229, 127, 103, 201, 230, 99,  146, 39,  184, 177,
    25,  152, 61,  13,  238, 29,  156, 83,  167, 211, 175, 115, 90,  40,  184,
    203, 235, 24,  244, 219, 176, 209, 2,   29,  156, 130, 164, 61,  68,  132,
    9,   121, 166, 131, 48,  246, 19,  74,  71,  28,  69,  113, 230, 231, 25,
    101, 186, 42,  121, 86,  73,  189, 42,  95,  103, 255, 102, 219, 183, 29,
    170, 127, 68,  102, 150, 86,  28,  162, 0,   247, 3,   163, 110, 154, 146,
    145, 195, 175, 245, 47,  101, 250, 113, 201, 119, 0,   0,   0};

void Assert(bool value, const std::string& message) {
  if (!value) {
    std::cout << "Failed!" << std::endl;
    std::cout << message << std::endl;
  }
  assert(value);
}

void Check(bool expected_result,
           bool expected_did_match_exception,
           bool expected_did_match_important,
           std::string expected_redirect,
           const std::string& test_description,
           adblock::Engine* engine,
           const std::string& url,
           const std::string& host,
           const std::string& tab_host,
           bool third_party,
           const std::string& resource_type) {
  bool did_match_exception = false;
  bool did_match_important = false;
  bool did_match_rule = false;
  std::string redirect;
  engine->matches(url, host, tab_host, third_party, resource_type,
                  &did_match_rule, &did_match_exception, &did_match_important,
                  &redirect);
  std::cout << test_description << "... ";
  if (expected_result != did_match_rule) {
    std::cout << "Failed!" << std::endl;
    std::cout << "Unexpected result: " << url << " in " << tab_host
              << std::endl;
    num_failed++;
  } else if (did_match_exception != expected_did_match_exception) {
    std::cout << "Failed!" << std::endl;
    std::cout << "Unexpected did match exception value: " << url << " in "
              << tab_host << std::endl;
  } else if (did_match_important != expected_did_match_important) {
    std::cout << "Failed!" << std::endl;
    std::cout << "Unexpected did match important value: " << url << " in "
              << tab_host << std::endl;
  } else {
    std::cout << "Passed!" << std::endl;
    num_passed++;
  }
  assert(expected_result == did_match_rule);
  assert(did_match_exception == expected_did_match_exception);
  assert(did_match_important == expected_did_match_important);
  assert(redirect == expected_redirect);
}

void TestBasics() {
  adblock::Engine engine(
      "-advertisement-icon.\n"
      "-advertisement-management\n"
      "-advertisement.\n"
      "-advertisement/script.\n"
      "@@good-advertisement\n");
  Check(true, false, false, "", "Basic match", &engine,
        "http://example.com/-advertisement-icon.", "example.com", "example.com",
        false, "image");
  Check(false, false, false, "", "Basic not match", &engine,
        "https://brianbondy.com", "brianbondy.com", "example.com", true,
        "image");
  Check(false, true, false, "", "Basic saved from exception", &engine,
        "http://example.com/good-advertisement-icon.", "example.com",
        "example.com", false, "image");
}

void TestDeserialization() {
  adblock::Engine engine("");
  engine.deserialize(
      reinterpret_cast<const char*>(ad_banner_dat_buffer),
      sizeof(ad_banner_dat_buffer) / sizeof(ad_banner_dat_buffer[0]));
  Check(true, false, false, "", "Basic match after deserialization", &engine,
        "http://example.com/ad-banner.gif", "example.com", "example.com", false,
        "image");

  adblock::Engine engine2("");
  engine2.deserialize(
      reinterpret_cast<const char*>(ad_banner_with_tag_abc_dat_buffer),
      sizeof(ad_banner_with_tag_abc_dat_buffer) /
          sizeof(ad_banner_with_tag_abc_dat_buffer[0]));
  Check(false, false, false, "",
        "Basic match after deserialization for a buffer with tags and no tag "
        "match",
        &engine2, "http://example.com/ad-banner.gif", "example.com",
        "example.com", false, "image");
  engine2.addTag("abc");
  Check(true, false, false, "",
        "Basic match after deserialization for a buffer with tags and a tag "
        "match",
        &engine2, "http://example.com/ad-banner.gif", "example.com",
        "example.com", false, "image");

  // Deserialize after adding tag still works
  adblock::Engine engine3("");
  engine3.addTag("abc");
  engine3.deserialize(
      reinterpret_cast<const char*>(ad_banner_with_tag_abc_dat_buffer),
      sizeof(ad_banner_with_tag_abc_dat_buffer) /
          sizeof(ad_banner_with_tag_abc_dat_buffer[0]));
  Check(true, false, false, "",
        "Basic match after deserialization with resources with a tag on the "
        "engine before",
        &engine3, "http://example.com/ad-banner.gif", "example.com",
        "example.com", false, "image");

  adblock::Engine engine4("");
  engine4.deserialize(
      reinterpret_cast<const char*>(ad_banner_with_resources_abc_dat_buffer),
      sizeof(ad_banner_with_resources_abc_dat_buffer) /
          sizeof(ad_banner_with_resources_abc_dat_buffer[0]));
  Check(true, false, false, "data:text/plain;base64,",
        "Basic match after deserialization with resources", &engine4,
        "http://example.com/ad-banner.gif", "example.com", "example.com", false,
        "image");
}

void TestTags() {
  adblock::Engine engine(
      "-advertisement-icon.$tag=abc\n"
      "-advertisement-management$tag=abc\n"
      "-advertisement.$tag=abc\n"
      "-advertisement/script.$tag=abc\n");
  Check(false, false, false, "", "Without needed tags", &engine,
        "http://example.com/-advertisement-icon.", "example.com", "example.com",
        false, "image");
  engine.addTag("abc");
  Assert(engine.tagExists("abc"), "abc tag should exist");
  Assert(!engine.tagExists("abcd"), "abcd should not exist");
  Check(true, false, false, "", "With needed tags", &engine,
        "http://example.com/-advertisement-icon.", "example.com", "example.com",
        false, "image");
  // Adding a second tag doesn't clear the first.
  engine.addTag("hello");
  Check(true, false, false, "", "With extra unneeded tags", &engine,
        "http://example.com/-advertisement-icon.", "example.com", "example.com",
        false, "image");
  engine.removeTag("abc");
  Check(false, false, false, "", "With removed tags", &engine,
        "http://example.com/-advertisement-icon.", "example.com", "example.com",
        false, "image");
}

void TestRedirects() {
  adblock::Engine engine("-advertisement-$redirect=1x1-transparent.gif\n");
  engine.addResources(
      "[{\"name\": \"1x1-transparent.gif\","
      "\"aliases\": [],"
      "\"kind\": {\"mime\": \"image/gif\"},"
      "\"content\":\"R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==\"}]");
  Check(true, false, false,
        "data:image/"
        "gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==",
        "Testing redirects match", &engine,
        "http://example.com/-advertisement-icon.", "example.com", "example.com",
        false, "image");
}

void TestRedirect() {
  adblock::Engine engine("-advertisement-$redirect=test\n");
  engine.addResource("test", "application/javascript", "YWxlcnQoMSk=");
  Check(true, false, false, "data:application/javascript;base64,YWxlcnQoMSk=",
        "Testing single redirect match", &engine,
        "http://example.com/-advertisement-icon.", "example.com", "example.com",
        false, "image");
}

void TestThirdParty() {
  adblock::Engine engine("-advertisement-icon$third-party");
  Check(true, false, false, "", "Without needed tags", &engine,
        "http://example.com/-advertisement-icon", "example.com",
        "brianbondy.com", true, "image");
  Check(false, false, false, "", "Without needed tags", &engine,
        "http://example.com/-advertisement-icon", "example.com", "example.com",
        false, "image");
}

void TestImportant() {
  adblock::Engine engine(
      "-advertisement-icon$important\n"
      "@@-advertisement-icon-good\n");
  Check(true, false, true, "", "Exactly matching important rule", &engine,
        "http://example.com/-advertisement-icon", "example.com", "example.com",
        false, "image");
  Check(true, false, true, "", "Matching exception rule and important rule",
        &engine, "http://example.com/-advertisement-icon-good", "example.com",
        "example.com", false, "image");
}

void TestException() {
  adblock::Engine engine("*banner.png\n");
  Check(true, false, false, "", "Without exception", &engine,
        "http://example.com/ad_banner.png", "example.com", "example.com", false,
        "image");

  adblock::Engine engine2("@@*ad_banner.png\n");
  Check(false, true, false, "", "With exception", &engine2,
        "http://example.com/ad_banner.png", "example.com", "example.com", false,
        "image");
}

void TestClassId() {
  adblock::Engine engine(
      "###element\n"
      "##.ads\n"
      "##.element\n"
      "###ads > #element\n"
      "##a[href^=\"test.com\"]\n"
      "###block\n"
      "###block + .child\n");
  std::vector<std::string> classes = std::vector<std::string>();
  std::vector<std::string> ids = std::vector<std::string>();
  std::vector<std::string> exceptions = std::vector<std::string>();
  std::string stylesheet =
      engine.hiddenClassIdSelectors(classes, ids, exceptions);
  assert(stylesheet == "[]");

  classes = std::vector<std::string>({"ads", "no-ads"});
  ids = std::vector<std::string>({"element"});
  exceptions = std::vector<std::string>();
  stylesheet = engine.hiddenClassIdSelectors(classes, ids, exceptions);
  assert(stylesheet == "[\".ads\",\"#element\"]");

  classes = std::vector<std::string>({"element", "a"});
  ids = std::vector<std::string>({"block", "ads", "a"});
  exceptions = std::vector<std::string>({"#block"});
  stylesheet = engine.hiddenClassIdSelectors(classes, ids, exceptions);
  assert(stylesheet ==
         "[\".element\",\"#block + .child\",\"#ads > #element\"]");

  // Classes and ids must be passed without the leading `.` or `#`, or they will
  // not be recognized
  classes = std::vector<std::string>({".element", ".a"});
  ids = std::vector<std::string>({"#block", "#ads", "#a"});
  exceptions = std::vector<std::string>({"block"});
  stylesheet = engine.hiddenClassIdSelectors(classes, ids, exceptions);
  assert(stylesheet == "[]");
}

void TestUrlCosmetics() {
  adblock::Engine engine(
      "a.com###element\n"
      "b.com##.ads\n"
      "##.block\n"
      "a.com#@#.block\n"
      "##a[href=\"b.com\"]\n"
      "b.*##div:style(background: #fff)\n");

  std::string a_resources = engine.urlCosmeticResources("https://a.com");
  std::string a_order1(
      R"({"hide_selectors":["a[href=\"b.com\"]","#element"],"style_selectors":{},"exceptions":[".block"],"injected_script":"","generichide":false})");
  std::string a_order2(
      R"({"hide_selectors":["#element","a[href=\"b.com\"]"],"style_selectors":{},"exceptions":[".block"],"injected_script":"","generichide":false})");
  assert(a_resources == a_order1 || a_resources == a_order2);

  std::string b_resources = engine.urlCosmeticResources("https://b.com");
  std::string b_order1(
      R"({"hide_selectors":["a[href=\"b.com\"]",".ads"],"style_selectors":{"div":["background: #fff"]},"exceptions":[],"injected_script":"","generichide":false})");
  std::string b_order2(
      R"({"hide_selectors":[".ads","a[href=\"b.com\"]"],"style_selectors":{"div":["background: #fff"]},"exceptions":[],"injected_script":"","generichide":false})");
  assert(b_resources == b_order1 || b_resources == b_order2);

  // The URL may include a path
  std::string path_b_resources =
      engine.urlCosmeticResources("https://b.com/index.html");
  assert(path_b_resources == b_order1 || path_b_resources == b_order2);

  // However, it must be a full URL, including scheme
  std::string bad_b_resources = engine.urlCosmeticResources("b.com");
  std::string bad_b_result(
      R"({"hide_selectors":[],"style_selectors":{},"exceptions":[],"injected_script":"","generichide":false})");
  assert(bad_b_resources == bad_b_result);
}

void TestSubdomainUrlCosmetics() {
  adblock::Engine engine(
      "a.co.uk##.element\n"
      "good.a.*#@#.element\n");

  std::string a_resources = engine.urlCosmeticResources("http://a.co.uk");
  std::string a_result(
      R"({"hide_selectors":[".element"],"style_selectors":{},"exceptions":[],"injected_script":"","generichide":false})");
  assert(a_resources == a_result);

  std::string bad_a_resources =
      engine.urlCosmeticResources("https://bad.a.co.uk");
  std::string bad_a_result(
      R"({"hide_selectors":[".element"],"style_selectors":{},"exceptions":[],"injected_script":"","generichide":false})");
  assert(bad_a_resources == bad_a_result);

  std::string good_a_resources =
      engine.urlCosmeticResources("https://good.a.co.uk");
  std::string good_a_result(
      R"({"hide_selectors":[],"style_selectors":{},"exceptions":[".element"],"injected_script":"","generichide":false})");
  assert(good_a_resources == good_a_result);

  std::string still_good_a_resources =
      engine.urlCosmeticResources("http://still.good.a.co.uk");
  std::string still_good_a_result(
      R"({"hide_selectors":[],"style_selectors":{},"exceptions":[".element"],"injected_script":"","generichide":false})");
  assert(still_good_a_resources == still_good_a_result);
}

void TestCosmeticScriptletResources() {
  adblock::Engine engine(
      "a.com##+js(scriptlet1)\n"
      "2.a.com##+js(scriptlet2.js, argument)\n");

  std::string a_unloaded = engine.urlCosmeticResources("https://a.com");
  std::string a_unloaded_result(
      R"({"hide_selectors":[],"style_selectors":{},"exceptions":[],"injected_script":"","generichide":false})");
  assert(a_unloaded == a_unloaded_result);

  engine.addResources(R"([
      {"name": "basic_scriptlet", "aliases": ["scriptlet1"], "kind": { "mime": "application/javascript" }, "content": "Y29uc29sZS5sb2coIkhpIik7" },
      {"name": "scriptlet2", "aliases": [], "kind": "template", "content": "d2luZG93LmxvY2F0aW9uLmhyZWYgPSAie3sxfX0i" }]
  )");

  std::string a_loaded = engine.urlCosmeticResources("https://a.com");
  std::string a_loaded_result(
      R"({"hide_selectors":[],"style_selectors":{},"exceptions":[],"injected_script":"console.log(\"Hi\");\n","generichide":false})");
  assert(a_loaded == a_loaded_result);

  std::string a2_loaded = engine.urlCosmeticResources("https://2.a.com");
  std::string a2_loaded_result(
      R"({"hide_selectors":[],"style_selectors":{},"exceptions":[],"injected_script":"console.log(\"Hi\");\nwindow.location.href = \"argument\"\n","generichide":false})");
  assert(a2_loaded == a2_loaded_result);
}

void TestGenerichide() {
  adblock::Engine engine(
      "##a[href=\"generic.com\"]\n"
      "@@||b.com$generichide\n"
      "b.com##.block\n"
      "##.block\n"
      "@@||a.com/test.html$generichide\n"
      "a.com##.block\n");

  std::string b_resources = engine.urlCosmeticResources("https://b.com");
  std::string b_result(
      R"({"hide_selectors":[".block"],"style_selectors":{},"exceptions":[],"injected_script":"","generichide":true})");
  assert(b_resources == b_result);

  std::string b_path_resources =
      engine.urlCosmeticResources("https://b.com/test.html");
  std::string b_path_result(
      R"({"hide_selectors":[".block"],"style_selectors":{},"exceptions":[],"injected_script":"","generichide":true})");
  assert(b_path_resources == b_path_result);

  std::string a_resources = engine.urlCosmeticResources("https://a.com");
  std::string a_order1(
      R"({"hide_selectors":[".block","a[href=\"generic.com\"]"],"style_selectors":{},"exceptions":[],"injected_script":"","generichide":false})");
  std::string a_order2(
      R"({"hide_selectors":["a[href=\"generic.com\"]",".block"],"style_selectors":{},"exceptions":[],"injected_script":"","generichide":false})");
  assert(a_resources == a_order1 || a_resources == a_order2);

  std::string a_path_resources =
      engine.urlCosmeticResources("https://a.com/test.html");
  std::string a_path_result(
      R"({"hide_selectors":[".block"],"style_selectors":{},"exceptions":[],"injected_script":"","generichide":true})");
  assert(a_path_resources == a_path_result);
}

// Naive domain resolution implementation. Assumes the hostname == the domain,
// other than the few explicitly listed exceptional cases.
void domainResolverImpl(const char* host, uint32_t* start, uint32_t* end) {
  if (!strcmp(host, "bad.a.co.uk")) {
    *start = 4;
  } else if (!strcmp(host, "good.a.co.uk")) {
    *start = 5;
  } else if (!strcmp(host, "still.good.a.co.uk")) {
    *start = 11;
  } else if (!strcmp(host, "2.a.com")) {
    *start = 2;
  } else {
    *start = 0;
  }
  *end = strlen(host);
}

int main() {
  adblock::SetDomainResolver(domainResolverImpl);

  TestBasics();
  TestDeserialization();
  TestTags();
  TestRedirects();
  TestRedirect();
  TestThirdParty();
  TestImportant();
  TestException();
  TestClassId();
  TestUrlCosmetics();
  TestSubdomainUrlCosmetics();
  TestGenerichide();
  TestCosmeticScriptletResources();
  std::cout << num_passed << " passed, " << num_failed << " failed"
            << std::endl;
  std::cout << "Success!";
  return 0;
}
