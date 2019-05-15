#include <iostream>
#include "wrapper.hpp"

using namespace std;
using namespace adblock;

void Check(Engine& engine, const std::string& url, const std::string& host,
    const std::string& tab_host, bool third_party,
    const std::string& resource_type) {
  bool match = engine.Matches(url, host, tab_host, third_party, resource_type);
  if (match) {
    cout << "Matched: ";
  } else {
    cout << "NOT matched: ";
  }
  cout << url << " in " << tab_host << endl;
}

int main() {

  Engine engine("-advertisement-icon.\n"
                "-advertisement-management\n"
                "-advertisement.\n"
                "-advertisement/script.\n");
  Check(engine, "http://example.com/-advertisement-icon.", "example.com", "example.com", true, "image");
  Check(engine, "https://brianbondy.com", "brianbondy.com", "example.com", false, "image");
  return 0;
}
