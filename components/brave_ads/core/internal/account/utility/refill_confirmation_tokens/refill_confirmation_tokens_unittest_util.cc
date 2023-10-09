/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_unittest_util.h"

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
        "batchProof": "nZBm4sojuuKV91w9/Hcevh3r0SSmx7Cu26oeUko7hwIXYQJXjuFabmJ40nNToVm7UAkiaQvoKDFViqfpHcwxAA==",
        "signedTokens": [
          "DA+IEWxDIW91q3JItW8D3xIYMgO0TDyP/TKXFQExLEs=",
          "3KQ0o6IgVLHpKLzH01cGxA89om8CvOrW+anxorhJISk=",
          "XlzLlvOPl3XqQAYF19ly95wnAu0IA5GNk9IIUVsen0Q=",
          "VIAZP4U2QYJt/uUHLf0rfTVmMqTbytvqEGWx5eZLYDs=",
          "rk+ollPWD+y8NlqiftGmr/riBJfsg34DM/M3Fi9BFA0=",
          "xn/IDC6/wUuag5XIHR34lnnMRTi1GDVb96MYmUBUgmo=",
          "iibC1sEFbkOqGvZuQgYG1vdr6QMfLYfvFWbybmo8Myo=",
          "GAw6J637OVCz7iKN1WSg5LbLem/CMGcIz/iqmcVEvBc=",
          "Vvr5eN/Wtk5papD2urom0iOZtInck8MCBFPKKn2yuEc=",
          "1unbpoVjusErAw5cM03aBAtVUS1rKz21Z+/qro4fKjY=",
          "/Efsmh2qQNviLW47/F7wjMYnvXsS3sNvoShIxg7tsCM=",
          "VrWO44mtMyBHi3vwG9WFJcnJAueZtqCcUsnMnaH8x38=",
          "yvr5gFkC70TOQuoPdc+1UUpL8TDvKgTHNIpIBqSVcic=",
          "EsbmSuk0n/bNwwg9w09+vcABAXyJWOJHllEJam0li1I=",
          "kpyU2LZomANA/sSiAmlhITmaSnUCF4k3ZbL8tC8bCBs=",
          "snY5KcxsrH4YtPtuATwrqFEsI8s41CwFwzD9NQPRYDM=",
          "Os0UZsMkLd2zjSvtlZXNONoC9lW88ub/am/m6M5JyA0=",
          "yhgSTuGXe/7rEOn71eJFsIs2yenE4yAqqQLTxZvj9AE=",
          "hswDh0BfDSFVC5IgVRo48u4DmNoVGXBguN0MvOcxFwQ=",
          "WKkneVbAf05kwIWiH8wN3jhNjwyJLYi9zH+wkNBo5nw=",
          "HH26LkAhxJxIFthoLyKD/OA67AzmyPW59ady3nYcnj4=",
          "5EPMh4OW09U3M4d8oPYv4OPtOZPiN0+cLTWIRc486lw=",
          "lhYrEba/qFw6hKUOhBJO6KVRUj4yQwXNhkhdNWvAFjg=",
          "XKkrY+E8P2c3vBLa2967Q1uP3o0bsz6ElDbetbiPuUE=",
          "/GgluIwAIj2zfuiY4YpfF+cLTByPNM37DBlld746bxY=",
          "dMwnd60OVo3jF/DkkZIO9Y68vvGoL0yfO9qlLsdTfwo=",
          "op4WXEs052uAvEvqvGWkr7PATcigxwwKExuIqMUoVQ4=",
          "VJrlz3pP+VNeBivSYUYOLqKEP2x9vr75AUkBcazpMhg=",
          "UJuKnh5+jVwrRctKm29v4FBLoLqy7Hq4ddr7Ci9o6X0=",
          "pEQVS63QXhWHHOIClAxKJFevczY2YcTnS5SnzxX/AAE=",
          "bjAvdetnrggnuQ4nUAG+SCUpMRhsnstXW/m9kIVwUys=",
          "tNcuHnWkzovoG/CQ/Uz8LwqojM9se+WxfdArBrMEjTE=",
          "lDT++JAPo/S5R4S9R9c/WBnrNq/qznsb121cybxDcFI=",
          "yIRJoQT0DErGmFNLt2DraTRqbiScbAh07TsAqHLAzlk=",
          "Vtt/1xciT3GzPVEqWS321kcD37JPW/gfA0i5A4jHSFA=",
          "WF1TDYuaZaah9xuS0PRonsMtwS/YYATyzrsB525OpEw=",
          "yq2LhUEeBFD2eaKu7Wmjyb1W2OyB482PwPKj7HjUaWg=",
          "JNbzAbJS/LWdUdWbd/epPHy935mpeo+1yBZrPQuH4k0=",
          "PPucf/FQzL7mU6Ec8gUtOiO+1V1sabDX76rqv/XjkFk=",
          "dH/pwaRwwyg9kSDUmGyfRSV94slSYXIbrl8ZhGkGiBQ=",
          "dj45NGx3tM2/t5hfu7shUxPKd1XmKpucopYBooxR/SE=",
          "emn0UWvwqfSZeR8uBqjT629UeP+7PXyfg4WXYIIk9BM=",
          "Gpiwdsv6YlBqgcWCA1oenPaX8VkvT2eXxvGC60SUC08=",
          "0m4tWlAVWh7OA1scMdbbXRLGM2ui72DbiSh3tgZ56Xk=",
          "uCYqm0IjumhZH7yBzZl+W2FbHwq3Qc0tV0CQTnJSV04=",
          "8OJEKXDcCoDEnzggFHaQpemkfPmHJ5u+isQzE8wnXkE=",
          "hHJHWTpnWUexvq8J218KqBqmblpimQR021/GbPFKT1Q=",
          "+MZDcE6B+WykiTN4ArTMQob21auW8/6PVAc4O62fq2s=",
          "MNm3EDUMslVioVqCDb5PkW8op+cHFyw8qo3BKxVZLF4=",
          "2LEoAWcTHMMFFV+/BYOGjPhdAtNL8asRwdliyaGQnRA="
        ],
        "publicKey": "bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg="
      })";
}

}  // namespace brave_ads::test
