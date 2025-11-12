/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage_test_util.h"

#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage.h"

namespace brave_ads::cbr::test {

TokenPreimage GetTokenPreimage() {
  return TokenPreimage(kTokenPreimageBase64);
}

TokenPreimage GetInvalidTokenPreimage() {
  return TokenPreimage(kInvalidBase64);
}

const std::vector<std::string>& GetTokenPreimages() {
  static base::NoDestructor<std::vector<std::string>> token_preimages({
      R"(/mfTAAjHrWmAlLiEktbqNS/dxoMVdnz1esoVplQUs7yG/apAq2K6OeST6lBTKFJmOq7rV8QbY/DF2HFRMcz/JQ==)",
      R"(8o+uKml91AthZzZgdH0iVGtZHfK2lAmPFdlhfF8rXnF67OtwnzHntFOimAmE73L7d9zh2sl0TJMv0mYwFf3zfg==)",
      R"(4F+VBba3Wv9CZx5HF/R217w4M4B7v4xAWTpJZ3bCIog03tm1eHrsRgCn1XGFxrugV7U/C5+waLadwEk9vqB23w==)",
      R"(Qu5OVNnHXfXB+aj6dJqV2B68pEfYwdevhMMBvlC/w4o81XE13y1b8dGYdDmN30ct4LacOJ8d/s8CVt2HZ9+P9g==)",
      R"(bTChDKb8bO4406rN6Kazn8rMjaFE+8VHsGjhRJQOhhuiE7iPkD1x8Wh16Iw2KVgB3F/gzp48TSLloErwrpGeTw==)",
      R"(VJpMA/p6XOysRoKyogmT3c8GzCmuOvPOfPFEQECU6RE4mmrG0iDSM7VbMYTJQNCUU2Gwh021TafmcS6SUO5lew==)",
      R"(yxV23PfA329B7GwzAKDoooS8qBUby82ldcQibOd0xAwbcqpp2fURn9JaDApnyO6NT6eFMfXXzvghOJJAWBSTJQ==)",
      R"(h8avY/txLkex4vX8cROpzCFqY2ehPZIw4avYWpkUUs6FuY/X3WNQBAv6k8BNYo2BMyw9WS75hO6NHd6ZHH/vCw==)",
      R"(+NDdXqzN+fBKzZsahecJ8BjKygzxxVklMeqZaQTBDIL8ymSL92xC0ArV/l5cJstOam3v3lm9dxkKaS/kq2xtNA==)",
      R"(iP/zkjmYnlxQQAPYshasqwM9aY6dcSnFWrgZ8PGwMv8cSoN0c1ZWBqB02iMKJ8tU9Z/cu4aOib6H+nCER/qCMA==)",
      R"(QUWt9T/TFI9H+6f3IHyZE1MDqZVte4sP7Bm9RItz8EL4MLabEVOJPFwJ+MMMqXKa1yAbZLk7CAPALRTAoQWhtQ==)",
      R"(IfiUpAFFGEogSCEJWn++VNMtE5D2FPAgUaZq7bT4zn+POnCHSTRTdCYfcAWT2O47bFXcW+hWRO26nqZG0nbGMw==)",
      R"(8PvoTrzyIRxOQN3pF+i6pBUsICiafyrtFzK2y3NKwPpEfhwySbWi0OmI4/EfRjH/8B1pYhC2e5ocfXxMQJg/8w==)",
      R"(B3ba4F/Z+7ear6hgVXbb+HqtY29zt9/08wqIiRAruCmz26JYrKK36FsHVoaWHVXbqE50kKa6EmBuwxk8K/p0DQ==)",
      R"(iBYbz22bz4DkUYFPvjxV/qQVvpwyBSgSo+28e1Fat0CqFBHzFtzvQqg/U2YBbuGoBG28pkyPOd90jyu6X/gDvg==)",
      R"(RddwoyJZ+MNSrSAbRMd6pdRWZCZiwtTecfBT6CwEAsrPPckdHnEohw52y8Zfd8E3CJL6PglGe45rStDxuIGGTQ==)",
      R"(nKwtChzFdVT/u9NwjsnWJ6S5wsd2zHUBumOQgjKNqAMhdKU3XhhEbOq+8PuNEPTD59IgPjsli7/jTghoNkE/zA==)",
      R"(tOyts/yWbWoIZ80CALNDz5iYbuyZ0kmtFG3maOIdK/MiTY7j2kEdaQa7zA/TGw/0nq+mOb26o39j6CVoQCKf3w==)",
      R"(AX0TXD4Qp8J+dORSHwvBU6Ns0AxeaNwZHxkurlIsrR7L5Pzuci2kvSDi9qomjUsA7byhWtIcOwPTL7NUPzaTLA==)",
      R"(fM+qLel3ljJMgZ3CBcDoo2KjZC/nMbnQReo7f1uNfnoQkW+5nHYMm7b4IheJ9llg1vU8I8pOllcXZTWlI7dw9A==)",
      R"(vh3GD+slh2KDvKG0RWQG9BmXwvUX7ZVrpHPXTPLGqhooc+3/Dfh0BnHVEA1aOt0H+jqfpK+gzeJwZPB+uE6h4A==)",
      R"(MLoNQTqT5VnEp0f30JB5014MguJ0nxq+r9p3KGJL+/ATOEwdMQh2Hwah+3nfmTdmS2nBxA0msxFn9hRZ6tAZUw==)",
      R"(k5ruhL1W9dtuUdnnNj/FDE0cHeLUR0+KLMWoNQ5kTbSjJkt7cw1hcEkkLVb+v9k8+dGugEktHiodhGOOngbdMw==)",
      R"(17i178+xnyy1hgK8Noy24I+YFtqg+VgDbN4KFyPYJiNXkI7pyA05+PGewcFByz+OJM9NjQK0D24hJVm8lg6kLw==)",
      R"(gIy44kVuFM7OIdd1iOXhez/wfIwZtF+M7Hmx38CQdivUT6545tIk5Il14czyXk/yXy/Parn0Sn+SD8Oc/ZltTQ==)",
      R"(vST4PalUIaZRZJWFyq6oecP3EG6Q7LgMVYejXHb/QfGARMuB3+qiRNPDq5eX8A6CzzWQkgi5xKmPlnjOQiKYWw==)",
      R"(MP6Z2d0QR4hQRWnVm0gRKvhmi8N8C3wT5iOHPDjTeDlrtt4+Cl1cBROumF2cU7fAFhW45UAm3ndtnBR6xq3R/A==)",
      R"(CJ/SVmXrTXYhwm8CS0ri6v5kUmuwufrm2zxUaImLfdL2iBbNui4gWg4Sot+QQWlLz8aeOrwwBsSO9JKkyn1xuA==)",
      R"(FklHuKltxiuFQbcn+9EbQ2VfJ5Qbtp6cyYp3abgtfmwuU0tHfBlqfNTUWfa8Ui9NVWjXi4lsNv7zxYL7K8DljA==)",
      R"(Au7mGjI/gSRwRXW/+hNqsCAXAzPgRMo98jqXdVK1vY6e6A4ycASI8n370p7f39a01pMrKnb6VRRJJGvTvf/P8Q==)",
      R"(RKKb+uW+hF36ukHPSwLjh8fLQYGJqHzX1mrCQCjs5iMNheGZRlF6mP4c1EZql8p+aFaZ36jg1xOXtOyEF088gQ==)",
      R"(15ztbRNtz3pU9rttKjOeBtSKrK4a4t1hGPkef/wni1B92ZG4KC31LbQBBSUKzljoacHCY7OyhgPuvjWNqpE/jQ==)",
      R"(OiXsHDy5c6n0Jdncui30fQaga+HdIRIVV31C+/gD5TxaoicZfH70gPXuqv6+NAGisXNPnRGupFA+Q68mgvw/Iw==)",
      R"(V8ePIKOqd2yvJh+WaZVG7P43gMmYI4gIBkdXo57ma8wEL57N+k4T2RKknrVdgQeilBkzzIb5KIun3tDu9AuEDg==)",
      R"(/ZfbQX5o7QbMmqaqm3jzuVcHjYowLDlV1zLhex0RBjOobDwndhNyZeRhoOqUQFrgS5tHFn9SRF0AlSFwd2JH9A==)",
      R"(iQkT7Z4mhNylLFZ7WLOdYcM2x60f0hOublqBGIYFjUYxk5zdynNJtId1HlyQ9SSTRku4e6/txOIciYEDUj/gRg==)",
      R"(AhgyLs+sKpxwWilA4vusIXV0KmgC2gC4EDXYQtUKCn/PCt8vKbQmUnG2eg3IOTLVaq0/O7l6NkfdUUBIMtgFcw==)",
      R"(F5ZCIStqAXxAWoGtZQcvo2IjvTHdWybN3pP7OkcxG7e44IhicC2ETToqJUvWpD+1h7EKlm4SvxqKSiXyDKOedA==)",
      R"(pk1dShFTH61C9vrANLoWNry7zb0HWTYH5IX7G2/uVaGjFUXWgrJOzRDTet938/v7jcqVqhfRFrXrQ/tGvVEMJA==)",
      R"(BPXrBvOkr2L6elCaY86fRWIQOwa0IP2bx0DN1hNcotuD1mZgECAYnVH+J1rHwqQtTDztea3UTuR8+N+bgHybyA==)",
      R"(x0ZQkkeA7pLD1fDLOBS59yUjhWmo2BkeeZIInXfg/g/nQEm2UGUuAlzrHda+XLazHnoZREaLP1yAzzEiOjFZtw==)",
      R"(usir+ZMjKzZFBSHaBtLDzoLpQut7FzpnCF06i4CWZ17VH5IL9qBmV0WkBgMyfvthtjP4tcRoYB+q2PC4bfnB1w==)",
      R"(H6Xnh1CxmhILBxminu2dHpJyor/vis8LEWRnsHzaUDb9H1ffJl2AjEHA7z2gNWf6XS35bdTKL2bSFNAz9tiu1A==)",
      R"(VV6ZzHhu3UQI/vQoXoZbsB6XPLMnGwtXACQDofKxvexJ1kd69RVX9sUSQ8l6f1r2fkQrY/wjYtRd94QDrtpLkg==)",
      R"(VVBri+U/df3inE+9b83RrUDaQqpoaQWDrtV9NKUbMXpAGypbLBik+6wayFS1TGOYy/YTO024AGoWH3VXNapjTg==)",
      R"(2Qc5OErDr7bSh8rPjmW/bTcmf7IjLjjSdrxCu3cstAaYz9azD/kE8S6H0b2I3YrRKmehmHznUVfFV+mrjGwsvQ==)",
      R"(VP6wYjOCCI0AEXfYcyTFc7ufz4i28lo8RTmIVzZl9TsF2wvvsWqwB8Yvd3iuc8yqHf6sTKdjIxarIHx1aNJ0fA==)",
      R"(5ePNEDI1lWVhJcd2TyFCE1FLJwhzQUIES/w0YCy0C9+1dwT8KPjnUoWsRc3ey9toeZ6KcX2+wAiTFDtbjNeWiQ==)",
      R"(uXo5mB3gx2jlwRobaY/xavCyfSHHfwe8PRAai1jgCYRyrtHtCCtFTFF0k88YWOO22LHHUeRy9W+znC3mwu1kng==)",
      R"(Cr+85HfJLSseyS1Ah3lxkbBE2XqBGfir3vJ78sxzhTUFM9emwdNzqT6wqc0tXk2SbY3C2WXGUKCPYwDEZN5FTw==)",
  });

  return *token_preimages;
}

}  // namespace brave_ads::cbr::test
