/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/test/refill_confirmation_tokens_test_util.h"

namespace brave_ads::test {

std::string BuildRequestSignedTokensUrlResponseBody() {
  return R"(
      {
        "nonce": "2f0e2891-e7a5-4262-835b-550b13e58e5c"
      })";
}

std::string BuildGetSignedTokensUrlResponseBody() {
  return R"(
      {
        "batchProof": "flsRyY/zcE6V1ymow+hJW9DdVjGlEAGyVr3QN7kc1Qf6eeYOSK93thF+IZehEXrsRFjFoV3reEm60a/pEq7XAA==",
        "signedTokens": [
          "2g0WjgYZfADeoAYI0kkXNVCcXCpfg5lv5yRdCHigkGs=",
          "XuBEdiNS8cOMKApsIiHygcnVOVssvaoudyd1wMI+O1Y=",
          "sr9ISUbtRozfF8IXslvveZSCE/NIZzkHr4WkcizZims=",
          "jDpLjMtRm6OjPwMmaH3IUP9zuYHvn1PkUamYrtuaCRs=",
          "5u947b1ODsUaTT1tU1Vh1qN24HSAaaCt0MapVNIiM2Y=",
          "XD25BcXUS4YJEGQdedCrli6wnufmFdKQmbAkKCJMnBo=",
          "QKcG8fsNPwWzmlZUZbfww0A+iBS8FM+a76pz/R4b2HQ=",
          "zFD7PmAnw/CP5Tq1qxw8iSbrjBH036al2dvb1fRZz0o=",
          "qMm/EqdSREFbFD4i6ezy+evFaGYp7Untq0iULvm4mwU=",
          "sjJQ9T5rFJcw1+ZlyeOZHY7Kmmlv9Po3FaBcZ1hL1hI=",
          "cOTDHJEx2ZLBZKk8W56KJq3fgsSj5o0mRaVCYQZXkxo=",
          "0CIrR2Ng3oOlJyOKtMwGSnfTXJwXjh5E/VBwkBLAbhg=",
          "IrcpnN7p4xwGbGGpqCMRh1G1c2ujilXuYfuYKIbxahc=",
          "HhtASybmIMS+HwgKsbvzUeM5TnLsYReixf8WnTCIvx0=",
          "9FpVkri+eN+D7sXzVjwObjRSx6ADewQ66SEuFCRlRx8=",
          "aJ7DnFP13RXo7GItmAvKPH5CvY2dITtc149PAr5WtBw=",
          "8DcOKuGIlMx00d+g4ixlz4VMxjKRQ2CJEyyf/fwEDTk=",
          "BPGAp/+/RFBTwm2Lx7+MxyYgkvzRdJqkWBw4TSRhPlE=",
          "RirDth6yyg9Zq0MCoiRiibFvaMVaFyyiQC/SDJZemiI=",
          "pImA5vt8XlSd7xWc0DcheI1pnIBRIiMKltplBn8vI0o=",
          "tCEMf+D18pRR6rWzWdHDyQlAjZ6/465GnUg6pXlcYkM=",
          "nJRbR/uZQTIKLQF6Yhxx/bywQ6SvjP7YJJz5a1k69xc=",
          "cLKRj8+1KR0lk2w1uUysR5Go87WgLfiJ+9NXD91AHl4=",
          "WPvZHt4qITOFMitv/NTXOx6M+GUf3H5ud+uuCLacrgs=",
          "XHCtobEsiESgGDgqCn4I6FmrU9vnEhXRNrzw5O0bdl0=",
          "yIDV8B7/fv1t8RcNLm93N0siuMX5AXAll5QHaOyAxj0=",
          "7pBWOhSj1cl2DlsCHC3/Dapd6bknZ+urewK6mtOKFlg=",
          "KhNVrYoxTyth9xDMSXLAyO1MyObuYQh1NyFeMshBUA4=",
          "xGqTbNWAJREpd2lKO3/pqqBmazx28VEtSwpdHJpqZzE=",
          "hkLhf4xPTDClmUoYrQZCXnP5jILU097Z5mULuez9/ns=",
          "IgJBYMaNLixag1yoXg6lNLuMjNoNE03m3456d3ekKE8=",
          "Rj1BEQ5Zb8OJAOhIFUkHatzcZPkiD71BQdLU7VQV2HA=",
          "yOKGTPFF3f64H1IVpUCs/I9uw/ZYQGlOsMnmLiieplg=",
          "4D9T1jXnerXvKC3hfERLC462Bg2fXDon+SPgyB59oBw=",
          "xLeSjxjpLs2g5nppceuPir5LaLdvU/ZwrQ6j9tgEklc=",
          "qGHBPL0PEyK3S190hPOJnMU8ptUmnfY8T81oEtoH4F0=",
          "XpO85gRmr7HKoAmLPqx+Ne2KWS9ewqclj9l865cnokc=",
          "yotMG+lPwhKxJrSDn0XXVF7+50m/WyjJmUszCEs1BhE=",
          "BNz99P06znT/ABsf0M3ga6AAPPIGoxWGhURRIMPqdmg=",
          "QP+g04U4LkLADAomyQui7+31al/UwEhsq9goa26ndm4=",
          "tmV4MEvcoaoY1LJvehD/57duhcbK3Y9+ULfuZGzHLWw=",
          "kjeRJnyBLsdetp8pR5uDxn/to4i4Y8XwXqTWmbEhsWo=",
          "IjlKYdnbqNacAzV8BUR+REHmj8V2EOcurU9afpATF1g=",
          "xJcKsn/r/mBJHOrHwKAlvdYIMZKvZcB4N7OtkCd4q3w=",
          "uDbbJC4fqBitdgvGbn2M1ksMXG7XxO4XEpzWqR9VIhw=",
          "MCmJPVJMzXTVl+7bR57iH+CRUWMrUUjcBJ6P8LdD0wc=",
          "4vfcsB7coDvhHP+HR71AJ5jYTAhuiWOdQN207dVEqAc=",
          "ZvXt0CyDwdHXN2p3t9xrXkb9Y6yjSRBC5L5EwIWlwGs=",
          "imsSQ9dFxDIoWexM877Bx19Elo4qFA2Vgds761PbMEQ=",
          "XsxhvO3k44prGYyJydyi2VSXqrWEX9AHREy8YfMyhEA="
        ],
        "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM="
      })";
}

}  // namespace brave_ads::test
