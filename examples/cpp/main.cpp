#include <iostream>
#include <assert.h>
#include "wrapper.hpp"

using namespace std;
using namespace adblock;

size_t num_passed = 0;
size_t num_failed = 0;

const unsigned char ad_banner_dat_buffer[] = {
  0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xff, 0x8d, 0x8c, 0xb1, 0x0d, 0x00, 0x11,
  0x18, 0x85, 0xc9, 0x55, 0x57, 0xdc, 0x0c, 0xd7,
  0x5c, 0x77, 0xd1, 0x5b, 0xc0, 0x20, 0xbf, 0xd0,
  0x2a, 0x58, 0xc2, 0x26, 0x12, 0x95, 0x9a, 0x0d,
  0x6c, 0xa0, 0xb5, 0x08, 0x89, 0x50, 0xe8, 0xbc,
  0xe6, 0xe5, 0xcb, 0xcb, 0xfb, 0x10, 0x3a, 0x0b,
  0x9e, 0x1d, 0x80, 0xfe, 0xe5, 0x73, 0x79, 0x71,
  0x7b, 0xaf, 0xb1, 0xdd, 0x93, 0x41, 0x10, 0x0e,
  0x4a, 0x49, 0xbd, 0x8e, 0xd5, 0x27, 0xf3, 0x30,
  0x1b, 0x77, 0x19, 0xee, 0x40, 0x9a, 0xd0, 0x4c,
  0x78, 0x00, 0x00, 0x00
};


const unsigned char ad_banner_with_tag_abc_dat_buffer[] = {
  0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xff, 0x95, 0x8b, 0xbd, 0x0d, 0x00, 0x10,
  0x10, 0x85, 0x4f, 0x54, 0xa6, 0x30, 0x81, 0xd2,
  0x24, 0x26, 0x70, 0x68, 0x2f, 0x61, 0x00, 0xa5,
  0xd6, 0x08, 0x96, 0xb2, 0x0f, 0x09, 0x67, 0x00,
  0xaf, 0x79, 0x3f, 0x79, 0x1f, 0xc0, 0x9f, 0x04,
  0xfb, 0xd2, 0xf2, 0x64, 0xc5, 0xdd, 0x47, 0x83,
  0x9e, 0x28, 0x15, 0xbe, 0xc9, 0xb7, 0x63, 0x80,
  0xd6, 0x47, 0x76, 0x76, 0xd6, 0xcb, 0x8b, 0x0d,
  0xc7, 0xc1, 0xc2, 0x31, 0x73, 0x00, 0x00, 0x00
};

void Check(bool expected_result,
    bool expected_cancel, bool expected_saved_from_exception,
    const std::string& test_description,
    Engine& engine, const std::string& url, const std::string& host,
    const std::string& tab_host, bool third_party,
    const std::string& resource_type) {
  bool cancel;
  bool saved_from_exception;
  bool match = engine.matches(url, host, tab_host, third_party,
      resource_type, &cancel, &saved_from_exception);
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
  assert(expected_result == match &&
      cancel == expected_cancel &&
      saved_from_exception == expected_saved_from_exception);
}

void TestBasics() {
  Engine engine("-advertisement-icon.\n"
                "-advertisement-management\n"
                "-advertisement.\n"
                "-advertisement/script.\n"
                "@@good-advertisement\n"
                );
  Check(true, false, false, "Basic match", engine, "http://example.com/-advertisement-icon.",
      "example.com", "example.com", false , "image");
  Check(false, false, false, "Basic not match", engine, "https://brianbondy.com",
      "brianbondy.com", "example.com", true, "image");
  Check(false, false, true, "Basic saved from exception", engine, "http://example.com/good-advertisement-icon.",
      "example.com", "example.com", false, "image");
}

void TestDeserialization() {
  Engine engine("");
  engine.deserialize(reinterpret_cast<const char*>(ad_banner_dat_buffer),
      sizeof(ad_banner_dat_buffer)/sizeof(ad_banner_dat_buffer[0]));
  Check(true, false, false, "Basic match after deserialization", engine, "http://example.com/ad-banner.gif",
      "example.com", "example.com", false , "image");

  Engine engine2("");
  engine2.deserialize(reinterpret_cast<const char*>(ad_banner_with_tag_abc_dat_buffer),
      sizeof(ad_banner_with_tag_abc_dat_buffer)/sizeof(ad_banner_with_tag_abc_dat_buffer[0]));
  Check(false, false, false, "Basic match after deserialization", engine2, "http://example.com/ad-banner.gif",
      "example.com", "example.com", false , "image");
  engine2.addTag("abc");
  Check(true, false, false, "Basic match after deserialization", engine2, "http://example.com/ad-banner.gif",
      "example.com", "example.com", false , "image");

  // Deserialize after adding tag still works
  Engine engine3("");
  engine3.addTag("abc");
  engine3.deserialize(reinterpret_cast<const char*>(ad_banner_with_tag_abc_dat_buffer),
      sizeof(ad_banner_with_tag_abc_dat_buffer)/sizeof(ad_banner_with_tag_abc_dat_buffer[0]));
  Check(true, false, false, "Basic match after deserialization", engine3, "http://example.com/ad-banner.gif",
      "example.com", "example.com", false , "image");
}

void TestTags() {
  Engine engine("-advertisement-icon.$tag=abc\n"
                "-advertisement-management$tag=abc\n"
                "-advertisement.$tag=abc\n"
                "-advertisement/script.$tag=abc\n");
  Check(false, false, false, "Without needed tags", engine,
      "http://example.com/-advertisement-icon.", "example.com", "example.com",
      false, "image");
  engine.addTag("abc");
  Check(true, false, false, "With needed tags",
      engine, "http://example.com/-advertisement-icon.", "example.com",
      "example.com", false, "image");
  // Adding a second tag doesn't clear the first.
  engine.addTag("hello");
  Check(true, false, false, "With extra unneeded tags",
      engine, "http://example.com/-advertisement-icon.", "example.com",
      "example.com", false, "image");
  engine.removeTag("abc");
  Check(false, false, false, "With removed tags",
      engine, "http://example.com/-advertisement-icon.", "example.com",
      "example.com", false, "image");
}

void TestExplicitCancel() {
  Engine engine("-advertisement-icon$explicitcancel\n"
                "@@-advertisement-icon-good\n");
  Check(true, true, false, "Without needed tags", engine,
      "http://example.com/-advertisement-icon", "example.com", "example.com",
      false, "image");
  Check(false, false, true, "Without needed tags", engine,
      "http://example.com/-advertisement-icon-good", "example.com", "example.com",
      false, "image");
}

void TestThirdParty() {
  Engine engine("-advertisement-icon$third-party");
  Check(true, false, false, "Without needed tags", engine,
      "http://example.com/-advertisement-icon", "example.com", "brianbondy.com",
      true, "image");
  Check(false, false, false, "Without needed tags", engine,
      "http://example.com/-advertisement-icon", "example.com", "example.com",
      false, "image");
}

void TestDefaultLists() {
  std::vector<FilterList>& default_lists = FilterList::GetDefaultLists();
  assert(default_lists.size() == 6);
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
  TestDeserialization();
  TestTags();
  TestExplicitCancel();
  TestThirdParty();
  TestDefaultLists();
  TestRegionalLists();
  cout << num_passed << " passed, " <<
      num_failed << " failed" << endl;
  cout << "Success!";
  return 0;
}
