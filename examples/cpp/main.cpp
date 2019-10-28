#include <iostream>
#include <assert.h>
#include "wrapper.hpp"

using namespace std;
using namespace adblock;

size_t num_passed = 0;
size_t num_failed = 0;

const unsigned char ad_banner_dat_buffer[] = {
  31, 139, 8, 0, 0, 0, 0, 0,
  0, 255, 1, 68, 0, 187, 255, 155,
  145, 128, 145, 128, 145, 128, 145, 128,
  145, 128, 145, 129, 207, 202, 167, 36,
  217, 43, 56, 97, 176, 145, 158, 145,
  206, 0, 3, 31, 255, 146, 1, 145,
  169, 97, 100, 45, 98, 97, 110, 110,
  101, 114, 192, 192, 192, 192, 192, 192,
  192, 192, 207, 186, 136, 69, 13, 115,
  187, 170, 226, 192, 192, 192, 144, 194,
  195, 194, 195, 207, 77, 26, 78, 68,
  0, 0, 0
};

const unsigned char ad_banner_with_tag_abc_dat_buffer[] = {
  31, 139, 8, 0, 0, 0, 0, 0,
  0, 255, 149, 139, 49, 14, 64, 48,
  24, 70, 137, 131, 88, 108, 98, 148,
  184, 135, 19, 252, 197, 218, 132, 3,
  8, 139, 85, 126, 171, 132, 193, 32,
  54, 71, 104, 218, 205, 160, 139, 197,
  105, 218, 166, 233, 5, 250, 125, 219,
  203, 123, 43, 14, 238, 163, 124, 206,
  228, 79, 11, 184, 113, 195, 55, 136,
  98, 181, 132, 120, 65, 157, 17, 160,
  180, 233, 152, 221, 1, 164, 98, 178,
  255, 242, 178, 221, 231, 201, 0, 19,
  122, 216, 92, 112, 161, 1, 58, 213,
  199, 143, 114, 0, 0, 0
};


const unsigned char ad_banner_with_resources_abc_dat_buffer[] = {
  31, 139, 8, 0, 0, 0, 0, 0,
  0, 255, 61, 139, 189, 10, 64, 80,
  28, 197, 201, 46, 229, 1, 44, 54,
  201, 234, 117, 174, 143, 65, 233, 18,
  6, 35, 118, 229, 127, 103, 201, 230,
  99, 146, 39, 184, 177, 25, 152, 61,
  13, 238, 29, 156, 83, 167, 211, 175,
  115, 90, 40, 184, 203, 235, 24, 244,
  219, 176, 209, 2, 29, 156, 130, 164,
  61, 68, 132, 9, 121, 166, 131, 48,
  246, 19, 74, 71, 28, 69, 113, 230,
  231, 25, 101, 186, 42, 121, 86, 73,
  189, 42, 95, 103, 255, 102, 219, 183,
  29, 170, 127, 68, 102, 150, 86, 28,
  162, 0, 247, 3, 163, 110, 154, 146,
  145, 195, 175, 245, 47, 101, 250, 113,
  201, 119, 0, 0, 0
};

void Assert(bool value, const std::string& message) {
  if (!value) {
    cout << "Failed!" << endl;
    cout << message << endl;
  }
  assert(value);
}

void Check(bool expected_result,
    bool expected_cancel,
    bool expected_saved_from_exception,
    std::string expected_redirect,
    const std::string& test_description,
    Engine& engine, const std::string& url, const std::string& host,
    const std::string& tab_host, bool third_party,
    const std::string& resource_type) {
  bool cancel;
  bool saved_from_exception;
  std::string redirect;
  bool match = engine.matches(url, host, tab_host, third_party,
      resource_type, &cancel, &saved_from_exception, &redirect);
  cout << test_description << "... ";
  if (expected_result != match) {
    cout << "Failed!" << endl;
    cout << "Unexpected result: " << url << " in " << tab_host << endl;
    num_failed++;
  } else if (cancel != expected_cancel) {
    cout << "Failed!" << endl;
    cout << "Unexpected cancel value: " << url <<
      " in " << tab_host << endl;
  } else if (saved_from_exception != expected_saved_from_exception) {
    cout << "Failed!" << endl;
    cout << "Unexpected saved from exception value: " << url <<
      " in " << tab_host << endl;
  } else {
    cout << "Passed!" << endl;
    num_passed++;
  }
  assert(expected_result == match);
  assert(cancel == expected_cancel);
  assert(saved_from_exception == expected_saved_from_exception);
  assert(redirect == expected_redirect);
}

void TestBasics() {
  Engine engine("-advertisement-icon.\n"
                "-advertisement-management\n"
                "-advertisement.\n"
                "-advertisement/script.\n"
                "@@good-advertisement\n"
                );
  Check(true, false, false, "", "Basic match", engine, "http://example.com/-advertisement-icon.",
      "example.com", "example.com", false , "image");
  Check(false, false, false, "", "Basic not match", engine, "https://brianbondy.com",
      "brianbondy.com", "example.com", true, "image");
  Check(false, false, true, "", "Basic saved from exception", engine, "http://example.com/good-advertisement-icon.",
      "example.com", "example.com", false, "image");
}

void TestAddingFilters() {
  Engine engine("");
  engine.addFilter("-advertisement-icon.");
  engine.addFilter("-advertisement-management");
  engine.addFilter("-advertisement.");
  engine.addFilter("-advertisement/script.");
  engine.addFilter("@@good-advertisement");
  Check(true, false, false, "", "Basic match", engine, "http://example.com/-advertisement-icon.",
      "example.com", "example.com", false , "image");
  Check(false, false, false, "", "Basic not match", engine, "https://brianbondy.com",
      "brianbondy.com", "example.com", true, "image");
  Check(false, false, true, "", "Basic saved from exception", engine, "http://example.com/good-advertisement-icon.",
      "example.com", "example.com", false, "image");
}

void TestDeserialization() {
  Engine engine("");
  engine.deserialize(reinterpret_cast<const char*>(ad_banner_dat_buffer),
      sizeof(ad_banner_dat_buffer)/sizeof(ad_banner_dat_buffer[0]));
  Check(true, false, false, "", "Basic match after deserialization", engine, "http://example.com/ad-banner.gif",
      "example.com", "example.com", false , "image");

  Engine engine2("");
  engine2.deserialize(reinterpret_cast<const char*>(ad_banner_with_tag_abc_dat_buffer),
      sizeof(ad_banner_with_tag_abc_dat_buffer)/sizeof(ad_banner_with_tag_abc_dat_buffer[0]));
  Check(false, false, false, "", "Basic match after deserialization for a buffer with tags and no tag match", engine2, "http://example.com/ad-banner.gif",
      "example.com", "example.com", false , "image");
  engine2.addTag("abc");
  Check(true, false, false, "", "Basic match after deserialization for a buffer with tags and a tag match", engine2, "http://example.com/ad-banner.gif",
      "example.com", "example.com", false , "image");

  // Deserialize after adding tag still works
  Engine engine3("");
  engine3.addTag("abc");
  engine3.deserialize(reinterpret_cast<const char*>(ad_banner_with_tag_abc_dat_buffer),
      sizeof(ad_banner_with_tag_abc_dat_buffer)/sizeof(ad_banner_with_tag_abc_dat_buffer[0]));
  Check(true, false, false, "", "Basic match after deserialization with resources with a tag on the engine before", engine3, "http://example.com/ad-banner.gif",
      "example.com", "example.com", false , "image");

  Engine engine4("");
  engine4.deserialize(reinterpret_cast<const char*>(ad_banner_with_resources_abc_dat_buffer),
      sizeof(ad_banner_with_resources_abc_dat_buffer)/sizeof(ad_banner_with_resources_abc_dat_buffer[0]));
  Check(true, false, false, "data:text/plain;base64,", "Basic match after deserialization with resources", engine4, "http://example.com/ad-banner.gif",
      "example.com", "example.com", false , "image");
}

void TestTags() {
  Engine engine("-advertisement-icon.$tag=abc\n"
                "-advertisement-management$tag=abc\n"
                "-advertisement.$tag=abc\n"
                "-advertisement/script.$tag=abc\n");
  Check(false, false, false, "", "Without needed tags", engine,
      "http://example.com/-advertisement-icon.", "example.com", "example.com",
      false, "image");
  engine.addTag("abc");
  Assert(engine.tagExists("abc"), "abc tag should exist");
  Assert(!engine.tagExists("abcd"), "abcd should not exist");
  Check(true, false, false, "", "With needed tags",
      engine, "http://example.com/-advertisement-icon.", "example.com",
      "example.com", false, "image");
  // Adding a second tag doesn't clear the first.
  engine.addTag("hello");
  Check(true, false, false, "", "With extra unneeded tags",
      engine, "http://example.com/-advertisement-icon.", "example.com",
      "example.com", false, "image");
  engine.removeTag("abc");
  Check(false, false, false, "", "With removed tags",
      engine, "http://example.com/-advertisement-icon.", "example.com",
      "example.com", false, "image");
}

void TestRedirects() {
  Engine engine("-advertisement-$redirect=1x1-transparent.gif\n");
  engine.addResources("[{\"name\": \"1x1-transparent.gif\","
                      "\"aliases\": [],"
                      "\"kind\": {\"mime\": \"image/gif\"},"
                      "\"content\":\"R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==\"}]");
  Check(true, false, false, "data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==", "Testing redirects match", engine,
      "http://example.com/-advertisement-icon.", "example.com", "example.com",
      false, "image");
}

void TestRedirect() {
  Engine engine("-advertisement-$redirect=test\n");
  engine.addResource("test", "application/javascript", "YWxlcnQoMSk=");
  Check(true, false, false, "data:application/javascript;base64,YWxlcnQoMSk=", "Testing single redirect match", engine,
      "http://example.com/-advertisement-icon.", "example.com", "example.com",
      false, "image");
}

void TestExplicitCancel() {
  Engine engine("-advertisement-icon$explicitcancel\n"
                "@@-advertisement-icon-good\n");
  Check(true, true, false, "", "Without needed tags", engine,
      "http://example.com/-advertisement-icon", "example.com", "example.com",
      false, "image");
  Check(false, false, true, "", "Without needed tags", engine,
      "http://example.com/-advertisement-icon-good", "example.com", "example.com",
      false, "image");
}

void TestThirdParty() {
  Engine engine("-advertisement-icon$third-party");
  Check(true, false, false, "", "Without needed tags", engine,
      "http://example.com/-advertisement-icon", "example.com", "brianbondy.com",
      true, "image");
  Check(false, false, false, "", "Without needed tags", engine,
      "http://example.com/-advertisement-icon", "example.com", "example.com",
      false, "image");
}

void TestDefaultLists() {
  std::vector<FilterList>& default_lists = FilterList::GetDefaultLists();
  assert(default_lists.size() == 8);
  FilterList& l = default_lists[0];
  assert(l.uuid == "67F880F5-7602-4042-8A3D-01481FD7437A");
  assert(l.url == "https://easylist.to/easylist/easylist.txt");
  assert(l.title == "EasyList");
  assert(l.url == "https://easylist.to/easylist/easylist.txt");
  assert(l.langs.size() == 0);
  assert(l.support_url == "https://easylist.to/");
  assert(l.component_id.empty());
  assert(l.base64_public_key.empty());
  num_passed++;

  // Includes Brave Disconnect list
  FilterList& l2 = default_lists[7];
  assert(l2.uuid == "9FA0665A-8FC0-4590-A80A-3FF6117A1258");
  assert(l2.url == "https://raw.githubusercontent.com"
      "/brave/adblock-lists/master/brave-disconnect.txt");
  num_passed++;
}

void TestRegionalLists() {
  std::vector<FilterList>& regional_lists = FilterList::GetRegionalLists();
  assert(regional_lists.size() >= 40);
  std::vector<FilterList>::iterator it =
    std::find_if(regional_lists.begin(), regional_lists.end(),
      [](FilterList& list) {
        return list.uuid == "80470EEC-970F-4F2C-BF6B-4810520C72E6";
      });
  assert(it != regional_lists.end());
  assert(it->langs.size() == 3);
  assert(it->langs[0] == "ru");
  assert(it->langs[1] == "uk");
  assert(it->langs[2] == "be");
  num_passed++;
}

int main() {
  TestBasics();
  TestAddingFilters();
  TestDeserialization();
  TestTags();
  TestRedirects();
  TestRedirect();
  TestExplicitCancel();
  TestThirdParty();
  TestDefaultLists();
  TestRegionalLists();
  cout << num_passed << " passed, " <<
      num_failed << " failed" << endl;
  cout << "Success!";
  return 0;
}
