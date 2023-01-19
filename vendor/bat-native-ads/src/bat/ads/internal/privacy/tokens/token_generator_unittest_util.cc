/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/token_generator_unittest_util.h"

#include <string>

#include "base/check.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"

namespace ads::privacy {

std::vector<cbr::Token> GetTokens(const int count) {
  const std::vector<std::string> tokens_base64 = {
      R"(nDM8XFo2GzY/ekTtHm3MYTK9Rs80rot3eS1n+WAuzmRvf64rHFMAcMUydrqKi2pUhgjthd8SM9BW3ituHudFNC5fS1c1Z+pe1oW2P5UxNOb8KurYGGQj/OHsG8jWhGMD)",
      R"(hOM1nqEKe6Yq3Ol/NtDMuovmhJ/OxUWcxcv4CHZKbNEutsqdKEJH5mnEZtDFeDe818xcVl8+iAA3PU5jkLY0ZgYiuNcUTptppdiUxy9ePPd7/R+XQyIvp/+sx0GM5q8C)",
      R"(dUneny/YSoEQ49Yq87RAFrm2W9aSe8ZvivfPd+cPpwPMrpou4SXlQo7iIoSxc2xkP97l8AHv203/8xa8iYcNauvy2dkyqHQYMkqf75XoXRKOTz9J0fU4mwA8VNQK/acC)",
      R"(91BYFcYeT5GxE69ZuQEFhW+GWc7kpubB23x1QkIr752y/nOkrlaPC23dLUHjHelkdkur8kl6tvB7lWkUodCuJZ+PEvLdQBUn0HB0Fhw7F3GjM7zcYm+X/Es0XxF0eqMO)",
      R"(fdYFlnxPmMlTxhx+UrC5RHDP1O2XZ25U5FMyTtOOZEbbOJxs1UZ3KyKO66p3Wey/kwdyothFSnj9emOdhREkhlakRGUhq6v1H/VW3Mss4ii9WGwCgHuTUsUveYLHhuIN)",
      R"(srAYtniu9oLiy6vGnT+9vxkyZXg2bBZ+AL34nJjzuMb3Fhni0eOls1HFdDgKYyyOXht/PYqh9rJVs0uxi1IHrPmPXgUF1lwFbT1RGbe7aPjlRuIiqY57PCZ2a5v1YCUP)",
      R"(5378G/kJEeSYiVgD1xggw+qVQMwE6OU6d4UYL9awIGYn18CGjbEoUK2/tME0H2euQ+ow0nTB6LRXVZTnG1i8gFdmLlNOdAgTa9QZaqGjaNWldc4qDuWnYUMjaERdxAsL)",
      R"(ygTH+4qCkFFC9QxnzCaltVWh7NUFgzX9AKtuA64VRs10PFEGg8om1RA8iOsoGP3NbJx7OsLs68Pt4vbhVFl3NP1FIhTOErUsr/9kVK3kSaxOq7zIzIIWtoQeBLHfbNcO)",
      R"(HMNghK4aJ35KE4dnKguB/ah4S8fnB2+/vyYOfVKAkpFwqaB+Cgn6U0Dx8Oji+3My9DRmDfeW0EZKuarbq+uZ0oHhXfNusDGsuXa9Ya1yip2bZhNtb6FYu9W0h4YP9n0K)",
      R"(GNOSvdBWA374/0pL8tO//zbTmgJHLxfF9wi3p1yH2ASF8Jblpo18+Nykw9wdYUd+UXDMV6jdKhJOKwPQK2yGqs6nUEenhYOWFhXOj2041Br1NDrZHGXeJSTJJFnXpyYN)",
      R"(W8l4ZszB8Z7GuPpoPYqoIOy670gNkYyzbgxo92Jsnn7yQxThJpD0u2Ku9Hp4oi/YprcnJt+2Dlt1Wsa0xYrJE63feK7Xr/OGXuDiAVgW5yt/lG/dqgb36LgNXyFkFpQA)",
      R"(/DPmSPDA4N3kVYncom7UDyI1aaZIP1rvEcEeum7/ID2pYab+B5U8V8c3Te0EVh0bDAeMFLGuzDOu2oqPOj11JrDuSt5oM/bx3XbZiStD/Jq7COgkQDilj7D0tj4gcFMK)",
      R"(aBgkTcTItwXbg3z9ZN3IXnUZe4Db9Y7bGO1nSMPZ/4wGHzKyNHP9d0C3IrO7ktysfTQ09M4uaY8Nl7esKcGiOer4opSu8qmQ8e/40dTWmWpNFeQgzYTBC4xD9V5Fsl8I)",
      R"(TCttRGdojV+oauGV9X8np4t9u8v1M0Y4w14MwOXrjdJfG73W8G8brdEQzhdHKJe5R+Yq54YQj+emNOZtlfSJmM7CtcGZJD8fl6vxC0GVjWY/iJWwiZyaF+bG07FiYFsE)",
      R"(DyUkPuaJTS74413U0Yu4ot8fswFlLxyYAHzLELAHY3WLWBGPmvUn3RV54DzK/ETVVG6lqFkiyHtVkeq+2pS399r1YJLVXT+eXk86vMbTTHjBxzf5gAZGlfKfBwaF43YA)",
      R"(cjByn71V4ayfoPUB/5zTIgynHHUHHT6q2mU3iCPU1AIkI1EdesV3/BCP3f+o9x3oUu5Z4upI5UVf2Fu5vw0+v/mzcgX1aUqEVvod0sr96Rth2Ey1pEuFAVyAqdS3Sa4C)",
      R"(lo0q3haERhRL5SplIQNG9Birn2XJ/8gTgQPGvd380lnfGInTgjODJaDPgB3IU6Uv/HO+SDl1J1akg3w36B7jiYGYRdv4adKhkQgPtisn01QmY/T6o26zUTz20EDS9zMN)",
      R"(AtPpTkiDbDJZWlrLzWRfAz8AJKGI7iyvyUiLHk+lFFh6semn4s5EVmUbI7UelcXSnu7g8H6iyRnSnbak10BGf5GA2BD5ZE3g3+JFnDOaSjijXztl04V4eRhNUKjT330E)",
      R"(oPYNrhlGNqfe3lXD3c2dGy+F6ko84fdjUrOorGDOlicrUYk1oY0uy+IDp42T9BnP3yRY2oqslK9q7CHwMrqwIYJ8GRlXUzxbgnxPVLHfoPRh6IgoJcfSFOGBhAse6CMG)",
      R"(wmOJTMZXl/Jxa1CJ6fSLLoZ1KcoWhAF6JX8osm0pJBWg9OatWcAC3mEPyTJEjyivsPI6IuiiNilYFbNlqFQfLqmWj5CvjsPLl9qF62dKi73O+0ppMc66/jJiu/tB4ooE)",
      R"(3ZdGwflDxCAWzrUdhsF3/mSHB9hqyfo5JCxe0bGBZY7q+PXI4OXy74XwiP9a57RC4xpygNbYiBOuOwM6QH0xF7Rkc3nogEEB9zk/FFlGTa58nmqCjXesKKikRrr+C8IJ)",
      R"(sObRN0+i0h7DJS9z/6owBepl9NH8KLgQVBeINzN7cxARXls28EWDMKvPaaX6tHvTnlwc5MidMfMf859OPEhZO4Xt4Koej3iDlgboyRgjNFEa4ikaUqswie6ui+wdaQYC)",
      R"(xF3Q6vzzci9l+L46bb/kxNjoNm+F5EDzj0+uzWX901v6FRxJk2hf5WqB/8axiUQzcZk8VtNuL12xxI7HrIERH2LA7i3K2qStYsZNNo7MQaNXsi/81gZyWVQj3zfSaY8A)",
      R"(Hpx//lczPWD76jmCvrcuu4kYfVuAEZxH+BZGrQtLr0Vb083NW5WXRVxFnp/URBGb5tMJGIol9HFePlS6e8Y3dEAVJpaNeoLCgdtxSk/kDP3u8LbJSCp/p+GhtLp0SmME)",
      R"(QuDdJzaPRYbFt2k1s/vYXeRBQpOfTniAj6a4xbzeQA1q7gJ/kPs8xBXUFqkTlqnA2HF0WY6rtzq4kGI7X8x8HeW4PBm1QY1giZcN+WqWu6fzz6iRqSP7Zeq9HigKr2sL)",
      R"(gsPziQgnVrIS+kSvTbUhSyU+cgeqeRD+A95Qjy2ZVENGwsrxKudQo+EftSfwNIiwxBayeE1GVkHalqt6nGbR7aKGki/3mWRUXSgZGTcV8njfvbOtXR4vve1tJijCDbsO)",
      R"(HyIL0wrnF82Hlu4O+d2lcw7JrK08tEySgmMWqJYKWiHEj7et65iXF/RnB0s5fI4F8XWZowOMsEx4GXYDoA7g2FCkEeRKI5udeufajVkXsb7SP5UEasZWviTHQHd8YKAK)",
      R"(KaO+oGCa0K9grIC/bScIJbByfk4T5Th6Hhjdsj1qZ1G8ddjh5vjcgOGqDwnOCR3aMGt2mrw8EwBh2LsV8BPpolZP1AIUB1A9L5pSoPaaC/kqsJDzFmInCPMTnqI0C10G)",
      R"(fBSYv5tYh5VQVuS79vkZKbu956W+sUeOzYdLwGWx+PmKVVC1EfFpfm5G8eOfwFZg8oXq9m3yvBELqbBXnNXTvwjrbvO34YXcs2a1ZXHCsclTJVrQuFP1NsX+/TSlhdgH)",
      R"(gvcXM8vhu0i1Ro3LF9YeIwvyx/9skD4NMC/uyn3op5505WhAQ+r/Moahe+Mn5G4+sqZnN0ZcNirLtn20w1pL7HnH8vDklsHVTSun69Xut7Nz3qQ+x5EO8e5KH+WpynAC)",
      R"(PwIGVD9KMEgn36L8hc6Y6PAbhxRtxqFa1VN2FcYN1/einCyCWyMAMJpdpprwjeAur3CmJaBYL3YRTgFeZow9WqSU5G/Kuw9AHnZUCsjRa5IyKab4EoEphCJ94Cnp+q8A)"};

  const int modulo = tokens_base64.size();

  std::vector<cbr::Token> tokens;

  for (int i = 0; i < count; i++) {
    const std::string& token_base64 = tokens_base64.at(i % modulo);
    const cbr::Token token = cbr::Token(token_base64);
    DCHECK(token.has_value());

    tokens.push_back(token);
  }

  return tokens;
}

}  // namespace ads::privacy
