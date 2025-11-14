/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_test_util.h"

#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"

namespace brave_ads::cbr::test {

Token GetToken() {
  return Token(kTokenBase64);
}

Token GetInvalidToken() {
  return Token(kInvalidBase64);
}

TokenList GetTokens() {
  return {GetToken()};
}

TokenList GetInvalidTokens() {
  return {GetInvalidToken()};
}

const TokenList& Tokens() {
  static base::
      NoDestructor<TokenList>
          tokens(
              {
                  Token(
                      R"(/mfTAAjHrWmAlLiEktbqNS/dxoMVdnz1esoVplQUs7yG/apAq2K6OeST6lBTKFJmOq7rV8QbY/DF2HFRMcz/JVkVTu9dLQdR595gZf/D4PvSuhgk5RcoBm3fSFGI4JQF)"),
                  Token(
                      R"(8o+uKml91AthZzZgdH0iVGtZHfK2lAmPFdlhfF8rXnF67OtwnzHntFOimAmE73L7d9zh2sl0TJMv0mYwFf3zfsLUKsTlhuI71SwQsb/wOPYS/FIy5ldvxgam7dZCehcE)"),
                  Token(
                      R"(4F+VBba3Wv9CZx5HF/R217w4M4B7v4xAWTpJZ3bCIog03tm1eHrsRgCn1XGFxrugV7U/C5+waLadwEk9vqB233U3gps93CiySTbmDngv8xPYE1kqLz7IzjpWn2MuVvMD)"),
                  Token(
                      R"(Qu5OVNnHXfXB+aj6dJqV2B68pEfYwdevhMMBvlC/w4o81XE13y1b8dGYdDmN30ct4LacOJ8d/s8CVt2HZ9+P9sFCRuTv7PS5o9LgescSqgzF7q5G6vlN4vjDIvrKtFMH)"),
                  Token(
                      R"(bTChDKb8bO4406rN6Kazn8rMjaFE+8VHsGjhRJQOhhuiE7iPkD1x8Wh16Iw2KVgB3F/gzp48TSLloErwrpGeT3nSw/uK3i2WGX88Taw5akjU1rGlBSfi8HjopMwgMywG)"),
                  Token(
                      R"(VJpMA/p6XOysRoKyogmT3c8GzCmuOvPOfPFEQECU6RE4mmrG0iDSM7VbMYTJQNCUU2Gwh021TafmcS6SUO5le68yBUTqiUAMheF9utim3NNbSpRkXJtQBBZwgND0rukO)"),
                  Token(
                      R"(yxV23PfA329B7GwzAKDoooS8qBUby82ldcQibOd0xAwbcqpp2fURn9JaDApnyO6NT6eFMfXXzvghOJJAWBSTJbQQRAnWgVW02QqZYfjnhuykhcem2g8/XwpErup+SjcL)"),
                  Token(
                      R"(h8avY/txLkex4vX8cROpzCFqY2ehPZIw4avYWpkUUs6FuY/X3WNQBAv6k8BNYo2BMyw9WS75hO6NHd6ZHH/vC5lfWV3IQLMvw3gOOV8rslDAS7zEcQO0393z9MD0ahEA)"),
                  Token(
                      R"(+NDdXqzN+fBKzZsahecJ8BjKygzxxVklMeqZaQTBDIL8ymSL92xC0ArV/l5cJstOam3v3lm9dxkKaS/kq2xtNLZSyVwwPbt6Lk0Jwy71hLalKRp2bjyLSu2NILiezYIK)"),
                  Token(
                      R"(iP/zkjmYnlxQQAPYshasqwM9aY6dcSnFWrgZ8PGwMv8cSoN0c1ZWBqB02iMKJ8tU9Z/cu4aOib6H+nCER/qCMPSTIc8VL7LLZYcCviIkFo8qEQNPTLKzUj89M5vmTYQM)"),
                  Token(
                      R"(QUWt9T/TFI9H+6f3IHyZE1MDqZVte4sP7Bm9RItz8EL4MLabEVOJPFwJ+MMMqXKa1yAbZLk7CAPALRTAoQWhtf1gqyS1iZJ9AKxzfoCWCpIOQjpcvNJDd4BtZgIsASEK)"),
                  Token(
                      R"(IfiUpAFFGEogSCEJWn++VNMtE5D2FPAgUaZq7bT4zn+POnCHSTRTdCYfcAWT2O47bFXcW+hWRO26nqZG0nbGMyqVfL86jZRflcXaoLcQ/hyFr8O/1SFcrwa6xCd9p6cL)"),
                  Token(
                      R"(8PvoTrzyIRxOQN3pF+i6pBUsICiafyrtFzK2y3NKwPpEfhwySbWi0OmI4/EfRjH/8B1pYhC2e5ocfXxMQJg/80WqrCUcyzI+peYC834YIVPE+Q8BA9JkUBpQdwj3sf0B)"),
                  Token(
                      R"(B3ba4F/Z+7ear6hgVXbb+HqtY29zt9/08wqIiRAruCmz26JYrKK36FsHVoaWHVXbqE50kKa6EmBuwxk8K/p0DbVeJG4WhqniIUoJsqWkJyxmJ7dPzlAHggwL4y+buEkP)"),
                  Token(
                      R"(iBYbz22bz4DkUYFPvjxV/qQVvpwyBSgSo+28e1Fat0CqFBHzFtzvQqg/U2YBbuGoBG28pkyPOd90jyu6X/gDvjIKHXViDxE12QUV/dJXeAuUyirpGHuqIAyM+3orzTgP)"),
                  Token(
                      R"(RddwoyJZ+MNSrSAbRMd6pdRWZCZiwtTecfBT6CwEAsrPPckdHnEohw52y8Zfd8E3CJL6PglGe45rStDxuIGGTX3uZPztMDymld7DOCFYdviDoi6lqdk139Dln+OYiTIA)"),
                  Token(
                      R"(nKwtChzFdVT/u9NwjsnWJ6S5wsd2zHUBumOQgjKNqAMhdKU3XhhEbOq+8PuNEPTD59IgPjsli7/jTghoNkE/zFzRF+D4fcVp+/ibr/Y/2Ds7GChOwNgHrG6wLiXX1IgG)"),
                  Token(
                      R"(tOyts/yWbWoIZ80CALNDz5iYbuyZ0kmtFG3maOIdK/MiTY7j2kEdaQa7zA/TGw/0nq+mOb26o39j6CVoQCKf3+N39pMPE/YwUWtrgn+YZY1O/9atBc3nL12L8X3Och0A)"),
                  Token(
                      R"(AX0TXD4Qp8J+dORSHwvBU6Ns0AxeaNwZHxkurlIsrR7L5Pzuci2kvSDi9qomjUsA7byhWtIcOwPTL7NUPzaTLFX0o+AT18gS2EaGBGWhgknDqvp9rMcV5S1/8tc+pyEO)"),
                  Token(
                      R"(fM+qLel3ljJMgZ3CBcDoo2KjZC/nMbnQReo7f1uNfnoQkW+5nHYMm7b4IheJ9llg1vU8I8pOllcXZTWlI7dw9JCJS2QX30flwR0cRWJ7IbHa1LjLcamPOXblqiw2zcMB)"),
                  Token(
                      R"(vh3GD+slh2KDvKG0RWQG9BmXwvUX7ZVrpHPXTPLGqhooc+3/Dfh0BnHVEA1aOt0H+jqfpK+gzeJwZPB+uE6h4LPvVCiunLRsFgziXXw/XMk1qAk4oEoSJpslr7ff8AEA)"),
                  Token(
                      R"(MLoNQTqT5VnEp0f30JB5014MguJ0nxq+r9p3KGJL+/ATOEwdMQh2Hwah+3nfmTdmS2nBxA0msxFn9hRZ6tAZUy/Q5WGyMFOBcZXabeO0f9LVD96xk3S6BMOg+DPgnHoM)"),
                  Token(
                      R"(k5ruhL1W9dtuUdnnNj/FDE0cHeLUR0+KLMWoNQ5kTbSjJkt7cw1hcEkkLVb+v9k8+dGugEktHiodhGOOngbdM4w9E86rnThbirq+SndrVEaeytPNfbHzfO0fyT0PqZAI)"),
                  Token(
                      R"(17i178+xnyy1hgK8Noy24I+YFtqg+VgDbN4KFyPYJiNXkI7pyA05+PGewcFByz+OJM9NjQK0D24hJVm8lg6kLyNFENW6Ztetv+EVdQ7xeM3H2tIsf4XJD5NyDwTNEpYM)"),
                  Token(
                      R"(gIy44kVuFM7OIdd1iOXhez/wfIwZtF+M7Hmx38CQdivUT6545tIk5Il14czyXk/yXy/Parn0Sn+SD8Oc/ZltTYwZlUtuNiVHqWVZ2gWxymU7MXhl7I+JUScVgw/S9NMO)"),
                  Token(
                      R"(vST4PalUIaZRZJWFyq6oecP3EG6Q7LgMVYejXHb/QfGARMuB3+qiRNPDq5eX8A6CzzWQkgi5xKmPlnjOQiKYW2NokBrV9cjUOcQwhQAPN/b207+0v2h7d04DcM8VUqoD)"),
                  Token(
                      R"(MP6Z2d0QR4hQRWnVm0gRKvhmi8N8C3wT5iOHPDjTeDlrtt4+Cl1cBROumF2cU7fAFhW45UAm3ndtnBR6xq3R/KKvwW7I3iZBYHYsTxPgOGXVpt9jfRz0G+pC9UlRMm4I)"),
                  Token(
                      R"(CJ/SVmXrTXYhwm8CS0ri6v5kUmuwufrm2zxUaImLfdL2iBbNui4gWg4Sot+QQWlLz8aeOrwwBsSO9JKkyn1xuCtzcNf0aQfnFvrxJO+xLWvsE5CFCQXBKDRIwBbkzHcD)"),
                  Token(
                      R"(FklHuKltxiuFQbcn+9EbQ2VfJ5Qbtp6cyYp3abgtfmwuU0tHfBlqfNTUWfa8Ui9NVWjXi4lsNv7zxYL7K8DljCAKdyBm7dCPkcEa9VzGoE+FVr70+A5g5+RpLAjbTgsC)"),
                  Token(
                      R"(Au7mGjI/gSRwRXW/+hNqsCAXAzPgRMo98jqXdVK1vY6e6A4ycASI8n370p7f39a01pMrKnb6VRRJJGvTvf/P8S/yoq5Vid9BPcgWKhh5HD3oqaUg+BJhIE7QCTFpFbcL)"),
                  Token(
                      R"(RKKb+uW+hF36ukHPSwLjh8fLQYGJqHzX1mrCQCjs5iMNheGZRlF6mP4c1EZql8p+aFaZ36jg1xOXtOyEF088gXbls/gdxOPRmmnrARQc4khLuwuwjE0GVtEuxl7Q7aYF)"),
                  Token(
                      R"(15ztbRNtz3pU9rttKjOeBtSKrK4a4t1hGPkef/wni1B92ZG4KC31LbQBBSUKzljoacHCY7OyhgPuvjWNqpE/jXNSbnyBwAylpciaZRqLyfoyT693OUIegpD+pJR1BfUH)"),
                  Token(
                      R"(OiXsHDy5c6n0Jdncui30fQaga+HdIRIVV31C+/gD5TxaoicZfH70gPXuqv6+NAGisXNPnRGupFA+Q68mgvw/I5dgdwIfxiJwwff3u9zvk0rZokzi+zidCzbgLvKWKRwO)"),
                  Token(
                      R"(V8ePIKOqd2yvJh+WaZVG7P43gMmYI4gIBkdXo57ma8wEL57N+k4T2RKknrVdgQeilBkzzIb5KIun3tDu9AuEDnYYkwVSFD3jcd71zjw+5CxPDiyGlbyLJoUhOgu0JU0A)"),
                  Token(
                      R"(/ZfbQX5o7QbMmqaqm3jzuVcHjYowLDlV1zLhex0RBjOobDwndhNyZeRhoOqUQFrgS5tHFn9SRF0AlSFwd2JH9E7Hr57TZ8geEo7uoQiK8gAJYoRWXfOMINLB9b8/rS8H)"),
                  Token(
                      R"(iQkT7Z4mhNylLFZ7WLOdYcM2x60f0hOublqBGIYFjUYxk5zdynNJtId1HlyQ9SSTRku4e6/txOIciYEDUj/gRgXofFO1/88lrekyjtCayUpsR6Q6ObsNRytkhr5yAAkO)"),
                  Token(
                      R"(AhgyLs+sKpxwWilA4vusIXV0KmgC2gC4EDXYQtUKCn/PCt8vKbQmUnG2eg3IOTLVaq0/O7l6NkfdUUBIMtgFc2J25e9RVqGj89mUHt86TE9yIR1jA1yH0ntNKpsjjnoA)"),
                  Token(R"(F5ZCIStqAXxAWoGtZQcvo2IjvTHdWybN3pP7OkcxG7e44IhicC2ETToqJUvWpD+1h7EKlm4SvxqKSiXyDKOedBkQQWW+Jpr244B7MKATw+XNIB31SaHKiJhIVBAe6TkP)"),
                  Token(
                      R"(pk1dShFTH61C9vrANLoWNry7zb0HWTYH5IX7G2/uVaGjFUXWgrJOzRDTet938/v7jcqVqhfRFrXrQ/tGvVEMJHzVf/Z5oM+btr1YQwN3YCfevmktYX6G8jmQZqXZ0rIM)"),
                  Token(
                      R"(BPXrBvOkr2L6elCaY86fRWIQOwa0IP2bx0DN1hNcotuD1mZgECAYnVH+J1rHwqQtTDztea3UTuR8+N+bgHybyGTmNej6sXRaw6Nf2t8IVTO09MPlfFICPkgO2jCn2SsK)"),
                  Token(
                      R"(x0ZQkkeA7pLD1fDLOBS59yUjhWmo2BkeeZIInXfg/g/nQEm2UGUuAlzrHda+XLazHnoZREaLP1yAzzEiOjFZt+uInayChjCygETw86y7a3Nj1Yia8SwWmvAvSXTCSXIK)"),
                  Token(
                      R"(usir+ZMjKzZFBSHaBtLDzoLpQut7FzpnCF06i4CWZ17VH5IL9qBmV0WkBgMyfvthtjP4tcRoYB+q2PC4bfnB1/d1TsjZneMzZnK/ClfCGGn22k+sEIbhytZDaVmlBqUI)"),
                  Token(
                      R"(H6Xnh1CxmhILBxminu2dHpJyor/vis8LEWRnsHzaUDb9H1ffJl2AjEHA7z2gNWf6XS35bdTKL2bSFNAz9tiu1DA2yBF0VUkb0rOLVvzk/b+bC6XaC5Wfr9lIXQTBciwP)"),
                  Token(
                      R"(VV6ZzHhu3UQI/vQoXoZbsB6XPLMnGwtXACQDofKxvexJ1kd69RVX9sUSQ8l6f1r2fkQrY/wjYtRd94QDrtpLkkotRlz9EiWg9w9XsLMVLn8nwrnl334xA3NUyPHbRm0O)"),
                  Token(
                      R"(VVBri+U/df3inE+9b83RrUDaQqpoaQWDrtV9NKUbMXpAGypbLBik+6wayFS1TGOYy/YTO024AGoWH3VXNapjTpCGBN+0T02D6nJ6hR3maMXryhNNA5Zo7RkSg9ZkvPAB)"),
                  Token(
                      R"(2Qc5OErDr7bSh8rPjmW/bTcmf7IjLjjSdrxCu3cstAaYz9azD/kE8S6H0b2I3YrRKmehmHznUVfFV+mrjGwsvfBD6pPXY1VehVjjvjK9j/7JBNc87jWaGuYgHxbSoe8G)"),
                  Token(
                      R"(VP6wYjOCCI0AEXfYcyTFc7ufz4i28lo8RTmIVzZl9TsF2wvvsWqwB8Yvd3iuc8yqHf6sTKdjIxarIHx1aNJ0fOg8Mz4mI8j0RgXoIysJu+5m27aDnFwwO6ekhNgyPSUI)"),
                  Token(
                      R"(5ePNEDI1lWVhJcd2TyFCE1FLJwhzQUIES/w0YCy0C9+1dwT8KPjnUoWsRc3ey9toeZ6KcX2+wAiTFDtbjNeWiTQTzfBQjbBNPFwGQSqtzJ4lf+zzLqSr3U0vbI8iPFsM)"),
                  Token(
                      R"(uXo5mB3gx2jlwRobaY/xavCyfSHHfwe8PRAai1jgCYRyrtHtCCtFTFF0k88YWOO22LHHUeRy9W+znC3mwu1knqrzAd2P82OEYGTAaHdVwydSsEKUCG92P4KQSlv+RhEL)"),
                  Token(
                      R"(Cr+85HfJLSseyS1Ah3lxkbBE2XqBGfir3vJ78sxzhTUFM9emwdNzqT6wqc0tXk2SbY3C2WXGUKCPYwDEZN5FT64SXZFySDJVBiAcptoYQVqzCDCwixQTlhYtm2OKWDYA)"),
              });

  return *tokens;
}

}  // namespace brave_ads::cbr::test
