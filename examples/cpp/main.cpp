#include <iostream>
#include "wrapper.hpp"

using namespace std;
using namespace adblock;

void Check(Blocker& blocker, const std::string& url,
    const std::string& tab_url, const std::string& resource_type) {
  bool match = blocker.Matches(url, tab_url, resource_type);
  if (match) {
    cout << "Matched: ";
  } else {
    cout << "NOT matched: ";
  }
  cout << url << " in " << tab_url << endl;
}

int main() {

  Blocker blocker("-advertisement-icon.\n"
                  "-advertisement-management\n"
                  "-advertisement.\n"
                  "-advertisement/script.\n");
  Check(blocker, "http://example.com/-advertisement-icon.", "http://example.com/helloworld", "image");
  Check(blocker, "https://brianbondy.com", "http://example.com/helloworld", "image");
  return 0;
}
