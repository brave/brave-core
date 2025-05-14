// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Shared
import os.log

typealias FavoriteSite = (url: URL, title: String)

struct FavoritesPreloadedData {
  static let youtube = FavoriteSite(
    url: URL(string: "https://www.youtube.com/")!,
    title: "YouTube"
  )
  static let wikipedia = FavoriteSite(
    url: URL(string: "https://www.wikipedia.org/")!,
    title: "Wikipedia"
  )
  static let facebook = FavoriteSite(
    url: URL(string: "https://www.facebook.com/")!,
    title: "Facebook"
  )
  static let brave = FavoriteSite(
    url: URL(string: "https://brave.com/whats-new/")!,
    title: "What's new in Brave"
  )
  static let popularFavorites = [youtube, wikipedia, facebook, brave]

  /// Returns a list of websites that should be preloaded for specific region. Currently all users get the same websites.
  // swift-format-ignore
  static func getList() -> [FavoriteSite] {
    let region = Locale.current.region?.identifier ?? ""
    Logger.module.debug("Preloading favorites, current region: \(region)")

    switch region {
    case "AX", "AL", "DZ", "AD", "AO", "AI", "AQ", "AG", "AM", "AW", "AZ", "BS", "BH", "BD", "BB",
      "BY", "BZ", "BJ", "BM", "BT", "BO", "BA", "BW", "BV", "IO", "BN", "BG", "BF", "BI", "KH",
      "CM", "CV", "KY", "CF", "TD", "CN", "CX", "CC", "KM", "CG", "CD", "CK", "CR", "CI", "CU",
      "CW", "CY", "DJ", "DM", "DO", "EC", "SV", "GQ", "ER", "ET", "FK", "FO", "FJ", "GF", "PF",
      "TF", "GA", "GM", "GE", "GH", "GI", "GL", "GD", "GP", "GU", "GT", "GG", "GN", "GW", "GY",
      "HT", "HM", "VA", "HN", "IN", "ID", "IR", "IQ", "IE", "IM", "IL", "JM", "JE", "JO", "KZ",
      "KE", "KI", "KP", "XK", "KW", "KG", "LA", "LB", "LS", "LR", "LY", "LI", "LU", "MO", "MK",
      "MW", "MY", "MV", "ML", "MT", "MH", "MQ", "MR", "MU", "YT", "MX", "FM", "MD", "MC", "MN",
      "ME", "MS", "MA", "MZ", "MM", "NA", "NR", "NP", "AN", "NC", "NI", "NE", "NG", "NU", "NF",
      "MP", "NO", "OM", "PK", "PW", "PS", "PA", "PG", "PY", "PE", "PH", "PN", "PL", "PT", "QA",
      "RE", "RU", "RW", "SH", "KN", "LC", "PM", "VC", "BL", "MF", "WS", "SM", "ST", "SA", "SN",
      "RS", "SC", "SL", "SG", "SX", "SK", "SI", "SB", "SO", "ZA", "GS", "SS", "LK", "SD", "SR",
      "SJ", "SZ", "SE", "CH", "SY", "TW", "TJ", "TZ", "TH", "TL", "TG", "TK", "TO", "TT", "TN",
      "TR", "TM", "TC", "TV", "UG", "UA", "AE", "UY", "UM", "UZ", "VU", "VE", "VN", "VI", "WF",
      "EH", "YE", "ZM", "ZW":
      return popularFavorites
    case "AF", "AT", "EE", "FI":
      var sites = [youtube, wikipedia, facebook]
      if region == "AF" {
        if let url = URL(string: "https://www.sporcle.com/") {
          sites.append(FavoriteSite(url, title: "Sporcle"))
        }
      } else if region == "AT" {
        if let url = URL(string: "https://orf.at/") {
          sites.append(FavoriteSite(url, title: "Orf"))
        }
      } else if region == "EE" {
        if let url = URL(string: "https://ekspress.delfi.ee//") {
          sites.append(FavoriteSite(url, title: "Eesti Ekspress"))
        }
      } else if region == "FI" {
        if let url = URL(string: "https://www.iltalehti.fi/") {
          sites.append(FavoriteSite(url, title: "Iltalehti"))
        }
        if let url = URL(string: "https://yle.fi/") {
          sites.append(FavoriteSite(url, title: "Yle"))
        }
      }
      return sites
    case "AS":
      var sites: [FavoriteSite] = []
      if let url = URL(string: "https://www.samoanews.com/") {
        sites.append(FavoriteSite(url, title: "Samoan News"))
      }
      if let url = URL(string: "https://forecast.weather.gov/MapClick.php?lat=-14.30068805999997&lon=-170.71811612199997") {
        sites.append(FavoriteSite(url, "Weather.gov"))
      }
      sites.append(contentsOf: [wikipedia, youtube])
      return sites
    case "AR":
      var sites: [FavoriteSite] = []
      if let url = URL(string: "https://www.infobae.com/america/") {
        sites.append(FavoriteSite(url, title: "infobae"))
      }
      sites.append(contentsOf: [wikipedia, facebook])
      if let url = URL(string: "https://www.mercadolibre.com.ar/") {
        sites.append(FavoriteSite(url, title: "Mercado Libre"))
      }
      return sites
    case "AU":
      var sites: [FavoriteSite] = []
      if let url = URL(string: "https://www.bom.gov.au/") {
        sites.append(FavoriteSite(url, title: "BOM"))
      }
      sites.append(youtube)
      if let url = URL(string: "https://www.swellnet.com/") {
        sites.append(FavoriteSite(url, title: "Swellnet"))
      }
      sites.append(wikipedia)
      if let url = URL(string: "https://cricket.com.au/") {
        sites.append(FavoriteSite(url, title: "Cricket.com.au"))
      }
      return sites
    case "BE", "BR", "CL", "CO", "CZ", "HR", "DK", "EG":
      var sites: [FavoriteSite] = [youtube, wikipedia]
      if region == "BE" {
        if let url = URL(string: "https://www.vrt.be/nl") {
          sites.append(FavoriteSite(url, title: "VRT"))
        }
        if let url = URL(string: "https://www.hln.be/") {
          sites.append(FavoriteSite(url, title: "HLN"))
        }
      } else if region == "BR" {
        if let url = URL(string: "https://www.uol.com.br/") {
          sites.append(FavoriteSite(url, title: "UOL"))
        }
        if let url = URL(string: "https://www.globo.com/") {
          sites.append(FavoriteSite(url, title: "Globo"))
        }
      } else if region == "CL" {
        if let url = URL(string: "https://www.emol.com/") {
          sites.append(FavoriteSite(url, title: "Emol"))
        }
        if let url = URL(string: "https://www.biobiochile.cl/") {
          sites.append(FavoriteSite(url, title: "BiobioChile"))
        }
      } else if region == "CO" {
        if let url = URL(string: "https://www.eltiempo.com/") {
          sites.append(FavoriteSite(url, title: "El Tiempo"))
        }
        if let url = URL(string: "https://www.semana.com/") {
          sites.append(FavoriteSite(url, title: "Semana"))
        }
      } else if region == "CZ" {
        if let url = URL(string: "https://www.novinky.cz/") {
          sites.append(FavoriteSite(url, title: "Novinky"))
        }
        if let url = URL(string: "https://www.idnes.cz/") {
          sites.append(FavoriteSite(url, title: "iDNES.cz"))
        }
        if let url = URL(string: "https://www.centrum.cz/") {
          sites.append(FavoriteSite(url, title: "Centrum.cz"))
        }
      } else if region == "HR" {
        if let url = URL(string: "https://www.jutarnji.hr/") {
          sites.append(FavoriteSite(url, title: "Jutarnji list"))
        }
        if let url = URL(string: "https://www.index.hr/") {
          sites.append(FavoriteSite(url, title: "Index.HR"))
        }
        if let url = URL(string: "https://www.24sata.hr/") {
          sites.append(FavoriteSite(url, title: "24 Sata"))
        }
      } else if region == "DK" {
        if let url = URL(string: "https://www.berlingske.dk/") {
          sites.append(FavoriteSite(url, title: "Berlingske"))
        }
        if let url = URL(string: "https://politiken.dk/") {
          sites.append(FavoriteSite(url, title: "Politiken"))
        }
      } else if region == "EG" {
        if let url = URL(string: "https://www.youm7.com/") {
          sites.append(FavoriteSite(url, title: "اليوم السابع"))
        }
        if let url = URL(string: "https://www.almasryalyoum.com/") {
          sites.append(FavoriteSite(url, title: "المصري اليوم"))
        }
        if let url = URL(string: "https://www.egyptindependent.com/") {
          sites.append(FavoriteSite(url, title: "Egyptian Independent"))
        }
        if let url = URL(string: "https://www.madamasr.com/en/") {
          sites.append(FavoriteSite(url, title: "Madamasr"))
        }
      }
      return sites
    case "VG":
      var sites: [FavoriteSite] = []
      if let url = URL(string: "https://www.bbc.com/") {
        sites.append(FavoriteSite(url, title: "The BBC"))
      }
      if let url = URL(string: "https://www.accuweather.com/en/vg/british-virgin-islands-weather") {
        sites.append(FavoriteSite(url, title: "AccuWeather"))
      }
      sites.append(contentsOf: [wikipedia, youtube])
      return sites
    case "CA":
      var sites: [FavoriteSite] = []
      if let url = URL(string: "https://www.theglobeandmail.com/") {
        sites.append(FavoriteSite(url, title: "The Globe and Mail"))
      }
      if let url = URL(string: "https://www.accuweather.com/") {
        sites.append(FavoriteSite(url, title: "AccuWeather"))
      }
      sites.append(contentsOf: [wikipedia, youtube])
      return sites
    case "FR":
      var sites: [FavoriteSite] = [youtube, facebook, wikipedia]
      if let url = URL(string: "https://www.amazon.fr/") {
        sites.append(FavoriteSite(url, title: "Amazon"))
      }
      if let url = URL(string: "https://www.lemonde.fr/") {
        sites.append(FavoriteSite(url, title: "Le Monde"))
      }
      return sites
    case "DE":
      var sites: [FavoriteSite] = [youtube]
      if let url = URL(string: "https://www.spiegel.de/") {
        sites.append(FavoriteSite(url, title: "Der Spiegel"))
      }
      sites.append(contentsOf: [wikipedia, brave])
      return sites
    case "GR":
      var sites: [FavoriteSite] = [youtube, wikipedia, facebook]
      if let url = URL(string: "https://www.kathimerini.gr/") {
        sites.append(FavoriteSite(url, title: "Η Καθημερινή"))
      }
      if let url = URL(string: "hhttps://www.naftemporiki.gr/") {
        sites.append(FavoriteSite(url, title: "Η Ναυτεμπορική"))
      }
      return sites
    case "HK":
      var sites: [FavoriteSite] = [youtube, wikipedia]
      if let url = URL(string: "https://www.scmp.com/") {
        sites.append(FavoriteSite(url, title: "SCMP"))
      }
      if let url = URL(string: "https://www.asiasentinel.com/") {
        sites.append(FavoriteSite(url, title: "Asia Sentinel"))
      }
      return sites
    case "HU", "IS", "IT", "LV", "LT", "MG", "NL":
      var sites: [FavoriteSite] = [youtube, wikipedia, facebook]
      if region == "HU" {
        if let url = URL(string: "https://european-union.europa.eu/news-and-events_en?prefLang=hu") {
          sites.append(FavoriteSite(url, title: "SCMP"))
        }
        if let url = URL(string: "https://www.asiasentinel.com/") {
          sites.append(FavoriteSite(url, title: "Asia Sentinel"))
        }
      } else if region == "IS" {
        if let url = URL(string: "https://european-union.europa.eu/news-and-events_en") {
          sites.append(FavoriteSite(url, title: "EU"))
        }
      } else if region == "IT" {
        if let url = URL(string: "https://european-union.europa.eu/news-and-events_en?prefLang=it") {
          sites.append(FavoriteSite(url, title: "EU"))
        }
      } else if region == "LV" {
        if let url = URL(string: "https://european-union.europa.eu/news-and-events_en?prefLang=lv") {
          sites.append(FavoriteSite(url, title: "EU"))
        }
      } else if region == "LT" {
        if let url = URL(string: "https://european-union.europa.eu/news-and-events_en?prefLang=lt") {
          sites.append(FavoriteSite(url, title: "EU"))
        }
      } else if region == "MG" {
        if let url = URL(string: "https://www.sporcle.com/") {
          sites.append(FavoriteSite(url, title: "Sporcle"))
        }
      } else if region == "NL" {
        if let url = URL(string: "https://european-union.europa.eu/news-and-events_en?prefLang=nl") {
          sites.append(FavoriteSite(url, title: "EU"))
        }
      }
      return sites
    case "JP":
      var sites: [FavoriteSite] = [youtube]
      if let url = URL(string: "https://yahoo.co.jp/") {
        sites.append(FavoriteSite(url, title: "Yahoo! JAPAN"))
      }
      if let url = URL(string: "https://brave.com/ja/ntp-tutorial/") {
        sites.append(FavoriteSite(url, title: "Braveガイド"))
      }
      if let url = URL(string: "https://x.com/") {
        sites.append(FavoriteSite(url, title: "X"))
      }
      return sites
    case "KR":
      var sites: [FavoriteSite] = [youtube]
      if let url = URL(string: "https://www.ilbe.com/") {
        sites.append(FavoriteSite(url, title: "Ilbe"))
      }
      sites.append(wikipedia)
      if let url = URL(string: "https://www.naver.com/") {
        sites.append(FavoriteSite(url, title: "Naver"))
      }
      if let url = URL(string: "https://www.coupang.com/") {
        sites.append(FavoriteSite(url, title: "Coupang"))
      }
      if let url = URL(string: "https://www.kakaocorp.com/page/") {
        sites.append(FavoriteSite(url, title: "Kakao"))
      }
      if let url = URL(string: "https://www.daum.net/") {
        sites.append(FavoriteSite(url, title: "Daum"))
      }
      return sites
    case "NZ":
      var sites: [FavoriteSite] = [youtube, wikipedia]
      if let url = URL(string: "https://www.teaonews.co.nz/") {
        sites.append(FavoriteSite(url, title: "Te Ao Māori News"))
      }
      if let url = URL(string: "https://www.nzherald.co.nz/") {
        sites.append(FavoriteSite(url, title: "The NZ Herald"))
      }
      if let url = URL(string: "https://www.rnz.co.nz/news/te-manu-korihi") {
        sites.append(FavoriteSite(url, title: "RNZ News"))
      }
      if let url = URL(string: "https://www.seek.co.nz/") {
        sites.append(FavoriteSite(url, title: "Seek"))
      }
      return sites
    case "PR":
      var sites: [FavoriteSite] = [youtube]
      if let url = URL(string: "https://forecast.weather.gov/MapClick.php?lat=18.22260649800006&lon=-66.46895343099999") {
        sites.append(FavoriteSite(url, title: "Weather.gov"))
      }
      if let url = URL(string: "https://www.elnuevodia.com/negocios/consumo/notas/uber-le-pone-fecha-a-su-lanzamiento-para-puerto-rico/") {
        sites.append(FavoriteSite(url, title: "El Nuevo Día"))
      }
      if let url = URL(string: "https://humanosdepuertorico.tumblr.com/") {
        sites.append(FavoriteSite(url, title: "Humanos de PR"))
      }
      sites.append(contentsOf: [wikipedia, youtube])
      return sites
    case "RO":
      var sites: [FavoriteSite] = [youtube, facebook]
      if let url = URL(string: "https://hotnews.ro/") {
        sites.append(FavoriteSite(url, title: "Hotnews"))
      }
      sites.append(contentsOf: [wikipedia, brave])
      return sites
    case "ES":
      var sites: [FavoriteSite] = [youtube, wikipedia]
      if let url = URL(string: "https://elpais.com/") {
        sites.append(FavoriteSite(url, title: "El País"))
      }
      if let url = URL(string: "https://www.elnacional.cat/") {
        sites.append(FavoriteSite(url, title: "ElNacional.cat"))
      }
      if let url = URL(string: "https://www.berria.eus/") {
        sites.append(FavoriteSite(url, title: "Berria"))
      }
      return sites
    case "GB":
      var sites: [FavoriteSite] = []
      if let url = URL(string: "https://www.bbc.com/") {
        sites.append(FavoriteSite(url, title: "The BBC"))
      }
      sites.append(contentsOf: [wikipedia, youtube, brave])
      return sites
    case "US":
      var sites: [FavoriteSite] = []
      if let url = URL(string: "https://www.reddit.com/") {
        sites.append(FavoriteSite(url, title: "Reddit"))
      }
      if let url = URL(string: "https://www.espn.com/") {
        sites.append(FavoriteSite(url, title: "ESPN"))
      }
      sites.append(contentsOf: [wikipedia, youtube])
      return sites
    default:
      return popularFavorites
    }
  }
}
