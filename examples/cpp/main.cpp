#include <iostream>
#include <assert.h>
#include "wrapper.hpp"

using namespace std;
using namespace adblock;

size_t num_passed = 0;
size_t num_failed = 0;

void Check(bool expected_result, const std::string& test_description,
    Engine& engine, const std::string& url, const std::string& host,
    const std::string& tab_host, bool third_party,
    const std::string& resource_type) {
  bool match = engine.Matches(url, host, tab_host, third_party, resource_type);
  cout << test_description << "... ";
  if (expected_result != match) {
    cout << "Failed!" << endl;
    cout << "Unexpected result: " << url << " in " << tab_host << endl;
    num_failed++;
  } else {
    cout << "Passed!" << endl;
    num_passed++;
  }
  assert(expected_result == match);
}

void TestBasics() {
  Engine engine("-advertisement-icon.\n"
                "-advertisement-management\n"
                "-advertisement.\n"
                "-advertisement/script.\n");
  Check(true, "Basic match", engine, "http://example.com/-advertisement-icon.",
      "example.com", "example.com", true, "image");
  Check(false, "Basic not match", engine, "https://brianbondy.com",
      "brianbondy.com", "example.com", false, "image");
}

void TestTags() {
  Engine engine("-advertisement-icon.$tag=abc\n"
                "-advertisement-management$tag=abc\n"
                "-advertisement.$tag=abc\n"
                "-advertisement/script.$tag=abc\n");
  Check(false, "Without needed tags", engine,
      "http://example.com/-advertisement-icon.", "example.com", "example.com",
      true, "image");
  engine.AddTag("abc");
  Check(true, "With needed tags",
      engine, "http://example.com/-advertisement-icon.", "example.com",
      "example.com", true, "image");
  engine.RemoveTag("abc");
  Check(false, "With removed tags",
      engine, "http://example.com/-advertisement-icon.", "example.com",
      "example.com", true, "image");
}

int main() {
  TestBasics();
  TestTags();
  cout << num_passed << " passed, " <<
      num_failed << " failed" << endl;
  cout << "Success!";
  return 0;
}
