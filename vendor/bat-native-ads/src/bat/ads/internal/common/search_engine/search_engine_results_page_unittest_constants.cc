/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/search_engine/search_engine_results_page_unittest_constants.h"

#include "base/no_destructor.h"
#include "url/gurl.h"

namespace ads {

const std::vector<GURL>& GetSearchEngineResultsPageUrls() {
  // When adding new search engines you should perform a search for |foobar| and
  // copy the complete URL from the address bar.
  static base::
      NoDestructor<std::vector<GURL>>
          urls(
              {GURL("https://developer.mozilla.org/en-US/search?q=foobar"),
               GURL(R"(https://duckduckgo.com/?q=foobar&t=h_&ia=web)"),
               GURL(R"(https://en.wikipedia.org/wiki/Foobar)"),
               GURL(R"(https://fireball.de/search?q=foobar)"),
               GURL(R"(https://github.com/search?q=foobar)"),
               GURL(R"(https://infogalactic.com/info/Foobar)"),
               GURL(R"(https://ja.wikipedia.org/wiki/Foobar)"),
               GURL(R"(https://results.excite.com/serp?q=foobar)"),
               GURL(R"(https://search.brave.com/search?q=foobar&source=web)"),
               GURL(
                   R"(https://search.lycos.com/web/?q=foobar&keyvol=00eba27cf23332982690&_gl=1%2Aaqbo9y%2A_ga%2AMzk3NjM2MDcxLjE2NTM5MjY5NDQ.%2A_ga_76FJGHQNN6%2AMTY1MzkyNjk0NC4xLjEuMTY1MzkyNjk4MC4w)"),
               GURL(
                   R"(https://search.yahoo.com/search;_ylt=AwrE19xR4pRi4HkAbx9DDWVH;_ylc=X1MDMTE5NzgwNDg2NwRfcgMyBGZyAwRmcjIDcDpzLHY6c2ZwLG06c2ItdG9wBGdwcmlkA245V3NRQnh5U1lHeW5haWhGdGp6X0EEbl9yc2x0AzAEbl9zdWdnAzEwBG9yaWdpbgNzZWFyY2gueWFob28uY29tBHBvcwMwBHBxc3RyAwRwcXN0cmwDMARxc3RybAM2BHF1ZXJ5A2Zvb2JhcgR0X3N0bXADMTY1MzkyNDQ0Ng--?p=foobar&fr=sfp&fr2=p%3As%2Cv%3Asfp%2Cm%3Asb-top&iscqry=)"),
               GURL(
                   R"(https://stackoverflow.com/search?q=foobar&s=2cacbef4-4b9e-4b96-a9ed-cdaf97f26dac)"),
               GURL(R"(https://swisscows.com/web?query=foobar)"),
               GURL(R"(https://twitter.com/search?q=foobar&src=typed_query)"),
               GURL(
                   R"(https://uk.search.yahoo.com/search?p=foobar&fr=yfp-t&fr2=p%3Afp%2Cm%3Asb&ei=UTF-8&fp=1)"),
               GURL(R"(https://www.amazon.co.uk/s?k=foobar&ref=nb_sb_noss)"),
               GURL(
                   R"(https://www.amazon.com/s?k=foobar&crid=2RGPVS512O6MC&sprefix=fo%2Caps%2C303&ref=nb_sb_noss_2)"),
               GURL(
                   R"(https://www.baidu.com/s?ie=utf-8&f=8&rsv_bp=1&rsv_idx=1&tn=baidu&wd=foobar&fenlei=256&rsv_pq=a98c990d00067b3a&rsv_t=e924%2F92qfHeGEe9hHP3joPcNEeV7qqhMrWfS8KWl7qxdom3iP3CPaNk5ozg1&rqlang=en&rsv_enter=1&rsv_dl=tb&rsv_sug3=3&rsv_sug1=1&rsv_sug7=100&rsv_sug2=0&rsv_btype=i&prefixsug=foobar&rsp=7&inputT=2637&rsv_sug4=2638&rsv_sug=1)"),
               GURL(
                   R"(https://www.bing.com/search?q=foobar&form=QBLH&sp=-1&pq=&sc=8-0&qs=n&sk=&cvid=0025E271E6E849BAA97EB176045A1ACB)"),
               GURL(
                   R"(https://www.dogpile.com/serp?q=foobar&sc=7jgbppidxNxC20)"),
               GURL(R"(https://www.ecosia.org/search?method=index&q=foobar)"),
               GURL(R"(https://www.findx.com/search?q=foobar)"),
               GURL(
                   R"(https://www.gigablast.com/search?c=main&qlangcountry=en-us&q=foobar)"),
               GURL(
                   R"(https://www.google.co.uk/search?q=foobar&source=hp&ei=tPSUYoO2Dc7GgQaXj4rAAg&iflsig=AJiK0e8AAAAAYpUCxCNJHkuwNPLoN3BeGzkEJeb4zUxX&ved=0ahUKEwiD4IuX1of4AhVOY8AKHZeHAigQ4dUDCAo&uact=5&oq=foobar&gs_lcp=Cgdnd3Mtd2l6EAMyBQgAEIAEMgUIABCABDIKCAAQsQMQgwEQCjIKCAAQsQMQgwEQCjIHCAAQsQMQCjIHCAAQsQMQCjIKCAAQsQMQgwEQCjIFCAAQgAQyCggAELEDEIMBEAoyBQgAEIAEOg4IABDqAhC0AhDZAhDlAjoRCC4QgAQQsQMQgwEQxwEQ0QM6CwguEIAEELEDEIMBOgsIABCABBCxAxCDAToICC4QsQMQgwE6CAgAELEDEIMBOg4ILhCABBCxAxCDARDUAjoICAAQgAQQsQM6EQguEIAEELEDEIMBEMcBEKMCOggILhCABBCxAzoICAAQgAQQyQM6BQgAEJIDOgUILhCABFD5A1jXGGClGmgEcAB4AYAB-gSIAeYOkgELMi4xLjEuMS4xLjGYAQCgAQGwAQg&sclient=gws-wiz)"),
               GURL(
                   R"(https://www.google.com/search?q=foobar&source=hp&ei=a_SUYqPWM8zVgQat0paQAw&iflsig=AJiK0e8AAAAAYpUCewOIHMGsHZw9I0JyvAp36Vr8ebqy&ved=0ahUKEwjjt8r01Yf4AhXMasAKHS2pBTIQ4dUDCAo&uact=5&oq=foobar&gs_lcp=Cgdnd3Mtd2l6EAMyCAgAEIAEELEDMggIABCABBCxAzIFCAAQgAQyBQgAEIAEMgUIABCABDIFCAAQgAQyBQgAEIAEMgUIABCABDIFCAAQgAQyBQgAEIAEOg4IABCPARDqAhCMAxDlAjoRCC4QgAQQsQMQgwEQxwEQ0QM6CwguEIAEELEDEIMBOgsIABCABBCxAxCDAToICC4QsQMQgwE6CAgAELEDEIMBOg4ILhCABBCxAxCDARDUAjoRCC4QgAQQsQMQgwEQxwEQowI6CAguEIAEELEDOgsILhCABBDHARCvAToICAAQgAQQyQM6BQgAEJIDOgoIABCxAxCDARAKOgcIABCxAxAKOgUILhCABFBdWOYLYOoNaAJwAHgAgAHzAYgB5AaSAQU0LjIuMZgBAKABAbABCg&sclient=gws-wiz)"),
               GURL(
                   R"(https://www.metacrawler.com/serp?q=foobar&sc=1txEVm9N438G20)"),
               GURL(R"(https://www.mojeek.co.uk/search?q=foobar)"),
               GURL(R"(https://www.mojeek.com/search?q=foobar)"),
               GURL(
                   R"(https://www.petalsearch.com/search?query=foobar&channel=all&from=PCweb&ps=10&pn=1&sid=s26awog2d63p2eryj0j1t4jtde5bxl0l&qs=1&page_start=0)"),
               GURL(R"(https://www.qwant.com/?q=foobar&t=web)"),
               GURL(
                   R"(https://www.semanticscholar.org/search?q=foobar&sort=relevance)"),
               GURL(
                   R"(https://www.sogou.com/web?query=foobar&_asf=www.sogou.com&_ast=&w=01019900&p=40040100&ie=utf8&from=index-nologin&s_from=index&sut=866&sst0=1653927358545&lkt=6%2C1653927357679%2C1653927358446&sugsuv=1653927332873722&sugtime=1653927358545)"),
               GURL(R"(https://www.startpage.com/sp/search)"),
               GURL(
                   R"(https://www.webcrawler.com/serp?q=foobar&sc=MQ_doqXGq3EycDL_UUfFwdc46L4OQAEeXLiJA7JSoqit01lROq--mxI5cErzOAUign5nlzOVX8I3S-g7vV1NVTlsVyPA0jnDnTu6cDXzmNBDI6QfiwhNOVVTbHw1RpzacIMvLERSIpdyBa6G-ES_1ZkCT68FZXhd-sz_3nyPwdkjEjZzPS6SxWzXrzua0JtPxbhJYLEHouYbxUBEFEfpeaLpuIWgDd-47PqBnopwGjBmnkaFZ0sIG8HkZHimpKRprWJyZ2jUIKSw-yA6Os-MQo7T1hbHQBmlt1ZwZA5vadVGaNAaQ2bdKgGzSPHHIvq8czhoFgRcU0zUVqbPaI4Ak-3mo9J_K8aYAtHorJCeYw_c23BSudWciKLNTEXnm2HdiXwxrhmaSvYgq2dhndpJ6airnmBaqrW6kyri_RBHsda6GhEWKwdL6Z99Q0mPIgEKVxbEa4vIQyHgWnLSuqBsPyLrymO4pzM0YLI8A54orUHQBY-JTxtf6NC-2Sy3GfpXX0BFeqj793Cgy7SZBLBu6xfxnlB2Expj0tWoXJN_RQtQAz3MN6HlldHkx-vgOnn_92hQjPjRum8ICykuvlKpcclRpu-c1rjpS2PI4cUU-aLVIV4X0HyarYK3bcj1ntidbV12AymDCQE2wiTIw3jQHHr3mNDVimR5jWc-S4RxjoJIOlE0kg2ZNiAcsIG3Iycz7627WZdgBFtT6dVIrrpVnziIspj0Ykt_K4RZmKBOhGbv4f9ko0RJIy4ZSq1q4m9NXtWYbWFEav00TPlHhglmiX4DRGvCca8pvvdFo12XTdeKJdAATQHM2efrbKf8EsDgIiR3ithjV1len4JYAbzoK7OdyfRw81ACjHJXhIprnQRRZDrJHRGuVceUDb4FK3m2Y7HNNffVBoGn40greWpbA9FZLZImHoUxTeXOVzAp47qOO6-ExMlehN5iNV2d6aWqQx6SubKPC6t80SKhzB54qFbzbTtSjFs30ZztUVGFshJRBuG6he_JAZDNFdhEZRaLAkOTSp51HAXzCkovTMwiXoF_pjzCjsqzhRd0K05Wfh6eZap2ijQ8_V9rP0HwXJCDnF26daFm-Td4XK906t5WYIbum-J2eJGZ0aZ3lL71_mJzYTJW-QaToDlmikWoBTfb5Pfu0fAjeL2n11AJmAHuMFiTwisQrwRWIT8B2dx-ObXnkiVQa-jP2FCMJE7F4XvveyDeBSVLiPgIZYhQOry73fNzenDtdySAfge9kC4ypr9hxEvjfQzGORKUlDJABC0yyBokSFA8fng_5401iG66BkqIDe6idz-7C6MvsEbm3_l0vETzOLoQOLCnCxCICUV74BYZGbk_U4gQGouUp_aeUfRr8iAeB94CUhuLYVNzXxjIubOpWeLZBLcEo67eHPlMT6BvIbqpYudl1KeKRUP_FtYZx6sIvhL7pKRWDyxm3jewsAoLTO_cUv3thVBPfZEQSrkOYtVzrDr4ZGT3jmLS8W0iWOZM-h5vWvoQ0Wmb0fq15_twlzrVRjuOAGJ_2qMZhonmx0mfdXcQxzz9j39zJenVrLDDsEgLnEcxn2ObCYrSrNnCOu3EuwqaPRGmkf38pANr3ZUnFQAwSl8GF84pqjADQ8lDfsc-xu8RjbmnE0GZwaHHVh98lkmN_cFXS1cpGDByli6NVjE26H2VqTsFOn-3Lenc_nnWyE9srd1jmncLSFu70WxZ0pTCIsAlU_44PkmAxqOWABL61n73i-dc7OZWARzYH-YZ_57fR0IQT3uvxv8J_ZPUSsHCBtmAmIAwRhDbhzNFWh7K4C1SOIV9Mkvny0hjcowxy39zju20R1RzKDiAnQMF0vMYSmjkqynQfd4TEP9LKPogM3gJIafVBLtYisp-E23t0WZpx9WXWDLl)"),
               GURL(R"(https://www.wolframalpha.com/input?i=foobar)"),
               GURL(R"(https://www.youtube.com/results?search_query=foobar)"),
               GURL(R"(https://yandex.com/search/?text=foobar&lr=104993)")});
  return *urls;
}

}  // namespace ads
