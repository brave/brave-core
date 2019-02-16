/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "confirmations_client_mock.h"
#include "confirmations_impl.h"
#include "request_signed_tokens_request.h"
#include "security_helper.h"
#include "include/bat/confirmations/wallet_info.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

namespace confirmations {

class ConfirmationsRequestSignedTokensRequestTest : public ::testing::Test {
 protected:
  MockConfirmationsClient* mock_confirmations_client_;
  ConfirmationsImpl* confirmations_;

  RequestSignedTokensRequest* request_;

  ConfirmationsRequestSignedTokensRequestTest() :
      mock_confirmations_client_(new MockConfirmationsClient()),
      confirmations_(new ConfirmationsImpl(mock_confirmations_client_)),
      request_(new RequestSignedTokensRequest()) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsRequestSignedTokensRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
    delete request_;

    delete confirmations_;
    delete mock_confirmations_client_;
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  std::vector<Token> GetTokens(const int count) {
    std::vector<std::string> tokens_base64 = {
      "B2CbFJJ1gKJy9qs8NMburYj12VAqnVfFrQ2K2u0QwcBi1YoMMHQfRQeDbOQ62Z+WrCOTYLbZrBY7+j9hz2jLFL74KSQig7/PDbqIpmNYs6PpNUK3MpVc4dm5R9lkySQF",  // NOLINT
      "MHbZ2XgFtno4g7yq/tmFCr1sFuFrkE7D6JjVAmM70ZJrwH/EqYNaWL1qANSKXX9ghyiN8KUDThEhDTqhuBQ4v7gzNY2qHav9uiAmjqvLzDp7oxmUBFohmdkVlvWhxV0F",  // NOLINT
      "6WWlDOIHNs6Az23V+VM3QTDFFDkR9D0CGZSd27/cjo3eO5EDEzi9Ev5omoJwZQHqiObVgUXmRFRa8UYXsL4O4MvBsYlgGz9VyoBLXo0ethmEBowsrMubj3GR4CQaN6gB",  // NOLINT
      "IzhzMBc/rI8uzGuARaudvUYY662c0tqzYDPOfvbWRiThTTyH9fU13nmAmhkdtpoUnDlGTE37fLDpjWPlGdAd9r2qh++09+sa9xHV+V9SXHbr9gtJBybZMWr8vjQuslMM",  // NOLINT
      "eZDj3OGto3E0Uz0djk6Ilfgz+Ar4kMAXOL68iLTNycBPgoNnM1rtjaL4OqvSc1ascZhGCf6Js42B/wPVzUYuKMloATKmYs7Ym+ndXnuX0FV9XJs94tlIGcp4k0uOMcgB",  // NOLINT
      "8QNMIJuJfu9W4KURg1Y2coXyKjbJOQLmo6RIGg+tKkUcY7srgUpac8XteSwWy6o6YLDoNKXS21FmbZ4VHb+Bv2NVhBWooK0b8lwQAdVUax5+Ej77qK//GeyRmAcAQV8G",  // NOLINT
      "6ILvEIM3+kgacI6JFa5415qAdzcg6hccQzEyhMsqYFa3MZKzvcLEF57pFFRoaYw7nFDQL8v8CDG2iSUoBIk8bmeoUwgdXsgofHvSahcBSWawmcnn8ESJTkZPGgxaFgcA",  // NOLINT
      "VHDbhwcInhhjL/HhSF+NyYak7Zy24xzDDTpI+3rsEZ7iL4SYUdcVkFmJ+bg8QlmPv8UMTchPBP7CVtCc96jj5PwGMsvAB8t2TffdSK9SHBRx/ZINmYSb7x+GTTdqWugB",  // NOLINT
      "YbH2x8oMkQrPR0uX6h8LrcgXSrPlSg60FFfp8V+GM8eiCQTwPJ643kilmlKU/qNZM3e28Hw3W4GPAELnm/YxFzG6qJ4B1wVTBdl/myIa0M3QIdoOn2//+JH2u4jRtIgN",  // NOLINT
      "0/KAtyvRoYLhsQnwu4McuG7pglpDpi2BXQi//FwGu8m/O+iTh1Lijzpt2RCnotGh0Wid9efnojrYQH5NJv9GYOhUDX7yYHVjUorc6y6SkUaO1aATc42RciRQ0cmuQFQC"   // NOLINT
    };

    // Blinded tokens for above tokens:
    //   "iEK4BXJINfAa0kzgpnnukGUAHvH5303+Y/msR5+u/nY=",
    //   "eAAv7FNH2twpELsYf3glHLlOhnnlIMovIeEgEmcjgyo=",
    //   "1G0+8546Y6jCIUXG0cKJq0qpkd6NsnG+4w9oSVW3gH8=",
    //   "9gtgRG1Fr6eQAfvIO7qGes2d0Zwnd7EXdOQI9ik0PRE=",
    //   "iGH6L3EtdYLQiD63D/elY3nfI2R8BJzq/ufPtFkTAXg=",
    //   "5mtjGDYwCC54EyFrr/5XoG98Cag7ughIYYr6mp8jmEQ=",
    //   "8vU5KFc8AXn45rcqTGdM9MeUvG+z8RL9o27Lir4izBY=",
    //   "huXHzk2SgmJkMauedoRUr/p86+jh1vKIa93O9FP2PQk=",
    //   "cg9nMhSA7hVoBFbq5rEGVF7kgAoXqMmPApmxO99aGVU=",
    //   "sBJB0ez2qw929moV4PZgw+AVbj7mBj9Mtqy3r2D0kw4="

    int modulo = tokens_base64.size();

    std::vector<Token> tokens;
    for (int i = 0; i < count; i++) {
      auto token_base64 = tokens_base64.at(i % modulo);
      auto token = Token::decode_base64(token_base64);

      tokens.push_back(token);
    }

    return tokens;
  }
};

TEST_F(ConfirmationsRequestSignedTokensRequestTest, BuildUrl) {
  // Arrange
  WalletInfo wallet_info;
  wallet_info.payment_id = "e7fcf220-d3f4-4111-a0b2-6157d0347567";
  wallet_info.public_key = "3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf";  // NOLINT

  // Act
  auto url = request_->BuildUrl(wallet_info);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/token/e7fcf220-d3f4-4111-a0b2-6157d0347567";  // NOLINT
  EXPECT_EQ(expected_url, url);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest, GetMethod) {
  // Arrange

  // Act
  auto method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::POST, method);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest, BuildBody) {
  // Arrange
  auto tokens = GetTokens(3);
  auto blinded_tokens = helper::Security::BlindTokens(tokens);

  // Act
  auto body = request_->BuildBody(blinded_tokens);

  // Assert
  std::string expected_body = R"({"blindedTokens":["iEK4BXJINfAa0kzgpnnukGUAHvH5303+Y/msR5+u/nY=","eAAv7FNH2twpELsYf3glHLlOhnnlIMovIeEgEmcjgyo=","1G0+8546Y6jCIUXG0cKJq0qpkd6NsnG+4w9oSVW3gH8="]})";  // NOLINT
  EXPECT_EQ(expected_body, body);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest, HeadersCount) {
  // Arrange
  std::string body = R"({"blindedTokens":["1Fwm6gOsXS4wmzjQjbgk69X7Yd1IGjl8nhiKGWCzXTk=","4hPPIOdzoRogm847b/AuHqapRevwByYeyLOTdnQ3Kk8=","BFx/fw+mK2N5JHwfuWoYHOvchCE4eQD8YWO3ET2ym3Q=","jGJV/w4aAMbIWNswlgySTRRFQmIaRb+1IGeyaIObPTk=","gAPctDibVJNImTIqkSdHPQOp+l3hIbINYS2ZDD/tPH4=","CIIlpfjC4fC4hzFRgzQL74BCIWs1TKdlj278nBHWCzA=","JgDYrHaIa51yQOl9uCfDmfKSAxgZbGsJ74CUe/qN7zc=","At4/Hcjzurz9+4NVq0PIGGt8NpcKmNnfhTykDTktSWg=","Xpoh1XhWWs4sI7XmgfSaVEJiU/svFW1T6XNlsRGqKHk=","ytSdCx4H2hCNlWZc59GBfhFBHRchSEuf8pzYzVIrzwM=","SpXgjYqH746GTlcufFiTQcP0IxpuO9r0LoREv/1JQww=","XlyA1GlQQakgu9di+27nMjRz8afZ6w/Ak9Xw01XQzR4=","6oICEjxG2lPgRMoGFz7NdQnPSZ16lwCnBoxMK/U8TQU=","SuzYYzPXI2+09Do/7A8b5YUTpUk4aD06FNTt33gWOnY=","LrSe7y4YjW0E8FuHBQd6xjbueUacKMGKcQyF0um2Eyc=","VLvXp7wZa81N8YhWhTpJoEidbwzfc267oE0/Ku/oC3Q=","KKd26nziS0k4xlg+/l3K7zIrhUUbRunCKlQKpcYvNFs=","NN1D1KKhPc/2gZQA9Rdm1h7OPrXgAIIvORTS0hip30g=","vscyDf3W6PGOg3I94oTmGyjeKYyB0qNAOfl4jZhHpmo=","VIIQ6gsx64nY0VgLatIpTTUs9s2axYDk5u/YIDqd/Wo=","qsUHc1BfYhcgjGU9Nb9hWJXvjuRaiH8byR6WjWXSQQQ=","uIYoo9Y7PuBH8URfZL31pR6ILaP5nH3Ut4yRe6AwjHA=","mh6huuiT7m0RbGs5k1IC+PQc8cGneKxVL/89CvQ/tzA=","tGloa/PK2MJDZ9yR6LL2U96WlH+6M7Da6eBNosTkj1M=","YjF+ljBv6I9tKoGTb/g29jFtwiqSRy+GbFmqoy4UjwE=","zFNLZf1IbCAJWTK6UEM3+GGCQ4CO89wLlKWZ9w48kWI=","4kIdTNVujRlS1Bf+a23Vzt6PITHpupb/YMQFZdV1aj0=","0HLAUuAJadMSX8Sgn3gdPjXO9s9BDT3vvpS4Zrtu9kU=","NDGBAi0R4FPWt0XgMn1g0uxCWOiOY6SZ+4XFmZx/CBg=","dtXEsDHaTHt8WK5ChXAuZ4vg79eKgNthDrACCBiwOk0=","4oY5iFVdAyLekOT1/fLeZvcJEKR9imFph8AhngF0Pgw=","+CeRUWzu6VxlxO4I1ZTLqAf3rTQd1RrAiRoywmMVPCc=","HOcA3/LprxFZc0gNT3XPNZsiFAG36cOWBwJ+002l0Qc=","lEO24pANnAFyXcMZw03rJXf5rfuManuHcfAalJQkZ0g=","sFXgELiusqD9SdHor3cZijzrlyzetafMfiGGWI6YDn0=","GJPsWMTK65dYYYnPRem7cc9MX0M6Q8XtLwboN+gEPF8=","NKZa4oTiJj/d2yCYbwCFdaQg2iZKArvEjQBylB1JOlI=","ti5l9fVKX/BDSolaMILczmfzQ75Y2AjiTII6FRTdx1Y=","Gtxl2H2r5PbOx+iFP8dzbuCRAqy9IZPwx6ypzoioDTc=","bIAmxGFA4XQntsXFd3PLHpyszB4wo59jFeWMWPMt9SE=","aJ+Uv2QxWPktxhzJH4nsyRUX88u2lajBv0cZWpfZnlo=","ACea3T+vdzi/lEgr4XmrFlFLSFFIPGPDzkZhaQrbYH4=","Egwk5NjjEaWrapV7oDI2Qsscldrc4yBIReju78MRixE=","ALV+JaHwFt44WFrPtKW2Jf3mQGxRSWXrdE60ha3J6X8=","utiko2dlzzmjjTd1QNLFg2IGs+phwHFOQom42IUiQj4=","SIIOeOuuMtN6eZFKHOYHXI6a9/QZjQoBJLJZ6PLb4XE=","5I/n0MtuDg0QkGRSzN8NalT0bksM209XCqlfwll+elA=","diM0LIWxPXa6RbBXMSIqKMWnQtXwtf1Mga1zBgNO4HA=","jnmj38cOfm4JSmZ1GUlt/BT/QzjsBnKOASMgTONd2zI=","VpWmr3pnfZE1f5KqZYwYAUDi0K04ZvibcA/p65VVDk8=","niNAlSVamLJMs9dHSmQ7Sa5z/ucwb7aDi+ovW1OIelY=","nOORDPKHI5RFJ4BsPgNCljS46GOU4JUFBFi0a7rj63s=","4v0n60wYwj3po/wsnClhnHEtpai8PGAiRMZgoS4UhWE=","mAbAnTSP3lXwToKbs33YkRrWF73ITwQy6iNTv1WFCh4=","xAn14oqfjUaViF7kr1AlH8PiE7amatXuDJy/sFzfqX4=","chniTWNWqAFCHksp4dbT9eNyRUqAxgkuuYRL4iW8WwE=","dj3/xCOmQ1lHOBYTdC+tyJFFKuCBIjsR733R7ehk4m0=","jK/O0uJqBmqaObz7S3C9JCv6fwmK6mX0jz4wNnuxH3s=","JFDbF4/bRaT1lqfwMMMkYthRn48O2ENsvJeDN9OJx2w=","atx0/2ZAlYuv0GoK9LWzl+KvlGw+vR0RF0+YQg/3gQw=","Qioe+g5INKK4n8a4jRTp756JEza9vApNB7jhclG+LXs=","9nblEhmRdSJGQpnXAtc6GtMcL4FVz8/nt4Z9lGMiPE4=","JqbX3u/cNHqvwNTR2/3Gfhi4ULwrYzWtP5wlsBZVqGY=","BFyyYHCpu9wpXQBXjDRZ5i5LEa8dY1VeJu1DScZ6bGU=","FnafJYZBSOULPWtJZVRY9tUOjpgAq+aOygioN//CuGs=","pAwG7HboPVeuCGSXA7RXw+eDj667UApMr1KSUx5f7Qg=","LpTnNc4QCvC/DBLGdfKf7r/DS7A74eWhQ9BLrnEaLk0=","bE3mseRRFqEis+hRH3JEihOQLtQ6+D9nAs+I3czNGX0=","JFTpRiNm2Tg822sdZrLNvWDRsDeVUX06HplUBx2JOA0=","6s9GicBRK2Y5FGgCYhJO3q1qkG+NTN68V8MYEAGSCnA=","aM/DJiQbIjPrTa+vqbOEyBrJYcFRCAd1VWKhk+CC9gI=","6CCWq/HwRCsY1ZVBp8r0Lf/f8zhQMYuMVRuq7qKuEGY=","EpodVxvX9X6vHaK9MyLemhzWIzZR7HzXj4V7eZuXNkQ=","AolM6TCNMNXd1Id9ColrHjpx3ECJX0SChbTrjFYR9GI=","YDaHbVval/mHopvAgnVo9svvx2nHOSf6arRRdoJYF0U=","PPNHrkMM44EZDNeD0mYKQ4zJo1elpWDMBpo+UlGTFnc=","5AykA0xLBRW6tQD/q9mWuvgHJJXkNLytS54TMlafFlw=","9G8rZDCIqPYIhsD024b32DPG4qgRG+GCvheo5pcYLRE=","TGlueCCVRBm9L8pVeDaH9SfiRfMxYtZnbaGTh8BUrjw=","VCDHZyGXUANKXPiMAUdvcwGmEgSjF3Rp0p5m7wnaD2k=","PgRiNEWFmMoZbXDB8XTUf54kHptmdd+nkqFwZXMTbw8=","zNh/pmL0nU2EgioT0vstea1z8ueWLJYF/aPHZOFHf3U=","7DNsBs0J0/iAlMQ3/atXLPubCT/10fVYGy6jtc1F8Tk=","cp58o4toOOaHTPqTP6kODtdW+n1sRZQgTByLI0Ebulw=","bLgcjSDRamrc2IGa/25VhC/rd5DDbyRNQU1Ouo1qcEI=","eFjam254LokXnmjGPVWdJfhV0/anmKY7aK+tFxwDe2I=","ekIddkNg0jEn3M6tvmKgStf9yZFF6fQdx2B67LExsmk=","zAJTYpueGMLLi297bKBgRi4ManZRFM6wxdpOYiovIEA=","uCBIo8IEb1ec3IhtDXkPYze+XU4wsfGR3oX02G7cSTc=","QNj0S9EMq94lAWQNnIN6e5UQpsdO+LCkvMCIcTkjxG4=","FFKXBCynr1ZaryylS5G2fCjqCytCdHl8ROloo0Cw5BA=","ZrOSxL42vFlhSYZTaiZ/x4WFXnJUx01oEwwtLajzUXo=","qo0+d+Sq6B28fxpsVYznbaiZAPP3J6DwUkiWlMjWvDo=","SCLNw3FEmsu2gSxz7lkZ2LIEyric9/XT/y1EYkIWpDo=","pPG6bTM72Raa9v9GR3x4iObdmPivP8++AXKjPBcFMU4=","SMdqY9YbxOn7CfkjUOy9PbQPiYdKZhwXhw36tCj6Jwo=","4OjUFfbXtu1t+vS90b3gGLJTnVwsQWf8ZpqxCH76BUw=","XNQyUvGUKrUf5cr09dr+ZItV+fuIxWYNAzLrSC2E7nQ=","Zlg090Q8tSs76RoTRqigI/QpDzXiie7EmKeASzc0CnY=","ErCvdmKjJkMqXSmS9cyHZKIqwTSigjWYt+XgVb2+EDo="]})";  // NOLINT

  WalletInfo wallet_info;
  wallet_info.payment_id = "e7fcf220-d3f4-4111-a0b2-6157d0347567";
  wallet_info.public_key = "3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf";  // NOLINT

  // Act
  auto headers = request_->BuildHeaders(body, wallet_info);

  // Assert
  auto count = headers.size();
  EXPECT_EQ(3UL, count);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest, BuildDigestHeaderValue) {
  // Arrange
  std::string body = R"({"blindedTokens":["1Fwm6gOsXS4wmzjQjbgk69X7Yd1IGjl8nhiKGWCzXTk=","4hPPIOdzoRogm847b/AuHqapRevwByYeyLOTdnQ3Kk8=","BFx/fw+mK2N5JHwfuWoYHOvchCE4eQD8YWO3ET2ym3Q=","jGJV/w4aAMbIWNswlgySTRRFQmIaRb+1IGeyaIObPTk=","gAPctDibVJNImTIqkSdHPQOp+l3hIbINYS2ZDD/tPH4=","CIIlpfjC4fC4hzFRgzQL74BCIWs1TKdlj278nBHWCzA=","JgDYrHaIa51yQOl9uCfDmfKSAxgZbGsJ74CUe/qN7zc=","At4/Hcjzurz9+4NVq0PIGGt8NpcKmNnfhTykDTktSWg=","Xpoh1XhWWs4sI7XmgfSaVEJiU/svFW1T6XNlsRGqKHk=","ytSdCx4H2hCNlWZc59GBfhFBHRchSEuf8pzYzVIrzwM=","SpXgjYqH746GTlcufFiTQcP0IxpuO9r0LoREv/1JQww=","XlyA1GlQQakgu9di+27nMjRz8afZ6w/Ak9Xw01XQzR4=","6oICEjxG2lPgRMoGFz7NdQnPSZ16lwCnBoxMK/U8TQU=","SuzYYzPXI2+09Do/7A8b5YUTpUk4aD06FNTt33gWOnY=","LrSe7y4YjW0E8FuHBQd6xjbueUacKMGKcQyF0um2Eyc=","VLvXp7wZa81N8YhWhTpJoEidbwzfc267oE0/Ku/oC3Q=","KKd26nziS0k4xlg+/l3K7zIrhUUbRunCKlQKpcYvNFs=","NN1D1KKhPc/2gZQA9Rdm1h7OPrXgAIIvORTS0hip30g=","vscyDf3W6PGOg3I94oTmGyjeKYyB0qNAOfl4jZhHpmo=","VIIQ6gsx64nY0VgLatIpTTUs9s2axYDk5u/YIDqd/Wo=","qsUHc1BfYhcgjGU9Nb9hWJXvjuRaiH8byR6WjWXSQQQ=","uIYoo9Y7PuBH8URfZL31pR6ILaP5nH3Ut4yRe6AwjHA=","mh6huuiT7m0RbGs5k1IC+PQc8cGneKxVL/89CvQ/tzA=","tGloa/PK2MJDZ9yR6LL2U96WlH+6M7Da6eBNosTkj1M=","YjF+ljBv6I9tKoGTb/g29jFtwiqSRy+GbFmqoy4UjwE=","zFNLZf1IbCAJWTK6UEM3+GGCQ4CO89wLlKWZ9w48kWI=","4kIdTNVujRlS1Bf+a23Vzt6PITHpupb/YMQFZdV1aj0=","0HLAUuAJadMSX8Sgn3gdPjXO9s9BDT3vvpS4Zrtu9kU=","NDGBAi0R4FPWt0XgMn1g0uxCWOiOY6SZ+4XFmZx/CBg=","dtXEsDHaTHt8WK5ChXAuZ4vg79eKgNthDrACCBiwOk0=","4oY5iFVdAyLekOT1/fLeZvcJEKR9imFph8AhngF0Pgw=","+CeRUWzu6VxlxO4I1ZTLqAf3rTQd1RrAiRoywmMVPCc=","HOcA3/LprxFZc0gNT3XPNZsiFAG36cOWBwJ+002l0Qc=","lEO24pANnAFyXcMZw03rJXf5rfuManuHcfAalJQkZ0g=","sFXgELiusqD9SdHor3cZijzrlyzetafMfiGGWI6YDn0=","GJPsWMTK65dYYYnPRem7cc9MX0M6Q8XtLwboN+gEPF8=","NKZa4oTiJj/d2yCYbwCFdaQg2iZKArvEjQBylB1JOlI=","ti5l9fVKX/BDSolaMILczmfzQ75Y2AjiTII6FRTdx1Y=","Gtxl2H2r5PbOx+iFP8dzbuCRAqy9IZPwx6ypzoioDTc=","bIAmxGFA4XQntsXFd3PLHpyszB4wo59jFeWMWPMt9SE=","aJ+Uv2QxWPktxhzJH4nsyRUX88u2lajBv0cZWpfZnlo=","ACea3T+vdzi/lEgr4XmrFlFLSFFIPGPDzkZhaQrbYH4=","Egwk5NjjEaWrapV7oDI2Qsscldrc4yBIReju78MRixE=","ALV+JaHwFt44WFrPtKW2Jf3mQGxRSWXrdE60ha3J6X8=","utiko2dlzzmjjTd1QNLFg2IGs+phwHFOQom42IUiQj4=","SIIOeOuuMtN6eZFKHOYHXI6a9/QZjQoBJLJZ6PLb4XE=","5I/n0MtuDg0QkGRSzN8NalT0bksM209XCqlfwll+elA=","diM0LIWxPXa6RbBXMSIqKMWnQtXwtf1Mga1zBgNO4HA=","jnmj38cOfm4JSmZ1GUlt/BT/QzjsBnKOASMgTONd2zI=","VpWmr3pnfZE1f5KqZYwYAUDi0K04ZvibcA/p65VVDk8=","niNAlSVamLJMs9dHSmQ7Sa5z/ucwb7aDi+ovW1OIelY=","nOORDPKHI5RFJ4BsPgNCljS46GOU4JUFBFi0a7rj63s=","4v0n60wYwj3po/wsnClhnHEtpai8PGAiRMZgoS4UhWE=","mAbAnTSP3lXwToKbs33YkRrWF73ITwQy6iNTv1WFCh4=","xAn14oqfjUaViF7kr1AlH8PiE7amatXuDJy/sFzfqX4=","chniTWNWqAFCHksp4dbT9eNyRUqAxgkuuYRL4iW8WwE=","dj3/xCOmQ1lHOBYTdC+tyJFFKuCBIjsR733R7ehk4m0=","jK/O0uJqBmqaObz7S3C9JCv6fwmK6mX0jz4wNnuxH3s=","JFDbF4/bRaT1lqfwMMMkYthRn48O2ENsvJeDN9OJx2w=","atx0/2ZAlYuv0GoK9LWzl+KvlGw+vR0RF0+YQg/3gQw=","Qioe+g5INKK4n8a4jRTp756JEza9vApNB7jhclG+LXs=","9nblEhmRdSJGQpnXAtc6GtMcL4FVz8/nt4Z9lGMiPE4=","JqbX3u/cNHqvwNTR2/3Gfhi4ULwrYzWtP5wlsBZVqGY=","BFyyYHCpu9wpXQBXjDRZ5i5LEa8dY1VeJu1DScZ6bGU=","FnafJYZBSOULPWtJZVRY9tUOjpgAq+aOygioN//CuGs=","pAwG7HboPVeuCGSXA7RXw+eDj667UApMr1KSUx5f7Qg=","LpTnNc4QCvC/DBLGdfKf7r/DS7A74eWhQ9BLrnEaLk0=","bE3mseRRFqEis+hRH3JEihOQLtQ6+D9nAs+I3czNGX0=","JFTpRiNm2Tg822sdZrLNvWDRsDeVUX06HplUBx2JOA0=","6s9GicBRK2Y5FGgCYhJO3q1qkG+NTN68V8MYEAGSCnA=","aM/DJiQbIjPrTa+vqbOEyBrJYcFRCAd1VWKhk+CC9gI=","6CCWq/HwRCsY1ZVBp8r0Lf/f8zhQMYuMVRuq7qKuEGY=","EpodVxvX9X6vHaK9MyLemhzWIzZR7HzXj4V7eZuXNkQ=","AolM6TCNMNXd1Id9ColrHjpx3ECJX0SChbTrjFYR9GI=","YDaHbVval/mHopvAgnVo9svvx2nHOSf6arRRdoJYF0U=","PPNHrkMM44EZDNeD0mYKQ4zJo1elpWDMBpo+UlGTFnc=","5AykA0xLBRW6tQD/q9mWuvgHJJXkNLytS54TMlafFlw=","9G8rZDCIqPYIhsD024b32DPG4qgRG+GCvheo5pcYLRE=","TGlueCCVRBm9L8pVeDaH9SfiRfMxYtZnbaGTh8BUrjw=","VCDHZyGXUANKXPiMAUdvcwGmEgSjF3Rp0p5m7wnaD2k=","PgRiNEWFmMoZbXDB8XTUf54kHptmdd+nkqFwZXMTbw8=","zNh/pmL0nU2EgioT0vstea1z8ueWLJYF/aPHZOFHf3U=","7DNsBs0J0/iAlMQ3/atXLPubCT/10fVYGy6jtc1F8Tk=","cp58o4toOOaHTPqTP6kODtdW+n1sRZQgTByLI0Ebulw=","bLgcjSDRamrc2IGa/25VhC/rd5DDbyRNQU1Ouo1qcEI=","eFjam254LokXnmjGPVWdJfhV0/anmKY7aK+tFxwDe2I=","ekIddkNg0jEn3M6tvmKgStf9yZFF6fQdx2B67LExsmk=","zAJTYpueGMLLi297bKBgRi4ManZRFM6wxdpOYiovIEA=","uCBIo8IEb1ec3IhtDXkPYze+XU4wsfGR3oX02G7cSTc=","QNj0S9EMq94lAWQNnIN6e5UQpsdO+LCkvMCIcTkjxG4=","FFKXBCynr1ZaryylS5G2fCjqCytCdHl8ROloo0Cw5BA=","ZrOSxL42vFlhSYZTaiZ/x4WFXnJUx01oEwwtLajzUXo=","qo0+d+Sq6B28fxpsVYznbaiZAPP3J6DwUkiWlMjWvDo=","SCLNw3FEmsu2gSxz7lkZ2LIEyric9/XT/y1EYkIWpDo=","pPG6bTM72Raa9v9GR3x4iObdmPivP8++AXKjPBcFMU4=","SMdqY9YbxOn7CfkjUOy9PbQPiYdKZhwXhw36tCj6Jwo=","4OjUFfbXtu1t+vS90b3gGLJTnVwsQWf8ZpqxCH76BUw=","XNQyUvGUKrUf5cr09dr+ZItV+fuIxWYNAzLrSC2E7nQ=","Zlg090Q8tSs76RoTRqigI/QpDzXiie7EmKeASzc0CnY=","ErCvdmKjJkMqXSmS9cyHZKIqwTSigjWYt+XgVb2+EDo="]})";  // NOLINT

  // Act
  auto header_value = request_->BuildDigestHeaderValue(body);

  // Assert
  std::string expected_header_value =
      "SHA-256=aZnA/ebclJA35jQ19ouTeW0X75OmXNnoBvAnDuXJtNI=";
  EXPECT_EQ(expected_header_value, header_value);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest, BuildSignatureHeaderValue) {
  // Arrange
  std::string body = R"({"blindedTokens":["1Fwm6gOsXS4wmzjQjbgk69X7Yd1IGjl8nhiKGWCzXTk=","4hPPIOdzoRogm847b/AuHqapRevwByYeyLOTdnQ3Kk8=","BFx/fw+mK2N5JHwfuWoYHOvchCE4eQD8YWO3ET2ym3Q=","jGJV/w4aAMbIWNswlgySTRRFQmIaRb+1IGeyaIObPTk=","gAPctDibVJNImTIqkSdHPQOp+l3hIbINYS2ZDD/tPH4=","CIIlpfjC4fC4hzFRgzQL74BCIWs1TKdlj278nBHWCzA=","JgDYrHaIa51yQOl9uCfDmfKSAxgZbGsJ74CUe/qN7zc=","At4/Hcjzurz9+4NVq0PIGGt8NpcKmNnfhTykDTktSWg=","Xpoh1XhWWs4sI7XmgfSaVEJiU/svFW1T6XNlsRGqKHk=","ytSdCx4H2hCNlWZc59GBfhFBHRchSEuf8pzYzVIrzwM=","SpXgjYqH746GTlcufFiTQcP0IxpuO9r0LoREv/1JQww=","XlyA1GlQQakgu9di+27nMjRz8afZ6w/Ak9Xw01XQzR4=","6oICEjxG2lPgRMoGFz7NdQnPSZ16lwCnBoxMK/U8TQU=","SuzYYzPXI2+09Do/7A8b5YUTpUk4aD06FNTt33gWOnY=","LrSe7y4YjW0E8FuHBQd6xjbueUacKMGKcQyF0um2Eyc=","VLvXp7wZa81N8YhWhTpJoEidbwzfc267oE0/Ku/oC3Q=","KKd26nziS0k4xlg+/l3K7zIrhUUbRunCKlQKpcYvNFs=","NN1D1KKhPc/2gZQA9Rdm1h7OPrXgAIIvORTS0hip30g=","vscyDf3W6PGOg3I94oTmGyjeKYyB0qNAOfl4jZhHpmo=","VIIQ6gsx64nY0VgLatIpTTUs9s2axYDk5u/YIDqd/Wo=","qsUHc1BfYhcgjGU9Nb9hWJXvjuRaiH8byR6WjWXSQQQ=","uIYoo9Y7PuBH8URfZL31pR6ILaP5nH3Ut4yRe6AwjHA=","mh6huuiT7m0RbGs5k1IC+PQc8cGneKxVL/89CvQ/tzA=","tGloa/PK2MJDZ9yR6LL2U96WlH+6M7Da6eBNosTkj1M=","YjF+ljBv6I9tKoGTb/g29jFtwiqSRy+GbFmqoy4UjwE=","zFNLZf1IbCAJWTK6UEM3+GGCQ4CO89wLlKWZ9w48kWI=","4kIdTNVujRlS1Bf+a23Vzt6PITHpupb/YMQFZdV1aj0=","0HLAUuAJadMSX8Sgn3gdPjXO9s9BDT3vvpS4Zrtu9kU=","NDGBAi0R4FPWt0XgMn1g0uxCWOiOY6SZ+4XFmZx/CBg=","dtXEsDHaTHt8WK5ChXAuZ4vg79eKgNthDrACCBiwOk0=","4oY5iFVdAyLekOT1/fLeZvcJEKR9imFph8AhngF0Pgw=","+CeRUWzu6VxlxO4I1ZTLqAf3rTQd1RrAiRoywmMVPCc=","HOcA3/LprxFZc0gNT3XPNZsiFAG36cOWBwJ+002l0Qc=","lEO24pANnAFyXcMZw03rJXf5rfuManuHcfAalJQkZ0g=","sFXgELiusqD9SdHor3cZijzrlyzetafMfiGGWI6YDn0=","GJPsWMTK65dYYYnPRem7cc9MX0M6Q8XtLwboN+gEPF8=","NKZa4oTiJj/d2yCYbwCFdaQg2iZKArvEjQBylB1JOlI=","ti5l9fVKX/BDSolaMILczmfzQ75Y2AjiTII6FRTdx1Y=","Gtxl2H2r5PbOx+iFP8dzbuCRAqy9IZPwx6ypzoioDTc=","bIAmxGFA4XQntsXFd3PLHpyszB4wo59jFeWMWPMt9SE=","aJ+Uv2QxWPktxhzJH4nsyRUX88u2lajBv0cZWpfZnlo=","ACea3T+vdzi/lEgr4XmrFlFLSFFIPGPDzkZhaQrbYH4=","Egwk5NjjEaWrapV7oDI2Qsscldrc4yBIReju78MRixE=","ALV+JaHwFt44WFrPtKW2Jf3mQGxRSWXrdE60ha3J6X8=","utiko2dlzzmjjTd1QNLFg2IGs+phwHFOQom42IUiQj4=","SIIOeOuuMtN6eZFKHOYHXI6a9/QZjQoBJLJZ6PLb4XE=","5I/n0MtuDg0QkGRSzN8NalT0bksM209XCqlfwll+elA=","diM0LIWxPXa6RbBXMSIqKMWnQtXwtf1Mga1zBgNO4HA=","jnmj38cOfm4JSmZ1GUlt/BT/QzjsBnKOASMgTONd2zI=","VpWmr3pnfZE1f5KqZYwYAUDi0K04ZvibcA/p65VVDk8=","niNAlSVamLJMs9dHSmQ7Sa5z/ucwb7aDi+ovW1OIelY=","nOORDPKHI5RFJ4BsPgNCljS46GOU4JUFBFi0a7rj63s=","4v0n60wYwj3po/wsnClhnHEtpai8PGAiRMZgoS4UhWE=","mAbAnTSP3lXwToKbs33YkRrWF73ITwQy6iNTv1WFCh4=","xAn14oqfjUaViF7kr1AlH8PiE7amatXuDJy/sFzfqX4=","chniTWNWqAFCHksp4dbT9eNyRUqAxgkuuYRL4iW8WwE=","dj3/xCOmQ1lHOBYTdC+tyJFFKuCBIjsR733R7ehk4m0=","jK/O0uJqBmqaObz7S3C9JCv6fwmK6mX0jz4wNnuxH3s=","JFDbF4/bRaT1lqfwMMMkYthRn48O2ENsvJeDN9OJx2w=","atx0/2ZAlYuv0GoK9LWzl+KvlGw+vR0RF0+YQg/3gQw=","Qioe+g5INKK4n8a4jRTp756JEza9vApNB7jhclG+LXs=","9nblEhmRdSJGQpnXAtc6GtMcL4FVz8/nt4Z9lGMiPE4=","JqbX3u/cNHqvwNTR2/3Gfhi4ULwrYzWtP5wlsBZVqGY=","BFyyYHCpu9wpXQBXjDRZ5i5LEa8dY1VeJu1DScZ6bGU=","FnafJYZBSOULPWtJZVRY9tUOjpgAq+aOygioN//CuGs=","pAwG7HboPVeuCGSXA7RXw+eDj667UApMr1KSUx5f7Qg=","LpTnNc4QCvC/DBLGdfKf7r/DS7A74eWhQ9BLrnEaLk0=","bE3mseRRFqEis+hRH3JEihOQLtQ6+D9nAs+I3czNGX0=","JFTpRiNm2Tg822sdZrLNvWDRsDeVUX06HplUBx2JOA0=","6s9GicBRK2Y5FGgCYhJO3q1qkG+NTN68V8MYEAGSCnA=","aM/DJiQbIjPrTa+vqbOEyBrJYcFRCAd1VWKhk+CC9gI=","6CCWq/HwRCsY1ZVBp8r0Lf/f8zhQMYuMVRuq7qKuEGY=","EpodVxvX9X6vHaK9MyLemhzWIzZR7HzXj4V7eZuXNkQ=","AolM6TCNMNXd1Id9ColrHjpx3ECJX0SChbTrjFYR9GI=","YDaHbVval/mHopvAgnVo9svvx2nHOSf6arRRdoJYF0U=","PPNHrkMM44EZDNeD0mYKQ4zJo1elpWDMBpo+UlGTFnc=","5AykA0xLBRW6tQD/q9mWuvgHJJXkNLytS54TMlafFlw=","9G8rZDCIqPYIhsD024b32DPG4qgRG+GCvheo5pcYLRE=","TGlueCCVRBm9L8pVeDaH9SfiRfMxYtZnbaGTh8BUrjw=","VCDHZyGXUANKXPiMAUdvcwGmEgSjF3Rp0p5m7wnaD2k=","PgRiNEWFmMoZbXDB8XTUf54kHptmdd+nkqFwZXMTbw8=","zNh/pmL0nU2EgioT0vstea1z8ueWLJYF/aPHZOFHf3U=","7DNsBs0J0/iAlMQ3/atXLPubCT/10fVYGy6jtc1F8Tk=","cp58o4toOOaHTPqTP6kODtdW+n1sRZQgTByLI0Ebulw=","bLgcjSDRamrc2IGa/25VhC/rd5DDbyRNQU1Ouo1qcEI=","eFjam254LokXnmjGPVWdJfhV0/anmKY7aK+tFxwDe2I=","ekIddkNg0jEn3M6tvmKgStf9yZFF6fQdx2B67LExsmk=","zAJTYpueGMLLi297bKBgRi4ManZRFM6wxdpOYiovIEA=","uCBIo8IEb1ec3IhtDXkPYze+XU4wsfGR3oX02G7cSTc=","QNj0S9EMq94lAWQNnIN6e5UQpsdO+LCkvMCIcTkjxG4=","FFKXBCynr1ZaryylS5G2fCjqCytCdHl8ROloo0Cw5BA=","ZrOSxL42vFlhSYZTaiZ/x4WFXnJUx01oEwwtLajzUXo=","qo0+d+Sq6B28fxpsVYznbaiZAPP3J6DwUkiWlMjWvDo=","SCLNw3FEmsu2gSxz7lkZ2LIEyric9/XT/y1EYkIWpDo=","pPG6bTM72Raa9v9GR3x4iObdmPivP8++AXKjPBcFMU4=","SMdqY9YbxOn7CfkjUOy9PbQPiYdKZhwXhw36tCj6Jwo=","4OjUFfbXtu1t+vS90b3gGLJTnVwsQWf8ZpqxCH76BUw=","XNQyUvGUKrUf5cr09dr+ZItV+fuIxWYNAzLrSC2E7nQ=","Zlg090Q8tSs76RoTRqigI/QpDzXiie7EmKeASzc0CnY=","ErCvdmKjJkMqXSmS9cyHZKIqwTSigjWYt+XgVb2+EDo="]})";  // NOLINT

  WalletInfo wallet_info;
  wallet_info.payment_id = "e7fcf220-d3f4-4111-a0b2-6157d0347567";
  wallet_info.public_key = "3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf";  // NOLINT

  // Act
  auto header_value = request_->BuildSignatureHeaderValue(body, wallet_info);

  // Assert
  std::string expected_header_value = R"(keyId="primary",algorithm="ed25519",headers="digest",signature="p6qg0RfYtYmo4B6WG9Mcap5BxmqDRpxlGrBOnKy5G5ZBnnnn9G8yMWYGZnxcj/6tY8sesQdD9rsVTh0zXId7BA==")";  // NOLINT
  EXPECT_EQ(expected_header_value, header_value);
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest, GetAcceptHeaderValue) {
  // Arrange

  // Act
  auto accept_header_value = request_->GetAcceptHeaderValue();

  // Assert
  EXPECT_EQ(accept_header_value, "application/json");
}

TEST_F(ConfirmationsRequestSignedTokensRequestTest, GetContentType) {
  // Arrange

  // Act
  auto content_type = request_->GetContentType();

  // Assert
  EXPECT_EQ(content_type, "application/json");
}

}  // namespace confirmations
