#include <iostream>
#include <assert.h>
#include "wrapper.hpp"

using namespace std;
using namespace adblock;

size_t num_passed = 0;
size_t num_failed = 0;

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
