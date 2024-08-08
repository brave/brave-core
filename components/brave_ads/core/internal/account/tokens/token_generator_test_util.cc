/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"

#include <string>

#include "base/check_op.h"
#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads::test {

// Signing Key:
//   p9d55MfQAPcFDoc57gratS4ZwP7eqSFR9EzQ0LjdrwU=

// Public Key:
//   bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=

// Blinded Tokens:
//   Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q=
//   shDzMRNpQKrQAfRctVm4l0Ulaoek0spX8iabH1+Vx00=
//   kMI3fgomSSNcT1N8d3b+AlZXybqA3st3Ks6XhwaSRF4=
//   HNC7QMQWzzf+Oh95u0dCanUC9jfE+Um6dSqfV/etGmA=
//   Bip78zU1Onnv4SAWNf983PKOc+4Z674cjpWFCkOQWl8=
//   JOzzftwNy6L/DZks/h1IxGrujCwGlmmtf3XI1Rskojc=
//   KN9qaHJoFsG4WlS0wqrrFvtH9lK+0dJAP1rxHVWA3mY=
//   TrMU/qRrpMUXlMHd2Lv3qctyrJylw2Dww573dKjnmDQ=
//   KrrcgMH/8dNp4SMQV6UO4HAJJ1UkHvyn5nriwXB9DQU=
//   6vJAxVLmyX6f15roPvlfVunFRxgzdMNUVaW2URUVyl8=
//   bn9Flqbtx1+cMe+1zxg2YBwir5SG5iBGcHHsy1ov9io=
//   ZJgRVfnllv0cZsIY6vCIBDBfj0MH79O9iIAjE2tgcx8=
//   8A/l2Jyb/PbAbqottQBvMH3PjXpo/BsNLcrO9XA5Zh0=
//   vo1vTg5P4aZrCqa8XQlD9HDd7I9CwpoLTM+OenyQAxs=
//   Cgzf7OBQ0MFgCUAHDIkbRA9VWWRftWY5ryR71YDMmyM=
//   rPT6t6nB7gyIkMIRPWmrFKRsUiEQXMAJv1BwVo5h/wM=
//   UA0ww/Rp0c/wjnuxwNClApgKJitVv/8AzfSXUzDwPB8=
//   aCrGBtAfRYIdVSiZC/b+CxDpHpmuZZsPDZ4+wCTtgxs=
//   jFcgvwkDg73uEm0FIkXynfXKRIqmiCYocNrXR99Lczs=
//   zKid4BTQH4L0vQAB5eSaQZpDV2++vN/TzGo+2xNYMBw=
//   FO0eQe97EurA0gfZMmPPQOtaHL36Oe0ppsEcou/DPGk=
//   VBgsBQQuWz4rGRt0Np/C9DQCs1W6P5IDotymec5rHSc=
//   0g4u7LXsz3xYu6AUvl4Nyc0noKA4oEgUmaoR/Eq6SUI=
//   fsSAPvpcItEYlyP8cTPvtHJXetnyNrzFpH06IDl/T1Y=
//   IryYkPpzqEWOJ2/L4y9Aq1RivBrhhqqj5WXg31sVYQs=
//   eFwVmfeqjjGNRG94C3EGFvs6jJdiPI2JQoaylrf9Slk=
//   dFz8S5yoZtpfIjQ8/VOAIJ1E2sNe5tWs81DVjHdOSlU=
//   rjvZluoYh9qznp+s0aLfY5DN7fzRajqX2zBhzE/F/A4=
//   KH+KBkDLSTmJ7oWoDZoM2uYwLagb1vp1Nv6RVzCnzVw=
//   Ck0B8iyefPM3iCS37Wrwto88aGMaZnI70LJ/t4V0Nmg=
//   UEUQyHa1SL9nbVHc/s5dAri0ffKc05N3UaTziBXUz0s=
//   OEq8huSJaHCyQ0GD1ndGN3HmbxqLqFrxRYu4rRcoU0o=
//   eo47t0y9YnhUq9E6USpSrRsKVDnKwE42Oj+2BuzM2Hs=
//   5j3feCuFYFbtbQDWrVqjavZgNVJRfe5Ye56qZ/cpKWs=
//   pHAeQZeGjmevJ3ANuSyKyva8BqZ7Qb5Nkk1QBJrAKBU=
//   mMKgzasY/a8WatViT0jBIAnYBVani9KNw4cDgZSyAyU=
//   KuCv8htyq/3uBNQXvsNRRiIzJrwOsd/IlUngMa8wUQM=
//   /CwA7TJsUMxuS9xvQs4fB2MJGwY9wGYbaCOswX6XNVk=
//   HjM5bebKsjgDfFxRN9+reqYHSHK1iIh2+vjUXhJh3xw=
//   ctSqv2QNEiGOcySJ9Uq3uIbTXoFGQIVMZJA/jZT7eBA=
//   fptOBB4M0fhYoT+qnclj7JXZwpQiLxMnZTosDfR1qxg=
//   OIGR9QZOgeZceOrq0ZxM8hj1qTp78GjlSKzw2JgMZiw=
//   Jvnj0RDBUgBsUmRkqL9BxlcjGg4g/aFaX039uaMzsVY=
//   HNXIIRwYTc1ERgBVBrb4ZYN9lKn4wSqaaaysrQi+rUs=
//   CvXEEsP6ltCSeQ2a5hmPA2SxBHPdEk6MLz9NW+qDKAA=
//   4rN6kq7bP46W5C5jCBoFXhaSyAqt+G1YQEgvVwlUsCU=
//   WMLVtZYQ50Z3UwVRbYVDCX/RaKgUyvhAdOOlYZAopzI=
//   2qOMS1Utn2dWYK7+qxsycgPvPhUgBN4mH+BvyBDM9kw=
//   YhojWF9EONUBPW6+VuvrsMt0MvVrX8MNK5OMb3f74lQ=
//   8s+alELEHHB0MugTqobAd1o/oM4148iQmsicOuImWx8=

// Signed Tokens:
//   DA+IEWxDIW91q3JItW8D3xIYMgO0TDyP/TKXFQExLEs=
//   3KQ0o6IgVLHpKLzH01cGxA89om8CvOrW+anxorhJISk=
//   XlzLlvOPl3XqQAYF19ly95wnAu0IA5GNk9IIUVsen0Q=
//   VIAZP4U2QYJt/uUHLf0rfTVmMqTbytvqEGWx5eZLYDs=
//   rk+ollPWD+y8NlqiftGmr/riBJfsg34DM/M3Fi9BFA0=
//   xn/IDC6/wUuag5XIHR34lnnMRTi1GDVb96MYmUBUgmo=
//   iibC1sEFbkOqGvZuQgYG1vdr6QMfLYfvFWbybmo8Myo=
//   GAw6J637OVCz7iKN1WSg5LbLem/CMGcIz/iqmcVEvBc=
//   Vvr5eN/Wtk5papD2urom0iOZtInck8MCBFPKKn2yuEc=
//   1unbpoVjusErAw5cM03aBAtVUS1rKz21Z+/qro4fKjY=
//   /Efsmh2qQNviLW47/F7wjMYnvXsS3sNvoShIxg7tsCM=
//   VrWO44mtMyBHi3vwG9WFJcnJAueZtqCcUsnMnaH8x38=
//   yvr5gFkC70TOQuoPdc+1UUpL8TDvKgTHNIpIBqSVcic=
//   EsbmSuk0n/bNwwg9w09+vcABAXyJWOJHllEJam0li1I=
//   kpyU2LZomANA/sSiAmlhITmaSnUCF4k3ZbL8tC8bCBs=
//   snY5KcxsrH4YtPtuATwrqFEsI8s41CwFwzD9NQPRYDM=
//   Os0UZsMkLd2zjSvtlZXNONoC9lW88ub/am/m6M5JyA0=
//   yhgSTuGXe/7rEOn71eJFsIs2yenE4yAqqQLTxZvj9AE=
//   hswDh0BfDSFVC5IgVRo48u4DmNoVGXBguN0MvOcxFwQ=
//   WKkneVbAf05kwIWiH8wN3jhNjwyJLYi9zH+wkNBo5nw=
//   HH26LkAhxJxIFthoLyKD/OA67AzmyPW59ady3nYcnj4=
//   5EPMh4OW09U3M4d8oPYv4OPtOZPiN0+cLTWIRc486lw=
//   lhYrEba/qFw6hKUOhBJO6KVRUj4yQwXNhkhdNWvAFjg=
//   XKkrY+E8P2c3vBLa2967Q1uP3o0bsz6ElDbetbiPuUE=
//   /GgluIwAIj2zfuiY4YpfF+cLTByPNM37DBlld746bxY=
//   dMwnd60OVo3jF/DkkZIO9Y68vvGoL0yfO9qlLsdTfwo=
//   op4WXEs052uAvEvqvGWkr7PATcigxwwKExuIqMUoVQ4=
//   VJrlz3pP+VNeBivSYUYOLqKEP2x9vr75AUkBcazpMhg=
//   UJuKnh5+jVwrRctKm29v4FBLoLqy7Hq4ddr7Ci9o6X0=
//   pEQVS63QXhWHHOIClAxKJFevczY2YcTnS5SnzxX/AAE=
//   bjAvdetnrggnuQ4nUAG+SCUpMRhsnstXW/m9kIVwUys=
//   tNcuHnWkzovoG/CQ/Uz8LwqojM9se+WxfdArBrMEjTE=
//   lDT++JAPo/S5R4S9R9c/WBnrNq/qznsb121cybxDcFI=
//   yIRJoQT0DErGmFNLt2DraTRqbiScbAh07TsAqHLAzlk=
//   Vtt/1xciT3GzPVEqWS321kcD37JPW/gfA0i5A4jHSFA=
//   WF1TDYuaZaah9xuS0PRonsMtwS/YYATyzrsB525OpEw=
//   yq2LhUEeBFD2eaKu7Wmjyb1W2OyB482PwPKj7HjUaWg=
//   JNbzAbJS/LWdUdWbd/epPHy935mpeo+1yBZrPQuH4k0=
//   PPucf/FQzL7mU6Ec8gUtOiO+1V1sabDX76rqv/XjkFk=
//   dH/pwaRwwyg9kSDUmGyfRSV94slSYXIbrl8ZhGkGiBQ=
//   dj45NGx3tM2/t5hfu7shUxPKd1XmKpucopYBooxR/SE=
//   emn0UWvwqfSZeR8uBqjT629UeP+7PXyfg4WXYIIk9BM=
//   Gpiwdsv6YlBqgcWCA1oenPaX8VkvT2eXxvGC60SUC08=
//   0m4tWlAVWh7OA1scMdbbXRLGM2ui72DbiSh3tgZ56Xk=
//   uCYqm0IjumhZH7yBzZl+W2FbHwq3Qc0tV0CQTnJSV04=
//   8OJEKXDcCoDEnzggFHaQpemkfPmHJ5u+isQzE8wnXkE=
//   hHJHWTpnWUexvq8J218KqBqmblpimQR021/GbPFKT1Q=
//   +MZDcE6B+WykiTN4ArTMQob21auW8/6PVAc4O62fq2s=
//   MNm3EDUMslVioVqCDb5PkW8op+cHFyw8qo3BKxVZLF4=
//   2LEoAWcTHMMFFV+/BYOGjPhdAtNL8asRwdliyaGQnRA=

// Batch DLEQ Proof:
//   gCF+6wrxT9bRFX+yjbdwPeWNZ8/wuDg6uZPy0Ea0yA2cfq5yEMpMfYMoH4BHW6EBbWR5QaOo1mVg+O9syYD3CA==

namespace {

const std::vector<std::string>& Tokens() {
  static base::NoDestructor<std::vector<std::string>>
      tokens({R"(aXZNwft34oG2JAVBnpYh/ktTOzr2gi0lKosYNczUUz6ZS9gaDTJmU2FHFps9dIq+QoDwjSjctR5v0rRn+dYo+AHScVqFAgJ5t2s4KtSyawW10gk6hfWPQw16Q0+8u5AG)", R"(SIXXr9oqmgiM3kFc/dfowJm+XZFkJRzIK4K8rpummrbYqTb/LOD73S037olwc0O/amQG7wEvs7NI3L+WUI0Qcgi+E6lT+smuWPXgQEvCZUUK6KYbDjCSvu0ZaO340+8I)", R"(xaU6n7xHrHu2SmO0KMyxgSlizSuzfycBgEx60aWDGWr7bXFERMCbpuMZlDj5f0NZ5afzCVR/IkytDpT62JQ6krKWX2wbmle/+Z/bh+DyrVEAVDWictj+YgXANF1ZnBIJ)", R"(AE+DFMLLmoj2r6VqC0HiFB8UQW5bfeqSw3DEBAEZX4D+AnUuIUgUNMxsyi7ZHcGmN8OrmwN1qbZQ6lyIvhSy63nv5tckQchU9XFWulcoVeBM8L7VRK69gHpzmWnq7scH)", R"(mO1Z9CIXTuvoGI/XNAb0EWLZf+2J+ALkrogJw7Y0FIODh2L6SImbRCLE+fasKyCiJwdzlRfylDv8ePPDtm4RWZ01SWP7m2ESfbkwPWeh9Gr0Xulg55dwXAKzhUB7vCcB)", R"(CiMm8C+ZNgmOuEi/nwrv0oR6B84l0iJtGeI0FRepz+mCxIx/wE0maXIwAJpjapo4zKjSB9wFhgdVjUzy/wv73o48wz/bBpPC1A+1d7dZeE2mLnYNXqZS+6DwOvSQqUcL)", R"(jEwqECOC+p9wKz21qYH4YFoY1GfzBO8NFiTq46RFLGs7tv5x4cM/ll2Wcr1SltGGvQ8GC4fonm/6V+O4I3AEBy2DGsjmnVUF0OuDeoq6qE48stC0hVipOZoAL+clxH0D)", R"(t48lP8F37p/dxTjkn9UmBRMiaWLE6mmcWn1LIwKTUz5iTB/gBmrTvCiSlxs8ggBV2oc0C/GxprAFLK3zPNKZVG+YdQZIhc+0Px1+Z/9SUupHqOhW3UnktAzblYmzcdEE)", R"(I2c7jiD41RS87xHPf1DXbHJ17PQ+8Tjo262y+ZpFadcJc5Mb/lVACVuJtmvQL5zZKHXePyU8ja/va4IIhDsDq+nKOBTUhfWllcycB++vZMYpAQ9tu7NApxB3rEwIZcoM)", R"(4tpgGG5F2vfzKgwa1zVzleow1ZuVbs6MbHgwnrqIRiC3dnbhDgqpdFQp/TDCxHbWSSKXcKClduu0VzMRdSkc/K26AZ1gPSYua3nHW2QQC016StGGp6bc2erCPiNLFzMP)", R"(LMf4p1jgLl1iSHAXg0E0jlBNQuvfjb7ijhSMnoVGKejnQIpyOJWDyxGJ9aoX0kNhu0SL+vJPtshkzHbp/+rK/p7W+1LY6oEUBRTB94AGZCzjneLxlc5skd7h9w9HHc8N)", R"(mgoPfFvPF/7kFUyYFfLbowW0NteiQnwphdK1kDNMFcEZD7ynXXGXEKDeNiYqBHItJc7BJvx1cCH+9CJfNn2SGbWvhJ8Y+snREgDwgEo7MGkTUFEm9l6S4psiVYpO20UL)", R"(KKiLhNB3O5oPGOztJUdAvq+bPJamKvcT74uyhmpg8qC46OwBsg7R0lxxistpqIkvpxFkTpCQvVtxAmzwLRxAx+SESEjdU+ehw6uXDlPTiD3GkWd7O1bGfzsNQKPRACgA)", R"(V+5MAR7zP6NpiDJqcpL6JCPxCsTIzFfsFwoZoJ3EYpTrfikcR9pJNtajnhIjbxrn99acocksuJDnvwptyqfyY7Zay07dZ+V3SZ+mlwnJMGAp19y7rObC2pN8E/2pC1UP)", R"(H5R6i2tirFKculydzNe2o06diEVrbvgUrXNtvsUBUgn3KF7Byq040yTCQIIlr+8IBbv8APl4AxDw8MILKylN6aUcbTnplp/pXKrQOb2ZTKt8WJTFL85Wv6Hhfr02+TQC)", R"(jSuz7mY/Q9oE9xSV7dy6XNa4g2DGVr7ZjZlyXg5paO3CFm1e9WvFlWbL3G3/N2ajNtRo5l37QSMXUOyRW0AY3IFFL44hZ/9Iy4Xct6lXHae5JLTWwr9JBe7ZberWK6AI)", R"(IrE4XOLAEwzpM9pmay3P7F9QsjciZIx1KfYpSUP2h8xeEMOwA7Tipqk8Xm4NzA6OdioAKlxf47PlbxeaEoVnoSkGL2fJn6RIcb0GxUg4t4KAA3D8rf0Wg5YW8YeGp+QJ)", R"(zO1nsI/DvPeMrPC9aPpHJsBWioVkD0RDSoVaa4yhCPXGrQN1bdKhpLZGriJyH7mQbtqm0MGKVelRC0alTSjsOsYdpz/BY64GutmZBITb9vaoSAparGP7ROqfVYJ5jZYL)", R"(9H/18LRQXRpARudmkiLe7Wvaq4twWq1MROc3Wt0PxT8kq85tLnvz7RXe7NRtKdnCyyvPHTs1hffixXuBJt8IIxE1OKYoqsiplMDGZZF2JqmP+m+EPhA0JnRSIJv3wy4B)", R"(j+D7QBcWiF5MmPOBpnCgLpgK9CbtIsKiGLoTqIHcqFs3Dki2dVfoj7tiEZEb81l5NLts5E+cNMU58S7/3BSRt0L92qx5oFreA1Z47UKqApPft/mDwzpkz4h4Escve2AI)", R"(BQ0u0OSkYiUbl3tiIOs4QHU/Odi2Cqt2oWZfCVAdH0tY6c6NEOkJx0nSoQifnuFsTYGKF+JEJTGiqYFHj8GxsOaspfsYVX4Qbx78Rdc2wMbIu4KbCaOl5Mm7N2uD43EL)", R"(y2kdqj1nDkNQobOmh/ODJ0h6Y15Zec11F16OGA5xOKNGDYUTgkKdCjl98i3nkfbctECkwNtgJ0RMMf2Ry42jnuxdhASRqKkpWJRhcaT0nCoJbDwXJUMSDMrOKq3ph78G)", R"(Zh4ZWKAyW9siFmQtKfA38Y7s714vU+R3r75D69h0cqXXe/rF+zjlGxDkhGSjgRwolkyE4sGHonMo44trpnnKsN+dPIlt12ynJXZ9UCIIbj7c+ZVSyK1ZhAN8mSdJqd0L)", R"(A1F0VCfTLHLN6ccYvHsL+9RjH3F8mA0ywplwOfGM3BzQRKq5acd6dbkks69hJnAtlaE/Clv/XMDxW06U5CY3CyLieu6ZGHReQ/ps9i4x3LZDhfB4Utg2jRG9oxNZkpkJ)", R"(pgIzidhnQC2PcoLqUzDF/T/1TVOPyGwYU5ywvIH5mamMOhE/7rxzFPKE0gNUpgsC/CEfht578yauLdU8LZWFZXgMwMCHoBZi1zBpEVGi/fW/79oeiUzOrFCiyFekbZQP)", R"(eiu1XoT/apSgB0klucjwEukgUt6+Xn8tBqXPFV7kJHaYoKXcC+gkdjhUTQlVQfufrauC1yHo3OVayo77YL8jN4sEB7tNMqc8Gx133YVPr/fx5za7rJ6zbFZAeKiI3hQF)", R"(x6/nCOpvodymKMF988O548b+dK9vUHXZEAuEH4o4P/2p1WwWuKuVXqyOIooCXTERb1a3Ti7amLu+utqgkRevL7by9+hiiSngnmOMcIE9awF0cSAKqKr5aPpYcFwmVIcO)", R"(m3TDBqsqauy3Kwuk3/WZaLB0Se//fGM+jS3Y5ZSylnC4osWwk1cz+C9Ee7Z9XjhTmAm3kmCyswI1f2RZXVScGkvImgJTtVLxAVQGCq6Lm2j0rFhqwz9IBMjjMjf/2aAK)", R"(WQRkd4GwN3BITmrHThH63vZsF5SmxPOnqd+evDDRtuaZRkUJQz443KA+EBTehcZGKodrDasKPTuAdCN/3pHESZUXV2xzrFbUA921XwlT3ZnDroUPgzqbLL+aToXFJn4L)", R"(4rCEOvy7x+nQOMMEttNsKo1wt5fCCWlyKTqy29LTcUrycHGV8pBZ6zegd52rhH1JkHWn8ikD9TE9sJ3+QMBHx0+/wVxy6eEIsVF25xLJidn7aChVyeMxKUve4lejgJQC)", R"(BvFJm+A7zKnNekfQwkfxhT5F8cC6f3GSavQqpAdtiJwq6FnIM7nVubXUdpQ8yQ24YGTH706j3Uyt6bodM4jVlk6EifHNJoK91eQzjT5zKylvda/JGoRyRH3VARCZkGMK)", R"(tiPYfNtyvCfSIDuFRVCZmal5SLvXunHUGji9OkJQywVrcpYwfq5EOkRFZ/QG8tRerCcR8pdr56Lt5OFaXRqRcaRMFxq6r/7wPqBGKQ06ib3KlZf8EF7TFFxQt7JE5PsP)", R"(GN4SMp4eVswnHxMnMv9rJSI8qAuN6PvosSV9vGUQdSFgLXx6MOZlHdsBwJtjLbHosLxLNbqIV/yyYULvU+uSptylVPjcSuFsOH9+e4N3ftyJV648M4nXRXNY8RwFcygG)", R"(iQoAmRHO28LLbdVJS+xYmyryE416dIJnF/8Ak9PaB8YAXOxog39kIj2K8nFUCnCQj30MXINBani6621nVAzcHpWQLP/0xmJd750LWMLi6FEKQVJ0vQdh+fqB2mhZrR0F)", R"(E7ksbu5grsY/axGwiee0l1PzFezAxxXDaakfzcMdM0KETswMr61xvZLpLqhmhLJ3RZ36xfs/nZJKr/OtyfVdoyGw+w5FZzx0HsOeBGgWj9i/oiO4sUTVzW9YK+KGLzwK)", R"(18P5NEgI8POskp+IX6dwdw2X9+fc8C4ffcYrEXcsqSDUOga60Au7pS+jqIWgLTjKzh5dvsxVPibQQEraAB/6/y6FWqmR8Er01WXd1rei/GwMHujnhWeuXBOsiKNJELED)", R"(Tya9Amnkf2bc3d3Eittou6EWfKaYko1zWepKAqZVPcl6gaHsrF2RPg9pO06a2N3DcER0USq/c5I+Z9LUz7amsuMztKoH1F+P/PpAEVJvT20Wlh4T5n1AWqSl8xHdnSgJ)", R"(1IIpZk5QuZCN5/ZFECV8YRCM936tfEPv78nTlVw59nw6n0BjkWBtLqk79S7pSkDaDnuXkmCOIVYRlvyduYpNdOgfDQEcuUEAgQZSimjw4DAHUvXOI/aDY2TJt1KqNwsK)", R"(7zLuE9ZorFPLCXRnl1NuU0f7tG3O1wthMDD3QfNi3iXp12DM+1ca5F3mq6/6eGl4uh1sIn6CgWiApHdUjmrNshMDikziuTTrW+ESaQJ+NLygWeNkv1nX0EcMv713mHAK)", R"(G+bzX+UaUOUBkdZ2NFBaFLN9cV7WyqDp9LLqhcvCtuMXM47CPAC7psvnx9vhQFd4H/km4FQDF2ACNfaMRQpwauIyce6b1VCCuPEu/bk3Tc6AKELazXObg5JWKmE+IBoM)", R"(3zZ5qq0Q+fT0tg1xIA1JPvclnTalTownxM9D5CPyguRuvnKYL1AkpqpHrlrW3aHPvumjJHmfoK8aGd+qDEokMKPAumkzLIKjGJBsQSwtTZ9mBIacHNZ2zcnK3NJUxsIK)", R"(EDvcX938qjWeXUBWdkUchcvRihQgMTedu+Xj3qUAyDI5t6pp9riJacYejKHDNKlN/Ihv6WmSv2ly7J0IdfUaXxozKdCgIswfP8yMrYlSNNLsAvufcEroSAJ1gyPIyt0G)", R"(d3P9GWd5jsWcKHWFeJXhHpi1dUFRH4Yi1re+6J60b+4Fekdos7fd6Nc98CZZr25gSv0we2LOWgDHfzKewDf+jOewiCHKTS2d4dqRCkShdSQFGxOh5NXwQ3suVWPpPTQO)", R"(w9Yr9tE37qHBEQllAHSFqxVJec9xNxVGz+YKXA5oiSVcfacXSgiOHnTVDCgAt7s2IIPFPAlBbWIMTwRZV9YXpZElMbsra7KcCTMT/JfHo5FinoGDnoI70hL6Rh81s24K)", R"(yY/f2PcUXbbpdbLMPfn+xzcZA7+WGqUV7KA89tD9zyfIv8BvavaUwcRo3E/lK9BzlwoFhS3qh2NGhqPZvNL1XbuFe3FS0eymQ2b9IVW9m7tmsEmMebSq/2XoqzpW0hoA)", R"(ss8UWBCNtn6+G6YfKd4CfsyPZaSQKo+ALkADAB3ZvnRhWp6SIII/Yj4Im5/OmTqhttaU9cA213HaOLGILh7RENT/Gu4i5ynwD+ZLoOlMVOjXoqFChYlv4F74GdTHWSAK)", R"(8PJB4cRDBZ5hd38nuiN+Zk9Blmi+EYhe0ioki+6VPeP4SfYHhokGmIh8d9+X6VMztfAlJKjIBNVrsHIFkP9soTXzJ9qkWJRc70ck6liAscFRaUHLfPx7Pqxax1nI0eoJ)", R"(G3DfPEcEMNntEGh2aURY4DY3cuxedEOVwy1zgM+10Do8Ro3mAc0jcdt5ObjmuiwjA5LWbFOaWxO2nLfqfSJSNBsj7sY/XEzFDweDz0+uTHolKfOdT9c/sIIs5/YZw78H)", R"(5OC85vTcMLhhQOlps973ITOzvU396bs0WH/RL1to2YCn7Nq5Jwi8a6j+o4LKJ6QxAk23Mb69lL213CgpJwidbj2zhyjcyHv0SmlsEWSc8wWqRB3W5xyKpQeaH5idIbkG)", R"(QD2dDacXv6U/Ng7QpMme8EwRTXBtEjbbkd8sesX/dobz1VmCyyuaW1iFpzkkg+1RXfUquyueiN/FjU77an3GyBiU1auMY++XVn2pEm6Nzu0ippW757AjLSw2jkEzC8UN)"});
  return *tokens;
}

}  // namespace

void MockTokenGenerator(const size_t count) {
  CHECK_GT(count, 0U);

  const auto* const token_generator_mock =
      static_cast<const TokenGeneratorMock*>(GetTokenGenerator());

  ON_CALL(*token_generator_mock, Generate(count))
      .WillByDefault(::testing::Return(BuildTokens(count)));
}

std::vector<cbr::Token> BuildTokens(const size_t count) {
  CHECK_GT(count, 0U);

  const size_t modulo = Tokens().size();

  std::vector<cbr::Token> tokens;

  for (size_t i = 0; i < count; ++i) {
    const std::string& token_base64 = Tokens().at(i % modulo);
    const cbr::Token token = cbr::Token(token_base64);
    CHECK(token.has_value());

    tokens.push_back(token);
  }

  return tokens;
}

}  // namespace brave_ads::test
