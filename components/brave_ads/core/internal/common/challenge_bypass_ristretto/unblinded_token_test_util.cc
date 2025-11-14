/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token_test_util.h"

#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"

namespace brave_ads::cbr::test {

UnblindedToken GetUnblindedToken() {
  return UnblindedToken(kUnblindedTokenBase64);
}

UnblindedTokenList GetUnblindedTokens() {
  UnblindedTokenList unblinded_tokens;
  const UnblindedToken unblinded_token = GetUnblindedToken();
  unblinded_tokens.push_back(unblinded_token);
  return unblinded_tokens;
}

const UnblindedTokenList& UnblindedTokens() {
  static base::NoDestructor<UnblindedTokenList> unblinded_tokens(
      {
          UnblindedToken(
              R"(/mfTAAjHrWmAlLiEktbqNS/dxoMVdnz1esoVplQUs7yG/apAq2K6OeST6lBTKFJmOq7rV8QbY/DF2HFRMcz/JTrpqSWv/sNVO/Pi8nHDyl3CET+S2CKkMmYlXW3DgqxW)"),
          UnblindedToken(
              R"(8o+uKml91AthZzZgdH0iVGtZHfK2lAmPFdlhfF8rXnF67OtwnzHntFOimAmE73L7d9zh2sl0TJMv0mYwFf3zfsLb7m0IE1e3UqGPVaKP3B+rLmCjbSau+mfzjqQ8OQMS)"),
          UnblindedToken(
              R"(4F+VBba3Wv9CZx5HF/R217w4M4B7v4xAWTpJZ3bCIog03tm1eHrsRgCn1XGFxrugV7U/C5+waLadwEk9vqB239LqecZUunqkY+vJ9bxNkWPUglLQ1FqY7mQ+WpEGmowV)"),
          UnblindedToken(
              R"(Qu5OVNnHXfXB+aj6dJqV2B68pEfYwdevhMMBvlC/w4o81XE13y1b8dGYdDmN30ct4LacOJ8d/s8CVt2HZ9+P9jRNMjwzSLhfVBDLcQ1TdaLT6Ab7XG1Z4DrYGxKnsplZ)"),
          UnblindedToken(
              R"(bTChDKb8bO4406rN6Kazn8rMjaFE+8VHsGjhRJQOhhuiE7iPkD1x8Wh16Iw2KVgB3F/gzp48TSLloErwrpGeT8SjqyV1C0+5iF46YGxZIYzIeN5vU3grxB4Z3/K2ToZE)"),
          UnblindedToken(
              R"(VJpMA/p6XOysRoKyogmT3c8GzCmuOvPOfPFEQECU6RE4mmrG0iDSM7VbMYTJQNCUU2Gwh021TafmcS6SUO5le3yFAskSYZ9BGLJpEmL1Sz/yxeAqKDAcaKXJr97K8NgI)"),
          UnblindedToken(
              R"(yxV23PfA329B7GwzAKDoooS8qBUby82ldcQibOd0xAwbcqpp2fURn9JaDApnyO6NT6eFMfXXzvghOJJAWBSTJVSqWe9ScXyOZg0ufl1BRAkmqpvfDWqIlB22YgNRYGpv)"),
          UnblindedToken(
              R"(h8avY/txLkex4vX8cROpzCFqY2ehPZIw4avYWpkUUs6FuY/X3WNQBAv6k8BNYo2BMyw9WS75hO6NHd6ZHH/vC/YwRA9eThulT5IL+P17VDSHET4kSX2kuVljF/cG1P8l)"),
          UnblindedToken(
              R"(+NDdXqzN+fBKzZsahecJ8BjKygzxxVklMeqZaQTBDIL8ymSL92xC0ArV/l5cJstOam3v3lm9dxkKaS/kq2xtNH7fqvaa2nNX3fo6KLA7opxbfS9owGNn6kFPeeurJmwd)"),
          UnblindedToken(
              R"(iP/zkjmYnlxQQAPYshasqwM9aY6dcSnFWrgZ8PGwMv8cSoN0c1ZWBqB02iMKJ8tU9Z/cu4aOib6H+nCER/qCMPoFPZgKPQiQNDDAwW8ZQSmQsv3jaVJMpv7RC2u7BgQ3)"),
          UnblindedToken(
              R"(QUWt9T/TFI9H+6f3IHyZE1MDqZVte4sP7Bm9RItz8EL4MLabEVOJPFwJ+MMMqXKa1yAbZLk7CAPALRTAoQWhtVawO/11QqrYPJoj0TaU24cd4At3UFZ5KvVUBpvxfZ1W)"),
          UnblindedToken(
              R"(IfiUpAFFGEogSCEJWn++VNMtE5D2FPAgUaZq7bT4zn+POnCHSTRTdCYfcAWT2O47bFXcW+hWRO26nqZG0nbGM8pFbbMoqjujbQ+6fMGtny4FKptrWR2kTZpuHp4Ak1Uf)"),
          UnblindedToken(
              R"(8PvoTrzyIRxOQN3pF+i6pBUsICiafyrtFzK2y3NKwPpEfhwySbWi0OmI4/EfRjH/8B1pYhC2e5ocfXxMQJg/87LrBn1LdamYzB/AiFwvHydlZAMf6gIl4H7yPgvJwuBB)"),
          UnblindedToken(
              R"(B3ba4F/Z+7ear6hgVXbb+HqtY29zt9/08wqIiRAruCmz26JYrKK36FsHVoaWHVXbqE50kKa6EmBuwxk8K/p0DZrTIhdthOSpwJ0U7oMjUM00A/ObfbJSBk4dIxnd11sW)"),
          UnblindedToken(
              R"(iBYbz22bz4DkUYFPvjxV/qQVvpwyBSgSo+28e1Fat0CqFBHzFtzvQqg/U2YBbuGoBG28pkyPOd90jyu6X/gDvjDh1/73iZoNpuTlufKelnJqsHl4W8FI6wNYxUA2sisz)"),
          UnblindedToken(
              R"(RddwoyJZ+MNSrSAbRMd6pdRWZCZiwtTecfBT6CwEAsrPPckdHnEohw52y8Zfd8E3CJL6PglGe45rStDxuIGGTVAZZmLXBcQ+ahfFsrdTr+MTJhGD+EGVKvDsXjvHhndg)"),
          UnblindedToken(
              R"(nKwtChzFdVT/u9NwjsnWJ6S5wsd2zHUBumOQgjKNqAMhdKU3XhhEbOq+8PuNEPTD59IgPjsli7/jTghoNkE/zLxUYIyzipZzlGqWsW9LP+akGLhMmHKj1IKdJE3GDmlA)"),
          UnblindedToken(
              R"(tOyts/yWbWoIZ80CALNDz5iYbuyZ0kmtFG3maOIdK/MiTY7j2kEdaQa7zA/TGw/0nq+mOb26o39j6CVoQCKf34wco6DYgv4qnRwtw+drj0TNAMch2N66ahg8ZVHh7LJN)"),
          UnblindedToken(
              R"(AX0TXD4Qp8J+dORSHwvBU6Ns0AxeaNwZHxkurlIsrR7L5Pzuci2kvSDi9qomjUsA7byhWtIcOwPTL7NUPzaTLPhgxaD6QZCdug8FMQC4ZsS+FXuzq3yONEW7Yuu1/b0J)"),
          UnblindedToken(
              R"(fM+qLel3ljJMgZ3CBcDoo2KjZC/nMbnQReo7f1uNfnoQkW+5nHYMm7b4IheJ9llg1vU8I8pOllcXZTWlI7dw9LK0AWVOA33a24Djiz5qlYXVtBhgy3gsBi4vub5qf4ts)"),
          UnblindedToken(R"(vh3GD+slh2KDvKG0RWQG9BmXwvUX7ZVrpHPXTPLGqhooc+3/Dfh0BnHVEA1aOt0H+jqfpK+gzeJwZPB+uE6h4OB+sxkg7TsgBmIuIE8njCCxV2b7L0Aj2WPmV+hLWaU6)"),
          UnblindedToken(
              R"(MLoNQTqT5VnEp0f30JB5014MguJ0nxq+r9p3KGJL+/ATOEwdMQh2Hwah+3nfmTdmS2nBxA0msxFn9hRZ6tAZU7KiJle1Oi2qJop6MO9ceD5F+pzzyxO96Wigr1ezPHEJ)"),
          UnblindedToken(
              R"(k5ruhL1W9dtuUdnnNj/FDE0cHeLUR0+KLMWoNQ5kTbSjJkt7cw1hcEkkLVb+v9k8+dGugEktHiodhGOOngbdM8S1TM1uIOGA61IhwSk9VxFaE7AiDX/AmGfg341zjoZU)"),
          UnblindedToken(
              R"(17i178+xnyy1hgK8Noy24I+YFtqg+VgDbN4KFyPYJiNXkI7pyA05+PGewcFByz+OJM9NjQK0D24hJVm8lg6kL8BNvK+9OGiIKwfujojLfJuZYE5PcEP3ri+x8NGVa/No)"),
          UnblindedToken(
              R"(gIy44kVuFM7OIdd1iOXhez/wfIwZtF+M7Hmx38CQdivUT6545tIk5Il14czyXk/yXy/Parn0Sn+SD8Oc/ZltTRjohh9TBupuoPkZHUm1trdOckr9spDiD4wNPVJdK+Nm)"),
          UnblindedToken(
              R"(vST4PalUIaZRZJWFyq6oecP3EG6Q7LgMVYejXHb/QfGARMuB3+qiRNPDq5eX8A6CzzWQkgi5xKmPlnjOQiKYW7CV5ZmcyZacOeB3Mrt2rD33XFHuk042qS9sGKsll617)"),
          UnblindedToken(
              R"(MP6Z2d0QR4hQRWnVm0gRKvhmi8N8C3wT5iOHPDjTeDlrtt4+Cl1cBROumF2cU7fAFhW45UAm3ndtnBR6xq3R/PT6GFGXd9WJmoq/ItPfZSTv9VeG4I0NHZ1lXeuxM0ZT)"),
          UnblindedToken(
              R"(CJ/SVmXrTXYhwm8CS0ri6v5kUmuwufrm2zxUaImLfdL2iBbNui4gWg4Sot+QQWlLz8aeOrwwBsSO9JKkyn1xuK7iBdTBz3LRt+5iaj0YMPNLyZTbz3Ydeo5byyBHighR)"),
          UnblindedToken(
              R"(FklHuKltxiuFQbcn+9EbQ2VfJ5Qbtp6cyYp3abgtfmwuU0tHfBlqfNTUWfa8Ui9NVWjXi4lsNv7zxYL7K8DljB7OoM52ttyB0W/6Tr5r92WbVTHqpyI+BPoHL7G0gtYZ)"),
          UnblindedToken(
              R"(Au7mGjI/gSRwRXW/+hNqsCAXAzPgRMo98jqXdVK1vY6e6A4ycASI8n370p7f39a01pMrKnb6VRRJJGvTvf/P8SouSSs5yJqoN4eizf0bOv8RI9DoFNyDn00d+gzg5swV)"),
          UnblindedToken(
              R"(RKKb+uW+hF36ukHPSwLjh8fLQYGJqHzX1mrCQCjs5iMNheGZRlF6mP4c1EZql8p+aFaZ36jg1xOXtOyEF088gWi2zv86w0o/1g7QsCCEX/czE+1Jo/Hd1HaTlp4N1uwn)"),
          UnblindedToken(
              R"(15ztbRNtz3pU9rttKjOeBtSKrK4a4t1hGPkef/wni1B92ZG4KC31LbQBBSUKzljoacHCY7OyhgPuvjWNqpE/jeKyIt/Oxj0cgdZVDgcFrnX+/EEl7IjcDXftgwe4q4oP)"),
          UnblindedToken(
              R"(OiXsHDy5c6n0Jdncui30fQaga+HdIRIVV31C+/gD5TxaoicZfH70gPXuqv6+NAGisXNPnRGupFA+Q68mgvw/I4pHCNkUFcEKEHvP6rQ+oawlQ8mxbBEPosTq6MI8NOsh)"),
          UnblindedToken(
              R"(V8ePIKOqd2yvJh+WaZVG7P43gMmYI4gIBkdXo57ma8wEL57N+k4T2RKknrVdgQeilBkzzIb5KIun3tDu9AuEDireSp195fVpBtLTZfGD3eHpKj2gN+qgKp/ID4Nyjz8i)"),
          UnblindedToken(
              R"(/ZfbQX5o7QbMmqaqm3jzuVcHjYowLDlV1zLhex0RBjOobDwndhNyZeRhoOqUQFrgS5tHFn9SRF0AlSFwd2JH9HRWN5GQYfzy0Ba5hY0mV1p0+apwEGtRIRLoIMFbzhwg)"),
          UnblindedToken(
              R"(iQkT7Z4mhNylLFZ7WLOdYcM2x60f0hOublqBGIYFjUYxk5zdynNJtId1HlyQ9SSTRku4e6/txOIciYEDUj/gRkpJ+eOXpHTCPxNto+fbyNsDGyQHjpmfik7jxhwiy1hY)"),
          UnblindedToken(
              R"(AhgyLs+sKpxwWilA4vusIXV0KmgC2gC4EDXYQtUKCn/PCt8vKbQmUnG2eg3IOTLVaq0/O7l6NkfdUUBIMtgFc9hojYjRDmAxpDBZ53/OtJKlI05/getwKUGQx0l8eRJI)"),
          UnblindedToken(
              R"(F5ZCIStqAXxAWoGtZQcvo2IjvTHdWybN3pP7OkcxG7e44IhicC2ETToqJUvWpD+1h7EKlm4SvxqKSiXyDKOedNYFK/BQg2N7sABDs7EzmhbDusFYNZf8MRq7Tzo43gYI)"),
          UnblindedToken(
              R"(pk1dShFTH61C9vrANLoWNry7zb0HWTYH5IX7G2/uVaGjFUXWgrJOzRDTet938/v7jcqVqhfRFrXrQ/tGvVEMJFIwqBDp/Mef+X0UrJJbBBS5SsvlPe5Xjpj9mwM8CIMP)"),
          UnblindedToken(
              R"(BPXrBvOkr2L6elCaY86fRWIQOwa0IP2bx0DN1hNcotuD1mZgECAYnVH+J1rHwqQtTDztea3UTuR8+N+bgHybyBSCfEsdMauHmLGqUEAqJwl2/+OCSE7XvEAhoq/2FWVO)"),
          UnblindedToken(
              R"(x0ZQkkeA7pLD1fDLOBS59yUjhWmo2BkeeZIInXfg/g/nQEm2UGUuAlzrHda+XLazHnoZREaLP1yAzzEiOjFZtyww7fyj73tWaEsXhXACE5GqwnEvWGctuyeh0tlhK9Vi)"),
          UnblindedToken(
              R"(usir+ZMjKzZFBSHaBtLDzoLpQut7FzpnCF06i4CWZ17VH5IL9qBmV0WkBgMyfvthtjP4tcRoYB+q2PC4bfnB17KmCD4QECVTS8acLnP1NnINLSRSya2MDBKfrZsBEJ1C)"),
          UnblindedToken(
              R"(H6Xnh1CxmhILBxminu2dHpJyor/vis8LEWRnsHzaUDb9H1ffJl2AjEHA7z2gNWf6XS35bdTKL2bSFNAz9tiu1NKwb6orgiVFEBvyDGK+5OtT0DEGwJ/W7qsvHEJabnli)"),
          UnblindedToken(
              R"(VV6ZzHhu3UQI/vQoXoZbsB6XPLMnGwtXACQDofKxvexJ1kd69RVX9sUSQ8l6f1r2fkQrY/wjYtRd94QDrtpLkpZXZ1SvJKEVr/r2C6bnffduw6tTI7ze4lEz2S/ltJpG)"),
          UnblindedToken(
              R"(VVBri+U/df3inE+9b83RrUDaQqpoaQWDrtV9NKUbMXpAGypbLBik+6wayFS1TGOYy/YTO024AGoWH3VXNapjTuCjFnm8MXzeyuDQDXqRI87L4nAeclytgwBUL2j802dJ)"),
          UnblindedToken(
              R"(2Qc5OErDr7bSh8rPjmW/bTcmf7IjLjjSdrxCu3cstAaYz9azD/kE8S6H0b2I3YrRKmehmHznUVfFV+mrjGwsvUyT0pCN2WGu7CcX2BuqZwy5UOoidhyNRyoiuG7KWk4U)"),
          UnblindedToken(
              R"(VP6wYjOCCI0AEXfYcyTFc7ufz4i28lo8RTmIVzZl9TsF2wvvsWqwB8Yvd3iuc8yqHf6sTKdjIxarIHx1aNJ0fEoqViYY/EkahNrqL2j/VC25yRFqc6Srphrl1rq7ZYwl)"),
          UnblindedToken(
              R"(5ePNEDI1lWVhJcd2TyFCE1FLJwhzQUIES/w0YCy0C9+1dwT8KPjnUoWsRc3ey9toeZ6KcX2+wAiTFDtbjNeWiZIRUekNTV7zZAeBg9qAqsOPBZQ4m1/cfgxPmn6UAdsw)"),
          UnblindedToken(
              R"(uXo5mB3gx2jlwRobaY/xavCyfSHHfwe8PRAai1jgCYRyrtHtCCtFTFF0k88YWOO22LHHUeRy9W+znC3mwu1knn6OqBV8YOXDSC/AcpJoO9gJV7nci7SJmwhc5QQmU5kT)"),
          UnblindedToken(
              R"(Cr+85HfJLSseyS1Ah3lxkbBE2XqBGfir3vJ78sxzhTUFM9emwdNzqT6wqc0tXk2SbY3C2WXGUKCPYwDEZN5FTzZWVkeyep/hJ6tPZ/Q4KtIGLDMhTK9MX/NQu1Khz8h3)"),
      });

  return *unblinded_tokens;
}

}  // namespace brave_ads::cbr::test
