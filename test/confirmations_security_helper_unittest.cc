/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "confirmations_client_mock.h"
#include "bat-native-confirmations/src/confirmations_impl.h"
#include "bat-native-confirmations/src/security_helper.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

namespace confirmations {

class ConfirmationsSecurityHelperTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  ConfirmationsSecurityHelperTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsSecurityHelperTest() override {
    // You can do clean-up work that doesn't throw exceptions here
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
};

TEST_F(ConfirmationsSecurityHelperTest, Sign) {
  // Arrange
  std::map<std::string, std::string> headers = {
    {"digest", "SHA-256=aZnA/ebclJA35jQ19ouTeW0X75OmXNnoBvAnDuXJtNI="}
  };

  std::string key_id = "primary";

  std::vector<uint8_t> public_key = {
      0x3f, 0xc8, 0xff, 0x3b, 0x12, 0x1e, 0x7b, 0x78, 0x75, 0x75, 0x0d, 0x26,
      0xea, 0xba, 0x6f, 0x06, 0xa3, 0xb0, 0x6d, 0x96, 0xcf, 0x6b, 0x2f, 0xb8,
      0x98, 0x32, 0x39, 0x17, 0xe7, 0xbe, 0x9d, 0x16, 0xe2, 0x55, 0xa4, 0xa6,
      0xf7, 0xeb, 0x86, 0x47, 0x42, 0x8f, 0x72, 0x7c, 0x0d, 0x4e, 0x19, 0x58,
      0xbd, 0x8e, 0x69, 0xa9, 0x84, 0xee, 0xe3, 0x85, 0x14, 0xd1, 0xe4, 0x83,
      0xaa, 0xb2, 0x7e, 0xdf
  };

  // Act
  auto signature = helper::Security::Sign(headers, key_id, public_key);

  // Assert
  std::string expected_signature = R"(keyId="primary",algorithm="ed25519",headers="digest",signature="p6qg0RfYtYmo4B6WG9Mcap5BxmqDRpxlGrBOnKy5G5ZBnnnn9G8yMWYGZnxcj/6tY8sesQdD9rsVTh0zXId7BA==")";  // NOLINT
  EXPECT_EQ(expected_signature, signature);
}

TEST_F(ConfirmationsSecurityHelperTest, Sign_InvalidPublicKey) {
  // Arrange
  std::map<std::string, std::string> headers = {
    {"digest", "SHA-256=aZnA/ebclJA35jQ19ouTeW0X75OmXNnoBvAnDuXJtNI="}
  };

  std::string key_id = "primary";

  std::vector<uint8_t> public_key = {
      0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
      0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
      0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
      0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
      0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
      0xde, 0xad, 0xbe, 0xef
  };

  // Act
  auto signature = helper::Security::Sign(headers, key_id, public_key);

  // Assert
  std::string expected_signature = "p6qg0RfYtYmo4B6WG9Mcap5BxmqDRpxlGrBOnKy5G5ZBnnnn9G8yMWYGZnxcj/6tY8sesQdD9rsVTh0zXId7BA==";  // NOLINT
  EXPECT_NE(expected_signature, signature);
}

TEST_F(ConfirmationsSecurityHelperTest, GenerateTokens) {
  // Arrange

  // Act
  auto tokens = helper::Security::GenerateTokens(5);

  // Assert
  auto count = tokens.size();
  EXPECT_EQ(5UL, count);
}

TEST_F(ConfirmationsSecurityHelperTest, BlindTokens) {
  // Arrange
  auto tokens = helper::Security::GenerateTokens(7);

  // Act
  auto blinded_tokens = helper::Security::BlindTokens(tokens);

  // Assert
  EXPECT_EQ(tokens.size(), blinded_tokens.size());
}

TEST_F(ConfirmationsSecurityHelperTest, GetSHA256) {
  // Arrange
  std::string body = R"({"blindedTokens":["1Fwm6gOsXS4wmzjQjbgk69X7Yd1IGjl8nhiKGWCzXTk=","4hPPIOdzoRogm847b/AuHqapRevwByYeyLOTdnQ3Kk8=","BFx/fw+mK2N5JHwfuWoYHOvchCE4eQD8YWO3ET2ym3Q=","jGJV/w4aAMbIWNswlgySTRRFQmIaRb+1IGeyaIObPTk=","gAPctDibVJNImTIqkSdHPQOp+l3hIbINYS2ZDD/tPH4=","CIIlpfjC4fC4hzFRgzQL74BCIWs1TKdlj278nBHWCzA=","JgDYrHaIa51yQOl9uCfDmfKSAxgZbGsJ74CUe/qN7zc=","At4/Hcjzurz9+4NVq0PIGGt8NpcKmNnfhTykDTktSWg=","Xpoh1XhWWs4sI7XmgfSaVEJiU/svFW1T6XNlsRGqKHk=","ytSdCx4H2hCNlWZc59GBfhFBHRchSEuf8pzYzVIrzwM=","SpXgjYqH746GTlcufFiTQcP0IxpuO9r0LoREv/1JQww=","XlyA1GlQQakgu9di+27nMjRz8afZ6w/Ak9Xw01XQzR4=","6oICEjxG2lPgRMoGFz7NdQnPSZ16lwCnBoxMK/U8TQU=","SuzYYzPXI2+09Do/7A8b5YUTpUk4aD06FNTt33gWOnY=","LrSe7y4YjW0E8FuHBQd6xjbueUacKMGKcQyF0um2Eyc=","VLvXp7wZa81N8YhWhTpJoEidbwzfc267oE0/Ku/oC3Q=","KKd26nziS0k4xlg+/l3K7zIrhUUbRunCKlQKpcYvNFs=","NN1D1KKhPc/2gZQA9Rdm1h7OPrXgAIIvORTS0hip30g=","vscyDf3W6PGOg3I94oTmGyjeKYyB0qNAOfl4jZhHpmo=","VIIQ6gsx64nY0VgLatIpTTUs9s2axYDk5u/YIDqd/Wo=","qsUHc1BfYhcgjGU9Nb9hWJXvjuRaiH8byR6WjWXSQQQ=","uIYoo9Y7PuBH8URfZL31pR6ILaP5nH3Ut4yRe6AwjHA=","mh6huuiT7m0RbGs5k1IC+PQc8cGneKxVL/89CvQ/tzA=","tGloa/PK2MJDZ9yR6LL2U96WlH+6M7Da6eBNosTkj1M=","YjF+ljBv6I9tKoGTb/g29jFtwiqSRy+GbFmqoy4UjwE=","zFNLZf1IbCAJWTK6UEM3+GGCQ4CO89wLlKWZ9w48kWI=","4kIdTNVujRlS1Bf+a23Vzt6PITHpupb/YMQFZdV1aj0=","0HLAUuAJadMSX8Sgn3gdPjXO9s9BDT3vvpS4Zrtu9kU=","NDGBAi0R4FPWt0XgMn1g0uxCWOiOY6SZ+4XFmZx/CBg=","dtXEsDHaTHt8WK5ChXAuZ4vg79eKgNthDrACCBiwOk0=","4oY5iFVdAyLekOT1/fLeZvcJEKR9imFph8AhngF0Pgw=","+CeRUWzu6VxlxO4I1ZTLqAf3rTQd1RrAiRoywmMVPCc=","HOcA3/LprxFZc0gNT3XPNZsiFAG36cOWBwJ+002l0Qc=","lEO24pANnAFyXcMZw03rJXf5rfuManuHcfAalJQkZ0g=","sFXgELiusqD9SdHor3cZijzrlyzetafMfiGGWI6YDn0=","GJPsWMTK65dYYYnPRem7cc9MX0M6Q8XtLwboN+gEPF8=","NKZa4oTiJj/d2yCYbwCFdaQg2iZKArvEjQBylB1JOlI=","ti5l9fVKX/BDSolaMILczmfzQ75Y2AjiTII6FRTdx1Y=","Gtxl2H2r5PbOx+iFP8dzbuCRAqy9IZPwx6ypzoioDTc=","bIAmxGFA4XQntsXFd3PLHpyszB4wo59jFeWMWPMt9SE=","aJ+Uv2QxWPktxhzJH4nsyRUX88u2lajBv0cZWpfZnlo=","ACea3T+vdzi/lEgr4XmrFlFLSFFIPGPDzkZhaQrbYH4=","Egwk5NjjEaWrapV7oDI2Qsscldrc4yBIReju78MRixE=","ALV+JaHwFt44WFrPtKW2Jf3mQGxRSWXrdE60ha3J6X8=","utiko2dlzzmjjTd1QNLFg2IGs+phwHFOQom42IUiQj4=","SIIOeOuuMtN6eZFKHOYHXI6a9/QZjQoBJLJZ6PLb4XE=","5I/n0MtuDg0QkGRSzN8NalT0bksM209XCqlfwll+elA=","diM0LIWxPXa6RbBXMSIqKMWnQtXwtf1Mga1zBgNO4HA=","jnmj38cOfm4JSmZ1GUlt/BT/QzjsBnKOASMgTONd2zI=","VpWmr3pnfZE1f5KqZYwYAUDi0K04ZvibcA/p65VVDk8=","niNAlSVamLJMs9dHSmQ7Sa5z/ucwb7aDi+ovW1OIelY=","nOORDPKHI5RFJ4BsPgNCljS46GOU4JUFBFi0a7rj63s=","4v0n60wYwj3po/wsnClhnHEtpai8PGAiRMZgoS4UhWE=","mAbAnTSP3lXwToKbs33YkRrWF73ITwQy6iNTv1WFCh4=","xAn14oqfjUaViF7kr1AlH8PiE7amatXuDJy/sFzfqX4=","chniTWNWqAFCHksp4dbT9eNyRUqAxgkuuYRL4iW8WwE=","dj3/xCOmQ1lHOBYTdC+tyJFFKuCBIjsR733R7ehk4m0=","jK/O0uJqBmqaObz7S3C9JCv6fwmK6mX0jz4wNnuxH3s=","JFDbF4/bRaT1lqfwMMMkYthRn48O2ENsvJeDN9OJx2w=","atx0/2ZAlYuv0GoK9LWzl+KvlGw+vR0RF0+YQg/3gQw=","Qioe+g5INKK4n8a4jRTp756JEza9vApNB7jhclG+LXs=","9nblEhmRdSJGQpnXAtc6GtMcL4FVz8/nt4Z9lGMiPE4=","JqbX3u/cNHqvwNTR2/3Gfhi4ULwrYzWtP5wlsBZVqGY=","BFyyYHCpu9wpXQBXjDRZ5i5LEa8dY1VeJu1DScZ6bGU=","FnafJYZBSOULPWtJZVRY9tUOjpgAq+aOygioN//CuGs=","pAwG7HboPVeuCGSXA7RXw+eDj667UApMr1KSUx5f7Qg=","LpTnNc4QCvC/DBLGdfKf7r/DS7A74eWhQ9BLrnEaLk0=","bE3mseRRFqEis+hRH3JEihOQLtQ6+D9nAs+I3czNGX0=","JFTpRiNm2Tg822sdZrLNvWDRsDeVUX06HplUBx2JOA0=","6s9GicBRK2Y5FGgCYhJO3q1qkG+NTN68V8MYEAGSCnA=","aM/DJiQbIjPrTa+vqbOEyBrJYcFRCAd1VWKhk+CC9gI=","6CCWq/HwRCsY1ZVBp8r0Lf/f8zhQMYuMVRuq7qKuEGY=","EpodVxvX9X6vHaK9MyLemhzWIzZR7HzXj4V7eZuXNkQ=","AolM6TCNMNXd1Id9ColrHjpx3ECJX0SChbTrjFYR9GI=","YDaHbVval/mHopvAgnVo9svvx2nHOSf6arRRdoJYF0U=","PPNHrkMM44EZDNeD0mYKQ4zJo1elpWDMBpo+UlGTFnc=","5AykA0xLBRW6tQD/q9mWuvgHJJXkNLytS54TMlafFlw=","9G8rZDCIqPYIhsD024b32DPG4qgRG+GCvheo5pcYLRE=","TGlueCCVRBm9L8pVeDaH9SfiRfMxYtZnbaGTh8BUrjw=","VCDHZyGXUANKXPiMAUdvcwGmEgSjF3Rp0p5m7wnaD2k=","PgRiNEWFmMoZbXDB8XTUf54kHptmdd+nkqFwZXMTbw8=","zNh/pmL0nU2EgioT0vstea1z8ueWLJYF/aPHZOFHf3U=","7DNsBs0J0/iAlMQ3/atXLPubCT/10fVYGy6jtc1F8Tk=","cp58o4toOOaHTPqTP6kODtdW+n1sRZQgTByLI0Ebulw=","bLgcjSDRamrc2IGa/25VhC/rd5DDbyRNQU1Ouo1qcEI=","eFjam254LokXnmjGPVWdJfhV0/anmKY7aK+tFxwDe2I=","ekIddkNg0jEn3M6tvmKgStf9yZFF6fQdx2B67LExsmk=","zAJTYpueGMLLi297bKBgRi4ManZRFM6wxdpOYiovIEA=","uCBIo8IEb1ec3IhtDXkPYze+XU4wsfGR3oX02G7cSTc=","QNj0S9EMq94lAWQNnIN6e5UQpsdO+LCkvMCIcTkjxG4=","FFKXBCynr1ZaryylS5G2fCjqCytCdHl8ROloo0Cw5BA=","ZrOSxL42vFlhSYZTaiZ/x4WFXnJUx01oEwwtLajzUXo=","qo0+d+Sq6B28fxpsVYznbaiZAPP3J6DwUkiWlMjWvDo=","SCLNw3FEmsu2gSxz7lkZ2LIEyric9/XT/y1EYkIWpDo=","pPG6bTM72Raa9v9GR3x4iObdmPivP8++AXKjPBcFMU4=","SMdqY9YbxOn7CfkjUOy9PbQPiYdKZhwXhw36tCj6Jwo=","4OjUFfbXtu1t+vS90b3gGLJTnVwsQWf8ZpqxCH76BUw=","XNQyUvGUKrUf5cr09dr+ZItV+fuIxWYNAzLrSC2E7nQ=","Zlg090Q8tSs76RoTRqigI/QpDzXiie7EmKeASzc0CnY=","ErCvdmKjJkMqXSmS9cyHZKIqwTSigjWYt+XgVb2+EDo="]})";  // NOLINT

  // Act
  auto sha256 = helper::Security::GetSHA256(body);
  auto sha256_base64 = helper::Security::GetBase64(sha256);

  // Assert
  std::string expected_sha256_base64 =
      "aZnA/ebclJA35jQ19ouTeW0X75OmXNnoBvAnDuXJtNI=";
  EXPECT_EQ(expected_sha256_base64, sha256_base64);
}

}  // namespace confirmations
