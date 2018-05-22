
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_UPDATER_LIST_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_UPDATER_LIST_H_

#include <string>
#include <vector>

namespace brave_shields {

// Note: MUST exactly match element order used by region_lists in
// ad_block/lists/regions.h

const std::vector<AdBlockRegionalUpdater> g_ad_block_regional_updaters = {
    // 9FCEECEC-52B4-4487-8E57-8781E82C91D0
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (ARA: Liste AR)",
        "gpgegghiabhggiplapgdfnfcmodkccji",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnbHn298ZQjKnWlC6Ngkv"
        "S3Dr7Neu87d1h8s3b9GTlc1QNDWiYgY5IfWVq/1FBw2nUFE/v8fNJg8quq8Z2nS8"
        "dYiJDVSGRggiCooa0OTCARL0BsGxHZO6s2QROYIcxPVnzISqg5zRIBc+8npE68uV"
        "UrDR6q/KdJ8siL2hrR/NybPp+uTK44lHOEIBFm8ih1rC6z+Y5dHfhax0CuL6wlWw"
        "VNcFe1macYEcOXShwkUOADh6rEBQZKJmv474xJutmB8nIpGq7C2Hn2HNNyfA6tYm"
        "hVlsaeEC44phGITKDai03wFsWWkHQPEU5HwFzKQGIBFwudyO8iigO5m+d3XSzgSZ"
        "tQIDAQAB",
    }),
    // FD176DD1-F9A0-4469-B43E-B1764893DD5C
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (BGR: Bulgarian Adblock list)",
        "coofeapfgmpkchclgdphgpmfhmnplbpn",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoqpe6QWKketr66AW+hKS"
        "xr5qds9gxmU72N20bG/nG8wfbPUdTwEqIG3m5e37Iq1FI0gcQir5UqwZZUgkum5d"
        "WJwgCB1SOaVvlZrWZbTbATKFcePswHqAIXMfS+wzMA/Ifoky26dE4k/rs6J/zQEb"
        "eXXon/ikjGJ7GxYeHYMBz9DAPQhcUoBlh1C0O0vhvXU+kG5DO4wdIt9W/cXJtv+8"
        "OTZ6HiTJw1j0jAliFZI/jhkYB6MW57OBpBYlWJQhMbLbK5opXq6d4ELbjC1amqI1"
        "lT3j5bl0g1OpMqL4Jtz6578G79gMJfxE3hA5tL0rGU3vAmwck/jXh7uOOzqetwdB"
        "cwIDAQAB",
    }),
    // 11F62B02-9D1F-4263-A7F8-77D2B55D4594
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (CHN: EasyList China)",
        "llhecljkijgcaalnbfadljdpkpbehakp",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAyahWbgHuWAI7CkBdclxO"
        "lehPVVGuG8u6bPi1vs016Kbhn9GThEVIP5qzAFQLA3jRrGy5B2nncdaCnibf7BkG"
        "sNR7nyQQuXAI2FGk9qCm36ZF7FI/yjtN0S0e6LzSswOcVhTdPnVkxYY6UDuKyzVR"
        "xgbF9Yg1aT45NFpJFZKFtKHexnLiY6KlZKV6GhY1jucjo7W77xdpLaspkYbQ69Uv"
        "DlSA093InAzzikuqBdKvY0FPvC6pgiefqWTMa4M1cZU9IoIiukqrpXQn1tC9PJ8C"
        "U4XKCTshaNbpX5wxY10rUl7i/WHNcXCfmCXxKbqRZ1SyH6KiiBrDpSnfKXxrQip4"
        "GwIDAQAB",
    }),
    // CC98E4BA-9257-4386-A1BC-1BBF6980324F
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (CHN: CJX's Annoyance List)",
        "llpoppgpcimnmhgehpipdmamalmpfbjd",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAudVtKg3tZbgealGvVzbE"
        "L3yP1YWdt6GRDreqy/b3kCce3AZ8WroL8jb6Zj/aapBRCNxBezXzij+b6QiIH/l7"
        "sn5Wf5HDs5Vnrx4fDvGRtSLpgP0cSuFGVDx71TQz4X+AnUubOeHskIlJJAT4t4cH"
        "Ws9c7EAl3ShG7DtvL2qHG2TUfJFqYOMOtQd2qG5H+X9zAUFP/qRHT55gzce8h+SX"
        "CsvdK4B8XK1cdvbIykllbGPzZr/TANn9gCtMKxUfk1qFn1uYD6mzg80KJmof8MHb"
        "Lon6KLMqywcqfwEwvoivxo6f5LkOUjhqDYZEQ5la3h7lFfHKz7fCE7FCww7bQ028"
        "lwIDAQAB",
    }),
    // 92AA0D3B-34AC-4657-9A5C-DBAD339AF8E2
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (CHN: CJX's EasyList Lite)",
        "lgfeompbgommiobcenmodekodmdajcal",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtAMyjMBZCbqNIuO01ZFJ"
        "5iKmcNFuJHXIUqhO9s6j6XnBAfak/OOk4s9k3maXyhaynXVpYATQyRHR0OEpmsQa"
        "wFgKCmVm1LB68jxJ5Hh1ZITG1UyfznYnozkjBtzdkMGKeuZFBaHo5PPueHVO7yJD"
        "HvU3UFW4vCJ01twXiH4y0qaYjL1CPr58J9U0oKxptsfwEC53WcDq6mKtAKRpyxN6"
        "vbtFJ5/li2yC0Ms+8Xe3Xv5ovniM/4vNf3Jn1w0jzgrDRcW2VhxpydsH6q7oaR2i"
        "gIzJ+XG6/k0g29CJhfT85dJNF31TwqvoI+Ju6hjZrEmSHmC7gbY7gN3ak+DbUrQx"
        "jwIDAQAB",
    }),
    // 7CCB6921-7FDA-4A9B-B70A-12DD0A8F08EA
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (CZE, SVK: EasyList Czech and Slovak)",
        "omkkefoeihpbpebhhbhmjekpnegokpbj",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuYBXbfloR5HddFlg80U8"
        "+pf5TqfFJQAf1bL4myp9KfGggwqrjuRzIkPOD8J8IvCp8tWv2f4QK9sAPHhtV6w1"
        "cnYX24lKxrQ/lHHAV6/CEcFa+2Yk7cRLKDC10H3r4FMRoCeAy/ruTjVPfIw+GuAf"
        "FYl1qYWBNxvW7XXw7cCIIYL4j82YQF6HjsWbTT+QHLCR6h66wvIyVQC9ppjJPxDa"
        "Eevjt4tohEFAB1NBC+Wxt8H/P5r5ayNcLnb9Ygt75haYL8VWZOJhO/neSTyuidTF"
        "G5ox2Ruc6TXP8t0IqpVtiZUDkx1jzUakIHoKNMBc7oz3P/SQ4AanZsIliJobXFeU"
        "iQIDAQAB",
    }),
    // E71426E7-E898-401C-A195-177945415F38
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (DEU: EasyList Germany)",
        "jmomcjcilfpbaaklkifaijjcnancamde",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1HeinhQW1+SS5Xkfxb1K"
        "GAdSQqcm8bTx3fg40xVwsSre6B/VKnBLgOD9mTnbBHBY4rfymxwhD7dTHf05Y7LA"
        "kAhTEFLyi09GYPBesJ7uO2EZMYuPEd6iKx1lKo/zF9eO0VrDtjz+vw9zwriHtFMJ"
        "Lxz2+QXH18tx/jcvRiM8mxKB64ma+mZO38zHDs+KPDNBigtXcMhjfKbk3vnl/bl/"
        "Adzibx44gEol+abEHkvItLwdaBb3vRTEFiiO8MoTJYFY8qMxwZK4aSr6Ox5yDKzC"
        "c7dy/eIarb+zfzbk7QJD+FsNuQ34h/7gMnUE+AX+SNxFpGKRRAJFHbBPr/ofHxor"
        "UQIDAQAB",
    }),
    // 9EF6A21C-5014-4199-95A2-A82491274203
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (DNK: Schacks Adblock Plus liste)",
        "facajiciiepdpjnoifonbfgcnlbpbieo",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxvmMjXthrTjGl0WDrL+4"
        "qYSVao5vKPGerr2VeUiZ1o94+P0IJyZzq3d7UP2nOvhveGl15YYxYss+sD/sUkUW"
        "57XMx+H4TF5OzGCwV8nkz4VoMIfEU6CKgYmRGHV2VoMdIHG7R++jX20+GAoeBw+a"
        "Bx9+AHlBouf1kvqbkutVh+Bre1cVa6YsgsPVcmhiEp7wjz2yB23f44+pBIQgWlKW"
        "n7z9e1osG4LUCGk6gavtRoNGS3TAUf1Sq9EUibFJVmBjujVoiQKD8GIFKmLM9Fxl"
        "1Q+xgG2PCCSBz5lSesHkphDpwhszedurpKbWsnsRPqbqR3GmpceKQheWL/Y56tf2"
        "gwIDAQAB",
    }),
    // 0783DBFD-B5E0-4982-9B4A-711BDDB925B7
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (EST: Eesti saitidele kohandatud filter)",
        "fnpjliiiicbbpkfihnggnmobcpppjhlj",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnrl1tavPfozqu7CmqNfV"
        "UtZfUlIitbWpFBRn+HVW0oEFUNqAwNlwHqy9QZP88wKvb5N3EJj6NAq6je4ii6nM"
        "kDn59teNzGA4m8QSkeOWT6pNm98FZA6HNHPnhnYSG2sT8tpQ8Uyh4ySrxj2ijVM0"
        "Hc01WKQ6zjkvZWOuZWllsCejRZmxGOLUUy5mtKhIfHiuleZ7AmKx46AiVFvrpvV5"
        "x8G2HKAlF/uDc6LmV0lfXcROt5RlY+kD/sQ6wKcatibpHbLoRHOJx3ac13+pvt85"
        "773af0MdrvdCYjxvqn3DJlKw9qqk/B59n+XdTmWcfC9k77Z0teoMM5EBy8G1nGbe"
        "lwIDAQAB",
    }),
    // 5E5C9C94-0516-45F2-9AFB-800F0EC74FCA
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (EU: Prebake - Filter Obtrusive Cookie Notices)",
        "abeicfkepbhgindohkebelkkhnnijcaf",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsDGkXlpuILNJ/iPEsXXd"
        "w+ZVBPf7L11UkG2az6NaeumxKfOTBDivPCXz48lWG4d6jySR8n+eMOaqX8FUVc50"
        "AGnWfq/Ud7PJdkvYNEyTNjZw7yQUta3q1yfc5gVsc1IiG9s0GuZxzSWnI0zfwMIG"
        "6JNTZTdPeIL1VxnFySXwyYKr1QhkVcpiN5AYNA8jXmZmmZ716Cti+1kAlGLQSO0A"
        "BqFZhJvSMGUb+12z6BW20tc4spt8QgY6a8CFMasdoD3hz+gKo+rvQlgPGWQhWxuT"
        "gRJZQnyS3EKeJYBaaS11c3xfIqkK85VXuMt9x/3bu9NkXFpXcNwqBo9zs+gxodVS"
        "gwIDAQAB",
    }),
    // 1C6D8556-3400-4358-B9AD-72689D7B2C46
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (FIN: Finnish Addition to Easylist)",
        "kdcalgmhljnckmnfcboeabeepgnlaemf",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA3seBXoyYSdtiqNAIaS5v"
        "9jP6Pr8xqgFnZyHknxNsC92fHyRW2nbuwMr78pWA4vPIyV6BFG5jS8k2RXEbWiOK"
        "NNsw7nWlfT4QMwkEu4uU1vqxsNDtdc1rdrc69aBegyNOQBS+W6aP1ESHp68AoalY"
        "KMHKpc+fi00sdQwYU9Y5oW9q4uRX3baAyuGZjP0xuKN3t+T1QnhbhkldP2WP0ooU"
        "/VRMhy2rYoE+W6eQRGrghJJG/wWznz5AiPD9EpPST/hoVWOKVco+12IbdILw7yGX"
        "2c65xPcLr6obVR+549QrgxU0W02XxS2lXKGc1NT2Zdl6ugh6XpW1RHVz7SjLIZgi"
        "fwIDAQAB",
    }),
    // 9852EFC4-99E4-4F2D-A915-9C3196C7A1DE
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (FRA: EasyList Liste FR)",
        "emaecjinaegfkoklcdafkiocjhoeilao",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsbqIWuMS7r2OPXCsIPbb"
        "LG1H/d3NM9uzCMscw7R9ZV3TwhygvMOpZrNp4Y4hImy2H+HE0OniCqzuOAaq7+SH"
        "XcdHwItvLKtnRmeWgdqxgEdzJ8rZMWnfi+dODTbA4QvxI6itU5of8trDFbLzFqgn"
        "EOBk8ZxtjM/M5v3UeYh+EYHSEyHnDSJKbKevlXC931xlbdca0q0Ps3Ln6w/pJFBy"
        "GbOh212mD/PvwS6jIH3LYjrMVUMefKC/ywn/AAdnwM5mGirm1NflQCJQOpTjIhbR"
        "IXBlACfV/hwI1lqfKbFnyr4aPOdg3JcOZZVoyi+ko3rKG3vH9JPWEy24Ys9A3SYp"
        "TwIDAQAB",
    }),
    // 6C0F4C7F-969B-48A0-897A-14583015A587
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (GRC: Greek AdBlock Filter)",
        "pmgkiiodjlmmpimpmphjhkodjnjfkeke",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4KGRug8Rw1WHk1BPfIdt"
        "dw7uwFijUac7jk1lb99lEfSq2uPV2bKCk8lLh/6ahlV/EjSN8mGiFfZIVTDFhuYh"
        "VuIO8iETrCZe1ChoI0F8ptHOPQXVPzKUFMpkRqAnH51vqx+3gG78A3+iGfAE+Lje"
        "rP1j4Jx5jSvTkbN8l+RqKMtjaaL9qRHv3aRQtYB/shGgdxKeOR0f8E6yJ4tIRDHB"
        "72bDufN7wbnRoHCNnLkrAPtbIwpWRLKYcOxAB6QqKNCLx/UX/pWpGtyJmMQQBpxQ"
        "gl3BT8daNp0h4Soc6VPZA9wEIQ5/a/8UpsBT9rwJGj5WdSBPSR8D54aULATPxsie"
        "nQIDAQAB",
    }),
    // EDEEE15A-6FA9-4FAC-8CA8-3565508EAAC3
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (HUN: hufilter)",
        "gemncmbgjgcjjepjkindgdhdilnaanlc",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4HNXDsDBPP4b/irxacZM"
        "YnaPjNMXS31e11nsFBvN9lFOkuwF3bEk9uEk0fDzocF6GSpXbUE0HVTqfKTTnZfv"
        "G9m+C3nT8j6N7BB/wST72s0zXCjSlLWJPGmFnFb/EDkFAGmA9FU4C+j28Obehd94"
        "OC9pSqu8DYK4LbMWPmk2fgpO9N3ZV/5Y2Ni69WKJwT72prSMzyVVEAYluCYPQWY9"
        "3g6dJ9RBtwnHCmdK5TG/bN2q6f50Cw/aJSv8nshSdp+KJK6yi6fBOxF5Xb0Bj+xZ"
        "GC4K4SW9JjElswaGJi2PX5I11w7xC24jNaW6BUHcJ6IXudIVmBFQxWWxkMVwfgqN"
        "lwIDAQAB",
    }),
    // 93123971-5AE6-47BA-93EA-BE1E4682E2B6
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (IDN: ABPindo)",
        "egooomckhdgnfbpofhkbhbkiejaihdll",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAptA5jVa5JkYI2jt905om"
        "4OLSHGahwgS7tu7GG0sk1YNafOo4ajKrN96Kxj0fgwGrJPhU1UiTDmrgLTZSbuC3"
        "hAscbfhuakVNo1pyFfSAVoLWSrOq5l4k6zZK+y1ahxdyJvlbz06RWE6OhIqExxGq"
        "LyMjEknkPGxBVO0cKcYHiGYUxvVPxQOg+9fGieXMlSGs/L7Mty1oJOoZ4JcPIFeS"
        "vQ5ax48E7l+yAW6psNpPqRAZ5fm7hhZXjd5+3cfXXIMStgX3X0MUHjx2KpYlv3Nx"
        "MjaZQOAZiuZ3W/H7VWnV7V/ScJ9Eb+e6iG4XS15f7vFQu4zPy4UTYOl6gXnIGWGm"
        "sQIDAQAB",
    }),
    // 4C07DB6B-6377-4347-836D-68702CF1494A
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (IN: Fanboy's India Filters)",
        "fijddbnggnpidebfbejillgbopcikfpi",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAraHfJfoceCra3QEYhK2N"
        "jl8oUvtDOiT60o/HjpSsarfEgIgCKgqF/sUGK2PVL4c2CsF+fG+YzI4eGrJ85rXL"
        "5Z47lmP666r9lZcMVoGs2L7RT8UvsVwpyxiH868kLVxeM0NBas/jIy7jAul5/fGf"
        "0fRUF0TbSIG/tghO8nR4bloYEbSri9BWcl6ccMfAKsIalNWnP3uifjC/t7BWIkY4"
        "86zq/UPXdIuJbRPWqJDTS6EZH5ZXFDkLE8tta3OrRV7n3ei2HdyTnJOVj2DSskKW"
        "wAA7fpYGVnmJouBqg++kC6YqSnaW2ElVzQIaiRigwXj4keAZzMoDbTrGh7g+LSVn"
        "gwIDAQAB",
    }),
    // C3C2F394-D7BB-4BC2-9793-E0F13B2B5971
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (IRN: AdBlock Iran Filter)",
        "dbcccdegkijbppmeaihneimbghfghkdl",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAm3AZE2ne7R55X2j6RxAQ"
        "HAZKl1hNPgwLOFsYpfAJ6m0uXmKJspguWxatJ9jDBbYLmtXnwX2WORILq1+r4kFt"
        "TcN8GNYe7/7o5yDLucI/W9d2vCjmEg95v50MzVQZSwd2gNZVZtL1s0S6pBwX0zI+"
        "6kHIFr2xqGV/FNE8L75f30rriQ0xKmenI1OWjyn8gNqIp4mKZW6XxkMRRS9+e0yn"
        "Di4ysQA9Ub5YHJxm0t62eqTmIyemgRhP6Rdbi0+GXbqFPjDfC26rtD3wy5f3aYL1"
        "V+2ADpdDyCeNlwCH7+vC7LWujqNTgK8wVJ4eH5VbUKC1e9cm/T57OsHJMDC5fbUu"
        "swIDAQAB",
    }),
    // 48796273-E783-431E-B864-44D3DCEA66DC
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (ISL: Icelandic ABP List)",
        "njhlaafgablgnekjaodhgbaomabjibaf",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqSwaNKhg90HCheaJu3sH"
        "ocbZUjXDs90I0OmijNkDeS291wUjvXAm5YqhNE8aZmPSMVZBjBCwKXrrtTOkMA1b"
        "1uBqJ2P83fCZsgNZWbGTD8MorMrU6vyqkWCqLRc+bTTUgzAd55ckUJ/M+HVnjo6Q"
        "fqUuB3kVzjpwJorQQZUYOLcgDY/Q5/tbrXI5+OGVxAb21pmnk8JHXNNWB2NvpA2o"
        "3p0ke/7WEoUH24l91ndOkXkN87eO8rSysl07Eq7gshbednYYiCxRPjuX0aPqbXMY"
        "NWXa5NdvIXFJcD2xV/l/QvXRYl+7Ca1igSXaiKc5eJyKSRqY4lf2vG0XCH6VZVxZ"
        "uQIDAQAB",
    }),
    // 85F65E06-D7DA-4144-B6A5-E1AA965D1E47
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (ISR: EasyList Hebrew)",
        "hjeidaaocognlgpdkfeenmiefipcffbo",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoUnmZ/fnWAGhAywLBs5I"
        "X0OMxK6LOwtjljcwEkt8QD7ZKBekxq+MDrUuRPzav3ky9IyREhXe9F4UWBKPDD8Z"
        "QXZ57WQAAMAp3IxbgdAsTqqTEEReUVx+pzjl8lxdp7xEG2gpuM5wq7bjn4zJ3kcd"
        "j3vx7bec/YbYf4fV0brQPWghKf2sh3mHXOVh68wEFXYBvcWkGXfuBoRbB9WLflqZ"
        "YRk3GrLllwBLn1Ag6iuKucvoyv7N23qXKIjqAhyKPmHx4l9w/v2c1pc3NB1af2xv"
        "tRWaQp19N98QouFFx5MwAI9+jR77Eox6QvRwA+L9CFkYlDTvT/aS3q+Zb1QH/8AE"
        "4QIDAQAB",
    }),
    // A0E9F361-A01F-4C0E-A52D-2977A1AD4BFB
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (ITA: ABP X Files)",
        "agfanagdjcijocanbeednbhclejcjlfo",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsorIQuFuMI5OaGYaYTu6"
        "+kZC4j3qPoWRoD7F9GS0IJC+VEk3XQ7UTsRXlrIxP9obmC7+pByP6hknBUzvKKS9"
        "I1v2voIqjUoydWOozbfoVoRhTLN3UiDnoDueXqXiv1MGLzY/ZcsxsxAlIiTcE7+/"
        "KdM6pJ72Mn/aLKU3escIJ5E5qOHJOFDLW9587JeWOzexaCOrtiZMclE0KWbUi7qB"
        "3Bz3auF6piSzoNGeI1NMwHSSAwhDOQ3UK09aqRKhyfBq6ugrrYyRAr3FWqmMBWki"
        "Tsr6SzrbQg3wcGbD+GDvoQmqVf8dH/WYG+srR6PyJdYH5mOQs6Yg+nu1gvwQ46Z7"
        "4QIDAQAB",
    }),
    // AB1A661D-E946-4F29-B47F-CA3885F6A9F7
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (ITA: EasyList Italy)",
        "nkmllpnhpfieajahfpfmjneipnddhimi",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAurn1cJrIcCa8P7hjGex+"
        "OUHi19PRxmjJ5DuQlMAeIaKibwaQOZEPXSvD+O+xgxHZJs1o2DE8zfj6yrAmDfu9"
        "+/T0ArT2RWuopDMEfaKdeG0ylHP62WJC+KGUhCiTNmLyPxbU9AiwydVyFOam8vs4"
        "Tr+9I3lYKVeClQrtDRM34BTOAsuHRjiuIKoC0jDC2kc+BAsAbzhIdrkEDGD+qx0r"
        "CRnGL6c8xODe2PLKSkCSIsqOk44eYOkBqQd0SgmCvQjXS2XczMDNuV7DCZofErsy"
        "2iEv/2kzhkkN8GFwbRkYGN9LuK8rtekE34AvZKRHS6e/pHjUCYJb/2xv6elC+VLs"
        "JwIDAQAB",
    }),
    // 03F91310-9244-40FA-BCF6-DA31B832F34D
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (JPN: ABP Japanese filters)",
        "ghnjmapememheddlfgmklijahiofgkea",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsOoGwN4i751gHi1QmHMk"
        "FZCXFPseO/Q8qKOQViZI7p6THKqF1G3uHNxh8NjwKfsdcJLyZbnWx7BvDeyUw3K9"
        "hqWw4Iq6C0Ta1YEqEJFhcltV7J7aCMPJHdjZk5rpya9eXTWX1hfIYOvujPisKuwM"
        "NUmnlpaeWThihf4twu9BUn/X6+jcaqVaQ73q5TLS5vp13A9q2qSbEa79f/uUT8oK"
        "zN4S/GorQ6faS4bOl3iHuCT9abVXdy80WSut4bBERKgbc+0aJvi1dhpbCeM4DxVV"
        "iM2ZccKvxSpyx4NvWj56dNKqFLvzoA4/Chz1udxifIXUHh0701s1Y4fLpY0wWP0u"
        "XQIDAQAB",
    }),
    // 51260D6E-28F8-4EEC-B76D-3046DADC27C9
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (KOR: Fanboy's Korean)",
        "oidcknjcjepjgfpammgdalpnjefekhge",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvCOljWKWLWfq/k/BE9gI"
        "ZtI1MstmG+NcgGBGAP0R7xgaUMU5phdSbQf83Zt9ctwDRpisHWlGS6o+tk93zMIo"
        "JVj6RMQ2Zee6QPAKAGgwuCXF7A/ciI3lRyX7ts49XV8GAbasu1mBHntz+GpmOVmo"
        "iRxcDMUDDEqsSXgckCM9HkYvIyHQWyEgeulKdhQ2HoCptD2Wgmws6NzRTgQ94+DH"
        "u2o6J4MsG74h7L/cG3XB8WQNuqlpjjFIQTXftuUWDSkyR3tlmMxGN1PXAH6RZBNm"
        "wQTwdgrOAqEup82dWaO3BqoYGZdYeRaUGRc73iPdvvjZb1tvmqLdVSq7Ur1XJjJJ"
        "TwIDAQAB",
    }),
    // 1E6CF01B-AFC4-47D2-AE59-3E32A1ED094F
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (KOR: Korean Adblock List)",
        "jboldinnegecjonfmaahihagfahjceoj",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArcE90V2GoQ9InvoiVqxs"
        "EnhHhdJz68yyWA+HzQBxcxitqfF5Vu6gRiKB3+W/iTJ9XNE449krWrYAjDqVt6GF"
        "8ilpieIngwCNsl7jP5RRhrcrk65as4bawSimodit+TVi7ZpIFDWoalj5RIO+rjKf"
        "wkkBHujeM7qYr/Vm1NXre57ea80TvbdoA4XgnAYHKY5VV/WstFL8FR3Do+38EyaI"
        "zKfoiRrV97BoC440oifNdi0nJQhIgULfNjUolOh9eAQKuymId+WjZeSKSckUyKMQ"
        "sQ0VVDjCbm5mwRqC5D+MT9L/8sKcdXBXuGrXyaZzp3eOmc41q01VkRLXCTrx37hG"
        "QwIDAQAB",
    }),
    // 45B3ED40-C607-454F-A623-195FDD084637
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (KOR: YousList)",
        "djhjpnilfflibdflbkgapjfldapkjcgl",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAux80m8cYDEXwq+nMwmui"
        "6NCO9SFAdcGly5eq4uGEIQNB1R6Tr9JMqosHLZ4PnaUJqJwFfLWfmxzXj3q0DIpq"
        "qpdSq/jTYT/MvOldC+VQFO+NIjXhtysh4Z5F0BzlsQx/leMnV6yoyQjBX53n9cl3"
        "BvQK/EdbuQSDiNqX2TSVLm7hnr7Vf8m4XYRSCSJybY/1Tk3Cqgqywlkr+YN58L1/"
        "txXCQ9LJ5SxJ9I56TxqA1uT97hBmQikvnopuLh1SovDfjtCZwWwaGDD4ujW+Qaeh"
        "9dRrojS47iwG/Twu1xbb7ra8cn8BxdzsPjUSSurpPz/9sUooYOGJO44p7u77sxeT"
        "XQIDAQAB",
    }),
    // 4E8B1A63-DEBE-4B8B-AD78-3811C632B353
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (LTU: Adblock Plus Lithuania)",
        "ekodlgldheejnlkhiceghfgdcplpeoek",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA5dB+7xR4lcPQCW84V4zh"
        "LiYhAvKxgdo2/cze+C8E3+ye1AO+a1CWbdPgft36vtTm4nkDzyC3P9O/aEU8jxSh"
        "KEU1DDk8YBdRnvctQ9PPvwNyeS9LCYeT5a9crE9M/Z+kaFyq0SRe5cpowOBG8x4O"
        "YTt9Y7L9whEGzZYRZlgklli1AES6e2B9XUAdHXV/wHsaf2FrdPFtDfZZEFdr60ed"
        "k4f0iGppiwkaGJiOWVF1ya47NoSMl4fIF7Klw9OkfKLJHjk9YXZmXCfqxQl8FnBF"
        "e/SzbSTVCAhdaggQAwG4VmojjMrBHcQl0VJDmpoY2jFZkiO3GLmAZCYIYaN1tFA8"
        "ZwIDAQAB",
    }),
    // 15B64333-BAF9-4B77-ADC8-935433CD6F4C
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (LVA: Latvian List)",
        "hmabmnondepbfogenlfklniehjedmicd",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAst1posKDpKt3WLU07Czi"
        "owQnBKYXzH2i/sDdJfMuTcKSNQvn9dxbHVLhg8Ih7NmN6SSJTRgb2PughdVNPXql"
        "T3/jGioDC0gN8kBrBoN2YWgIW2wdvTCPvBOfwTOhGueQY6AtE7zD/3m9v6Wfcw07"
        "Rj84Su0qI1Zadmq2pBWo5z82vOAI2yV83YGDbnyK1JaFeLToYQmj+bMEojoZ4Lk4"
        "PbFmopVh1GkeOdCKtVN2NTIy43N/w0tS0wlLxjwTyZ6RIcK3VOhQXBqcpwKpKm/4"
        "WDksTvNRLZ8e526z/nqaasM/meS22hURh6NPtIOdy6/TspTzFPiRdj2xgNfQZ9oR"
        "xwIDAQAB",
    }),
    // 9D644676-4784-4982-B94D-C9AB19098D2A
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (NLD: EasyList Dutch)",
        "fbmjnabmpmfnfknjmbegjmjigmelggmf",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqqfwmNS4XOq9pWC3XSMt"
        "5WcqoKaj3lRpYAwZKTP+6DwA9pG+Zw+0iWC95riSLjqPgX+0d2cZaqjinuNn3mUM"
        "OeGdbwSIeRLE50J5J/dMmkg5YO09orZKLBjMfJG5IDgfXdZLSJtmzKC4Xj2y6KSu"
        "Q7N0Sg5f1Ecc19nFbcFazCaIhKvcoA84J7Twf2IoCDuPMsGplgZCBtFQkKeqILaV"
        "hJZeD0my6pdC2KJREbM3eRnntE44O0sbmemCfHs9BV50hVb913zGDZ379eTqg3mP"
        "jvH+VnY+7RvjVPayJP4+51zRJYKi18W7KMry3sj4ZZ3EyNKmbwlGQOzAyd/Qtj4I"
        "3wIDAQAB",
    }),
    // BF9234EB-4CB7-4CED-9FCB-F1FD31B0666C
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (POL: polskie filtry do Adblocka i uBlocka)",
        "paoecjnjjbclkgbempaeemcbeldldlbo",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsUqWP6CeMx79UyZ3GZ1X"
        "cBGexIgml00sB286wZ7dJsfqG7oI0EGRoqrDeRreYcOTl+HvXsRJvR1FfkKJzD5s"
        "vdhR4mn4lI+FXUDCvgEZ9CFa0YfASuoTIrdZtG74Twu2ai52ZJzrQ9ike97bdwzu"
        "Zo+uymw26S+5/+IQbriIYoxEbJd7EryZuo+W65LdSat/NOKKf1QnVTIOoqMrXiew"
        "RYywnmZATfDIi0uKXuQfF15lbNBkQllmPH1xlMkz2WnvSvqI4HKPAmEFJWVUkiNh"
        "GKFZkTk1+88CgGGPVsKllxLaDOD+j8Kb0+h44RxObHTF/vFkfh8FfzujFj3Htevj"
        "CQIDAQAB",
    }),
    // 1088D292-2369-4D40-9BDF-C7DC03C05966
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (RUS: Adguard Russian Filter)",
        "dmoefgliihlcfplldbllllbofegmojne",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4p+4n2wNFQCqQBBJDsvs"
        "+oqNYGzX3cbpY7fKbCjrRVE5esJK5HZJDoUUg43pPvKrCOIQ+lF+dXpBaCNnO4O/"
        "7JeFt2IFRJnKhE3ipIBAAbFymfo5T2uWFdyh6HcK0FNyJ/7FyHnANe7vYhXJS1Fq"
        "mh6jTYkAEIbrbmxtzrDMefx3XJcVhUV3XAPlP+K3MerxudIH++4fn3X0vKob5oQQ"
        "Q9ZZ1PVcW6ZdZTQwQWtaVDb6prT+ULaphRRmnZpZuRXyHMv9KC8YP3K5ou+/Yd3u"
        "xxMwKmJXD67ZoNMtS/Dtr0btQsLxiEgox5Swd4iqyLM/SMxr3LqgUIlNwn7KRbMn"
        "ZwIDAQAB",
    }),
    // DABC6490-70E5-46DD-8BE2-358FB9A37C85
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (RUS: BitBlock List)",
        "fmcofgdkijoanfaodpdfjipdgnjbiolk",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEApGBuuxC9pqx2s5hUd3K8"
        "zeFUtMFt9X+rSKR3elqWztjIQrbOdXSeezHvhAUdzgc+Wv79ZoRf4i4amYs3Mg3w"
        "g783BAqLvlu9r6FsUAbcgVQtt+MT3Z4ZepwvzWU0NjUd1q4O2pNEUsE8SPjmOeb3"
        "KHOF5WX7CA1uIHT5xGQsU5Uh3VTZC8FIOGjCskDAAnJGUeOowlMBGL2UvlNQLiqz"
        "PSvI9byjwxIMN5OfCmxXXr4R9m88oVK2D1gj7vfwBVJcRdV8ner4ZSuT68ncSyaQ"
        "RtgI3/QyHc0J6giCRFmF0bHN/5kjFIWrHg5+uiBQN4Qt39TVCUU024Fi2RGInvTT"
        "dQIDAQAB",
    }),
    // 80470EEC-970F-4F2C-BF6B-4810520C72E6
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (RUS: RU AdList)",
        "enkheaiicpeffbfgjiklngbpkilnbkoi",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArVVgKRE868yup0HfX4+H"
        "yZmJVIk33AKivwvRRfjHRxeC+lLnRjNiY0LKS/K65J6SNLgUsZGfT5u4h4F423O/"
        "pbZl6zdfs5kOyStlmLPXhFtF/bIXIsUtdJ0R3dEz+nSg0C2L/FnE5Qr8M4thdmq/"
        "DIP1C70mj8pCnX1939hXyR0ymQkYp573O+LJ0q1L41jBqHzNKWngfBc79I2Kbt1p"
        "LluBT2X7zZVbb+1ap3Ad/VMeFDB2yurRs88cYJZOal7mgTgI/Zkuzsh2Dnql5+UN"
        "OCHinYjcOvUifGgkdsJIJxL57PxRzbriLCNjShoOV3Fpc0XYL1KSWvIVuW0bYeLm"
        "rwIDAQAB",
    }),
    // AE657374-1851-4DC4-892B-9212B13B15A7
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (SPA: EasyList Spanish)",
        "pdecoifadfkklajdlmndjpkhabpklldh",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2eGyWcTM6Cpmkw6CBBxQ"
        "bJCgp3Q4jyh+JR/Aqq5G+OFzxFpwlqW0dH9kNuUs30iSt1tt1gMZGYnhPKiGhtX3"
        "nV1iYg2K8k82wNqA5+ODfHxnnVn536UoC7rmjXL+mhpymxgkjGCQ+1HVmnCcSC9m"
        "xTPy65ihor+YZcRRPo0IhjQTx3NgdpzkGYvpQVjwnw3a5FpRBCbbp3X2x3EGV3Dc"
        "jvT6DvvxSU/mAUPlXISo9OFHYUpADilqAevXQIs49LSmefSDu4pezGyR/JoRLh7Q"
        "R4N3fC17V2E0GazWxvn2U985hPE3tvFcH+LM3EypVRCl6E9AiUZCeumqMBffyXw1"
        "AwIDAQAB",
    }),
    // 418D293D-72A8-4A28-8718-A1EE40A45AAF
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (SVN: Slovenian List)",
        "lddghfaofadfpaajgncgkbjhalgohfkd",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4cRSF2Rg5SSG6mwE6NQC"
        "nX4a0MfzC9URqNFnI4Wf3d3a7CkhmVeNZHxSCGGLxNV9SjCi5tko7NdMqIwnN/vl"
        "iZV+jnEDi8Lj+zz9nftkaGXe3jNoP7tr/+Qkqphc76j3wIpsQx/vBnfVTn5lrNyn"
        "ZL6qpFzX5dj4ukdJ6BOx1YTNdJV9LOyMWbC5rno1mpd14aS7R2T6xfnm3+nupaZM"
        "AbUeN/1bwxDdND/mbjFzFvkPCC+4m758tI/5kSJOefy8kNvp9BM64LXPA4sF59tt"
        "JtCIOJDAyhM1P0Danyze2g/0GGnojDuzZilfeSCeEpDsc+S78Tyqz/lMtxt2LZkv"
        "oQIDAQAB",
    }),
    // 7DC2AC80-5BBC-49B8-B473-A31A1145CAC1
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (SWE: Frellwit's Filter List)",
        "oimfmeehpinnecjghphifehbbnddjkmf",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA17Vf1qj8dWwYVtGpBHWc"
        "9gLiITU1XrTnb1sDASIeuKYp9JNBtEnBwy4oBlOoZd2uWFKxXrRtaimdwqa627gi"
        "9DB17t/RgzisXSpLubXbVVelRWllaX26SioGxsGcQhS2/e1Bc0inQ8GODM6mk5FP"
        "Z9RObFN1N/QVz35anN4VNcjtETD/XpujYXE1BU3C0KGBlWwc+cQZ6sGojWEPrb7a"
        "RXSTJ5y/ugwGomTTpbT+Jt9nFrMfuAmJHvWS0Ev96dDmn1zsuoPGUExVFjGBunph"
        "RYMVCg9LUGzY0FN5+dp6fljrTJrtUOEfvh40vmjahKd0w6bKpgTAOUEaWulmVSr3"
        "7QIDAQAB",
    }),
    // 1BE19EFD-9191-4560-878E-30ECA72B5B3C
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (TUR: Adguard Turkish Filter)",
        "oooemoeokehlgldpjjhcgbndjcekllim",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2We4hmp3TwsrKyOb6rF/"
        "mCjy9TW3w9n9CD1rZMXUF3U6CCgxH4lps5HiLlxUFaIhhcUEXrGlXbk4TE2LlTv4"
        "VS53O23YixZXQ/xMmpWSyBvc3/jBCrAAcvDLAZY53J1T/9t7DNZdpXkX3rNpYB4L"
        "5/5dyzQI+sZZoTBe5dLyJOR1uDZJphpXRWSKqBRLn4SJ5uOGgtqG5J4rMhB+SUrN"
        "hWs8AyM8+tdoaxOjx7n+PA2Rx7/foty1Bbd7Hfc1Eg0C9R40inJNgH+IDxZ07ZFq"
        "iAuY1Z16lr4bwunk7ft4tTafci0M2t86JkoH0B4yiTBKthB6AkmZ0/dejeQeOBsz"
        "YQIDAQAB",
    }),
    // 6A0209AC-9869-4FD6-A9DF-039B4200D52C
    AdBlockRegionalUpdater({
        "Brave Ad Block Updater (VIE: Fanboy's Vietnamese)",
        "cklgijeopkpaadeipkhdaodemoenlene",
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAymFhKEG/UJ8ZyKjdx4xf"
        "RFtECXdWXixG8GoS3mrw/haeVQoB1jXmPBQTZfL2WGZqYvrAkHRRel7XEoZNYziP"
        "3bCYbS4yVqKnDUp1u5GIsMsN0Pff1O1SHEbqClb79vAVhftNq1VQkHPpXQdoSiIN"
        "Q12Om8WbOIuaNxkrTToFW7XRMtbI3tluoLUSy9YTkCEGah68Dl1uL6nOzOxaMV1i"
        "QRRk5Pw4ugTzwGHHL2U2kDYDNrlywK8cUIFgtZskqQ/TF1zF6u9xTGjwjB9X319X"
        "rTg2llcojCgj/dllBuXL2aJoDsS3qAVzqbSYxIE6bQU8JX8wv+KCDMpJt/dHPQqO"
        "MwIDAQAB",
    }),
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_UPDATER_LIST_H_
