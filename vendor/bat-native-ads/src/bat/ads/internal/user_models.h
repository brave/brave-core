/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_USER_MODEL_COMPONENTS_H_
#define BAT_ADS_INTERNAL_USER_MODEL_COMPONENTS_H_

#include <map>
#include <set>
#include <string>

namespace ads {

const std::set<std::string> kPageClassificationUserModelIds = {
  "ijmgabghpbflfadffhpmjklamndnonha",
  "hddanjaanmjbdklklpldpgpmbdmaiihb",
  "blcjdmhlkddhompnlbjlpccpelipihem",
  "pecokcgeeiabdlkfkfjpmfldfhhjlmom",
  "pgkommhmfkkkfbbcnnfhlkagbodoimjm",
  "emopjfcnkbjfedjbfgajdajnkkfekcbl",
  "hfiknbegiiiigegdgpcgekhdlpdmladb",
  "onjbjcnjheabeghbflckfekjnnjgfabn",
  "ghgfdmhmhifphaokjfplffppmlfchofm",
  "mbcmenffnlanegglgpdgbmmldfpclnkm",
  "clemenahenllbgeilmkllcpfcfhloebp",
  "cmggjadifnmfdfeohgomnaggonihmbli",
  "fabdaiigiipinlpgkkakcffbhkmfnfek",
  "cgpdnipmfmbfceokoadgbliclnoeddef",
  "hjfgajachhnlgjfjgbjbehgcoecfigap",
  "keghklelbpkaiogcinbjnnjideedfmdd",
  "lldcgicjgpomllohjncpcjkijjklooji",
  "ogmkkmobemflinkielcngdanaojkamcc",
  "ligbalgipjnajannojamijenlackcioc",
  "bkicjkclpdihdjbpajnegckopabcebff",
  "dafhhkffcifanfgpjlgejcahkidpbnfj",
  "ngnfehahlmgkoclalhldboigojplccbl",
  "hkfnnljkbmadknefddjfligobbiagmea",
  "igdpeoeohlgknkdnjhbijpfeenfiepdc",
  "mdacdgffhlidpdmhckokeoajhojeibmp",
  "keahfifnfebhoaaghffigjnhgbkkaibd",
  "fllhkadnpidionapphjicdkfdiloghad",
  "eakppbikiomjdnligoccikcdipojiljk",
  "ddekfkhnjcpbankekbelkeekibbhmgnh",
  "oblfikajhadjnmjiihdchdfdcfehlbpj",
  "obdagonejiaflgbifdloghkllgdjpcdj",
  "apmkijnjnhdabkckmkkejgdnbgdglocb",
  "gdmbpbmoiogajaogpajfhonmlepcdokn",
  "amedpgedagedjlkgjajebgecfkldmdfa",
  "ncjeinjdknabbgmaijmnipndbggmchff",
  "nllaodpgkekajbabhjphhekenlnlpdmd",
  "klifniioldbebiedppmbobpdiombacge",
  "aoljgchlinejchjbbkicamhfdapghahp",
  "opoleacilplnkhobipjcihpdoklpnjkk",
  "jginkacioplimdjobccplmgiphpjjigl",
  "emgmepnebbddgnkhfmhdhmjifkglkamo",
  "halbpcgpgcafebkldcfhllomekophick",
  "onmakacikbbnhmanodpjhijljadlpbda",
  "bjhkalknmdllcnkcjdjncplkbbeigklb",
  "jamflccjbegjmghgaljipcjjbipgojbn",
  "gfoibbmiaikelajlipoffiklghlbapjf",
  "djokgcimofealcnfijnlfdnfajpdjcfg",
  "hbejpnagkgeeohiojniljejpdpojmfdp",
  "anhpkncejedojfopbigplmbfmbhkagao",
  "ejioajnkmjfjfbbanmgfbagjbdlfodge",
  "hlipecdabdcghhdkhfmhiclaobjjmhbo",
  "eclclcmhpefndfimkgjknaenojpdffjp",
  "aefhgfnampgebnpchhfkaoaiijpmhcca",
  "ebgmbleidclecpicaccgpjdhndholiel",
  "mdpcebhdjplkegddkiodmbjcmlnhbpog",
  "hpkelamfnimiapfbeeelpfkhkgfoejil",
  "khgpojhhoikmhodflkppdcakhbkaojpi",
  "gffjpkbdngpbfkflpnoodjfkpelbappk",
  "pkmkhkpglegjekkfabaelkbgkfegnmde",
  "emhbebmifclalgbdpodobmckfehlkhfp",
  "cmkgcbedakcdopgjdhbbpjjaodjcdbdp",
  "ifbdofecjcadpnkokmdfahhjadcppmko",
  "hoeihggfhgnfpdnaeocfpmoelcenhfla",
  "gbmolmcnbhegkhjhbhjehcpbjonjlgfg",
  "fioklomfllflofcdiklpgabemclgkkdh",
  "oiihbhoknlbjonghmcplpbpkcdeinlfg",
  "nchaailfhkbnlnaobgjmoamdfnclhdoo",
  "fbdjobfnceggaokdnlebbaplnhednlhl",
  "nkajjfpapgfhlcpmmoipafbfnnmojaep",
  "dhhkjdedjghadoekfklpheblplmlpdec",
  "ijaiihoedhaocihjjkfjnhfhbceekdkg",
  "eociikjclgmjinkgeoehleofopogehja",
  "ncnmgkcadooabjhgjlkkdipdnfokpjnm",
  "jdfafcdnmjeadcohbmjeeijgheobldng",
  "jknflnmanienedkoeoginjmbpmkpclki",
  "nggdckgfnooehkpnmjdodbknekmhcdeg",
  "mhobnohaonkecggnifnffofnihbakjic",
  "hhejjhncnckfmpkpkhkbknekhkepcill",
  "ljllkgialkdmamlacenmjkhjmimndfil",
  "dhigelnhdnmhffomicjdmdhefhedgijm",
  "jcfelbpkigpapilbogpidbfdffgdfafe",
  "ookcfboifajbeojnnebiaoplifgiinof",
  "njimokpabbaelbbpohoflcbjhdgonbmf",
  "danmahhfdmncbiipbefmjdkembceajdk",
  "lcahllbcfbhghpjjocdhmilokfbpbekn",
  "jbhiacghlejpbieldkdfkgenhnolndlf",
  "hfboaaehpnfpnpompagbamoknlnladfn",
  "cppbcboljlmfdgeehadijemhkifhcpnl",
  "knnpciecjoakhokllbgioaceglldlgan",
  "chnbfebpflegknnjiikofmnebcbphead",
  "hkfkdejbdionnjdhgfhpkcmknmamddde",
  "nnbaaidlgckbmfdlnioepikbcmjmbadb",
  "dedchpogdooaakplmpjobpkiggalnlif",
  "alaghdpgppakjfjcipokbaempndnglfk",
  "copfpkggfedomijbmceepmahananponb",
  "ljambfnmibabkhcpgppfblodipceahab",
  "lflklgkjbemnncejeanindnikiaicpod",
  "lkcfaihllkinjhmdjgaccjhgdobabpbj",
  "anikcbbbhcobgockdcemaboadbdcnhlg",
  "gaompbafbaolhddgoafjhkgmnjpjpbip",
  "pppjaeenalohmnhjpemhdkkkghfddbfp",
  "papnfbjjblebaaeenodbmllepiijfmhn",
  "jmakhmnfhlhioagnmgnakhigadgkpkcl",
  "gakleannelcniohpkolmbgfpnkjlbnll",
  "lmpaafjmphnclpkfcejgjbnieahkgemg",
  "fehmnfinbijjdacjgeofhojfdjhbehic",
  "aadlcogkfhaficfijoolhlajkngeecea",
  "edcpinkoiknmjafcdpolkkeiieddmbab",
  "ooncphbdmekmhldbojgoboebnongkpic",
  "kemfolmmdooeepfhbpiemnnekfjlbnnd",
  "mihcpmkclenpfgielcipmdbcfpncfojc",
  "jpleaamlgnfhfemdcfmbknnhcbfnglkh",
  "gbigbbblbdmfhbpobcliehihdedicjfl",
  "fnmjhgcaloniglpngailbaillhbenela",
  "fgicknpghikljlipmfibgghcdikjfjfj",
  "nodfcenkjehpafmbmgnapoieilnoijap",
  "dghnlalnaoekcligakadcmmioaieangj",
  "fbfeebiglbpbmgbefgmijdbcchmdfdhm",
  "gdkeabmpgllapbjgifgfmlfelpdlkapj",
  "fnhldinjahkdbngcnjfcmidhpjedinbg",
  "aegokocmijocdgiddgjbjkdfiheijfpl",
  "amkpggbpieoafkbmkijjnefikhjjfogn",
  "adccmiokahgjhdbmhldbnkkplgcgfkpp",
  "ghikcfknmlkdjiiljfpgpmcfjinpollk",
  "hinecgnhkigghpncjnflaokadaclcfpm",
  "blaocfojieebnkolagngecdhnipocicj",
  "aijecnhpjljblhnogamehknbmljlbfgn",
  "fikmpfipjepnnhiolongfjimedlhaemk",
  "ikpplkdenofcphgejneekjmhepajgopf",
  "ndlciiakjgfcefimfjlfcjgohlgidpnc",
  "nlabdknfkecjaechkekdlkgnapljpfla",
  "piebpodmljdloocefikhekfjajloneij",
  "hffipkehifobjlkdjagndofmpjnpkgje",
  "nigmjcnboijpcoikglccmoncigioojpa",
  "inkmdnaeojfdngbdkbinoinflfahcjfc",
  "golaphmkhjkdmcakpigbjhneagiapkfh",
  "kcmiboiihhehobbffjebhgadbalknboh",
  "cmomlghkjichjgbkakaoenfenincefnj",
  "mfaajikelgfodgcgnldapbgjdbncmibc",
  "gndfhmmkadfdhmchhljmcdhlicdmmlbn",
  "pdgppejghdoknllcnfikoaloobopajjo",
  "djmefhmnkffabdodgcfjmgffpindaaak",
  "bdepmnbdekgdgjimffimkfeoggmnlbbf",
  "mogllbjhcnfhcejalaikleeogjmmfkdm",
  "gnhdcgmlejfbcccdjdhjalacjcimlkjh",
  "jifgjineejhedlmjnkcijoincbhelicp",
  "doclofgiadjildnifgkajdlpngijbpop",
  "mgdaglmilmjenimbkdmneckfbphfllic",
  "ahiocclicnhmiobhocikfdamfccbehhn",
  "aondicpcneldjpbfemaimbpomgaepjhg",
  "ccmmjlklhnoojaganaecljeecenhafok",
  "khbhchcpljcejlmijghlabpcmlkkfnid",
  "jpgndiehmchkacbfggdgkoohioocdhbp",
  "nbmbpelgpalcgdghkeafabljjbhmalhf",
  "nonmahhknjgpnoamcdihefcbpdioimbh",
  "olopfkdcklijkianjbegdegilmhdgpcj",
  "jllmphacilbjnfngcojfgmiimipclfbm",
  "hkeoedmbihkkglaobeembiogeofffpop",
  "ijgcgakmmgjaladckdppemjgdnjlcgpo",
  "liddcpbnodlgenmbmmfchepoebgfondk",
  "kcoilhabhhnfdakenmhddnhngngggcmp",
  "gjinficpofcocgaaogaiimhacbfkmjmj",
  "hhliclmbfpdlpkdhmpkleicjnemheeea",
  "kpdcfihnokkbialolpedfamclbdlgopi",
  "nhbpjehmiofogaicflcjhcfdmmkgbohp",
  "mmmembcojnkgfclnogjfeeiicmiijcnk",
  "ldjaelhegioonaebajlgfnhcipdajfeo",
  "fnokionjijmckgggmhogifnocpabdafk",
  "ohcnbpfpchlcjchnljcdjebcjdbneecj",
  "pbegjfplhidokoonohceljofcpbojglg",
  "jaggpekahffhnhhchdkpnigfmdlhenpo",
  "jephmoboccidmbemhjckbcagagijgcef",
  "mbhiljiiffkobikkoechkpeaopagfhep",
  "pbjakpdfjkmcajeobebemnjglbjiniln",
  "bfljdbgfmdjgbomhiaoeoleidbfcmmpn",
  "fmiofedgokpciaklgakbnminmmkocgnd",
  "gpfmbdepojhpjlaelmnlbgginpgnbmfd",
  "mhdpccgjfkfkdbbpapbgcahhknmbdnjn",
  "eahefjeohmofagkliaddkmokbecfhclm",
  "gjigddoamjemfcahionjikmlfijoiecf",
  "jhnklldjooclfmgpkipaemehnngabckf",
  "fjfbodkpnkomodlcanacakhcfmjjgkdf",
  "bncbapkadghlbephbogcmomlecfmdhnb",
  "dhlnknppkgfgehmmipicnlplhjgpnmnh"
};

const std::map<std::string, std::string> kPageClassificationLanguageCodes = {
  {
    "ab", "ijmgabghpbflfadffhpmjklamndnonha"
  },
  {
    "aa", "hddanjaanmjbdklklpldpgpmbdmaiihb"
  },
  {
    "af", "blcjdmhlkddhompnlbjlpccpelipihem"
  },
  {
    "ak", "pecokcgeeiabdlkfkfjpmfldfhhjlmom"
  },
  {
    "sq", "pgkommhmfkkkfbbcnnfhlkagbodoimjm"
  },
  {
    "am", "emopjfcnkbjfedjbfgajdajnkkfekcbl"
  },
  {
    "ar", "hfiknbegiiiigegdgpcgekhdlpdmladb"
  },
  {
    "an", "onjbjcnjheabeghbflckfekjnnjgfabn"
  },
  {
    "hy", "ghgfdmhmhifphaokjfplffppmlfchofm"
  },
  {
    "as", "mbcmenffnlanegglgpdgbmmldfpclnkm"
  },
  {
    "av", "clemenahenllbgeilmkllcpfcfhloebp"
  },
  {
    "ae", "cmggjadifnmfdfeohgomnaggonihmbli"
  },
  {
    "ay", "fabdaiigiipinlpgkkakcffbhkmfnfek"
  },
  {
    "az", "cgpdnipmfmbfceokoadgbliclnoeddef"
  },
  {
    "bm", "hjfgajachhnlgjfjgbjbehgcoecfigap"
  },
  {
    "ba", "keghklelbpkaiogcinbjnnjideedfmdd"
  },
  {
    "eu", "lldcgicjgpomllohjncpcjkijjklooji"
  },
  {
    "be", "ogmkkmobemflinkielcngdanaojkamcc"
  },
  {
    "bn", "ligbalgipjnajannojamijenlackcioc"
  },
  {
    "bh", "bkicjkclpdihdjbpajnegckopabcebff"
  },
  {
    "bi", "dafhhkffcifanfgpjlgejcahkidpbnfj"
  },
  {
    "bs", "ngnfehahlmgkoclalhldboigojplccbl"
  },
  {
    "br", "hkfnnljkbmadknefddjfligobbiagmea"
  },
  {
    "bg", "igdpeoeohlgknkdnjhbijpfeenfiepdc"
  },
  {
    "my", "mdacdgffhlidpdmhckokeoajhojeibmp"
  },
  {
    "ca", "keahfifnfebhoaaghffigjnhgbkkaibd"
  },
  {
    "ch", "fllhkadnpidionapphjicdkfdiloghad"
  },
  {
    "ce", "eakppbikiomjdnligoccikcdipojiljk"
  },
  {
    "ny", "ddekfkhnjcpbankekbelkeekibbhmgnh"
  },
  {
    "zh", "oblfikajhadjnmjiihdchdfdcfehlbpj"
  },
  {
    "cv", "obdagonejiaflgbifdloghkllgdjpcdj"
  },
  {
    "kw", "apmkijnjnhdabkckmkkejgdnbgdglocb"
  },
  {
    "co", "gdmbpbmoiogajaogpajfhonmlepcdokn"
  },
  {
    "cr", "amedpgedagedjlkgjajebgecfkldmdfa"
  },
  {
    "hr", "ncjeinjdknabbgmaijmnipndbggmchff"
  },
  {
    "cs", "nllaodpgkekajbabhjphhekenlnlpdmd"
  },
  {
    "da", "klifniioldbebiedppmbobpdiombacge"
  },
  {
    "dv", "aoljgchlinejchjbbkicamhfdapghahp"
  },
  {
    "nl", "opoleacilplnkhobipjcihpdoklpnjkk"
  },
  {
    "dz", "jginkacioplimdjobccplmgiphpjjigl"
  },
  {
    "en", "emgmepnebbddgnkhfmhdhmjifkglkamo"
  },
  {
    "eo", "halbpcgpgcafebkldcfhllomekophick"
  },
  {
    "et", "onmakacikbbnhmanodpjhijljadlpbda"
  },
  {
    "ee", "bjhkalknmdllcnkcjdjncplkbbeigklb"
  },
  {
    "fo", "jamflccjbegjmghgaljipcjjbipgojbn"
  },
  {
    "fj", "gfoibbmiaikelajlipoffiklghlbapjf"
  },
  {
    "fi", "djokgcimofealcnfijnlfdnfajpdjcfg"
  },
  {
    "fr", "hbejpnagkgeeohiojniljejpdpojmfdp"
  },
  {
    "ff", "anhpkncejedojfopbigplmbfmbhkagao"
  },
  {
    "gl", "ejioajnkmjfjfbbanmgfbagjbdlfodge"
  },
  {
    "ka", "hlipecdabdcghhdkhfmhiclaobjjmhbo"
  },
  {
    "de", "eclclcmhpefndfimkgjknaenojpdffjp"
  },
  {
    "el", "aefhgfnampgebnpchhfkaoaiijpmhcca"
  },
  {
    "gn", "ebgmbleidclecpicaccgpjdhndholiel"
  },
  {
    "gu", "mdpcebhdjplkegddkiodmbjcmlnhbpog"
  },
  {
    "ht", "hpkelamfnimiapfbeeelpfkhkgfoejil"
  },
  {
    "ha", "khgpojhhoikmhodflkppdcakhbkaojpi"
  },
  {
    "he", "gffjpkbdngpbfkflpnoodjfkpelbappk"
  },
  {
    "hz", "pkmkhkpglegjekkfabaelkbgkfegnmde"
  },
  {
    "hi", "emhbebmifclalgbdpodobmckfehlkhfp"
  },
  {
    "ho", "cmkgcbedakcdopgjdhbbpjjaodjcdbdp"
  },
  {
    "hu", "ifbdofecjcadpnkokmdfahhjadcppmko"
  },
  {
    "ia", "hoeihggfhgnfpdnaeocfpmoelcenhfla"
  },
  {
    "id", "gbmolmcnbhegkhjhbhjehcpbjonjlgfg"
  },
  {
    "ie", "fioklomfllflofcdiklpgabemclgkkdh"
  },
  {
    "ga", "oiihbhoknlbjonghmcplpbpkcdeinlfg"
  },
  {
    "ig", "nchaailfhkbnlnaobgjmoamdfnclhdoo"
  },
  {
    "ik", "fbdjobfnceggaokdnlebbaplnhednlhl"
  },
  {
    "io", "nkajjfpapgfhlcpmmoipafbfnnmojaep"
  },
  {
    "is", "dhhkjdedjghadoekfklpheblplmlpdec"
  },
  {
    "it", "ijaiihoedhaocihjjkfjnhfhbceekdkg"
  },
  {
    "iu", "eociikjclgmjinkgeoehleofopogehja"
  },
  {
    "ja", "ncnmgkcadooabjhgjlkkdipdnfokpjnm"
  },
  {
    "jv", "jdfafcdnmjeadcohbmjeeijgheobldng"
  },
  {
    "kl", "jknflnmanienedkoeoginjmbpmkpclki"
  },
  {
    "kn", "nggdckgfnooehkpnmjdodbknekmhcdeg"
  },
  {
    "kr", "mhobnohaonkecggnifnffofnihbakjic"
  },
  {
    "ks", "hhejjhncnckfmpkpkhkbknekhkepcill"
  },
  {
    "kk", "ljllkgialkdmamlacenmjkhjmimndfil"
  },
  {
    "km", "dhigelnhdnmhffomicjdmdhefhedgijm"
  },
  {
    "ki", "jcfelbpkigpapilbogpidbfdffgdfafe"
  },
  {
    "rw", "ookcfboifajbeojnnebiaoplifgiinof"
  },
  {
    "ky", "njimokpabbaelbbpohoflcbjhdgonbmf"
  },
  {
    "kv", "danmahhfdmncbiipbefmjdkembceajdk"
  },
  {
    "kg", "lcahllbcfbhghpjjocdhmilokfbpbekn"
  },
  {
    "ko", "jbhiacghlejpbieldkdfkgenhnolndlf"
  },
  {
    "ku", "hfboaaehpnfpnpompagbamoknlnladfn"
  },
  {
    "kj", "cppbcboljlmfdgeehadijemhkifhcpnl"
  },
  {
    "la", "knnpciecjoakhokllbgioaceglldlgan"
  },
  {
    "lb", "chnbfebpflegknnjiikofmnebcbphead"
  },
  {
    "lg", "hkfkdejbdionnjdhgfhpkcmknmamddde"
  },
  {
    "li", "nnbaaidlgckbmfdlnioepikbcmjmbadb"
  },
  {
    "ln", "dedchpogdooaakplmpjobpkiggalnlif"
  },
  {
    "lo", "alaghdpgppakjfjcipokbaempndnglfk"
  },
  {
    "lt", "copfpkggfedomijbmceepmahananponb"
  },
  {
    "lu", "ljambfnmibabkhcpgppfblodipceahab"
  },
  {
    "lv", "lflklgkjbemnncejeanindnikiaicpod"
  },
  {
    "gv", "lkcfaihllkinjhmdjgaccjhgdobabpbj"
  },
  {
    "mk", "anikcbbbhcobgockdcemaboadbdcnhlg"
  },
  {
    "mg", "gaompbafbaolhddgoafjhkgmnjpjpbip"
  },
  {
    "ms", "pppjaeenalohmnhjpemhdkkkghfddbfp"
  },
  {
    "ml", "papnfbjjblebaaeenodbmllepiijfmhn"
  },
  {
    "mt", "jmakhmnfhlhioagnmgnakhigadgkpkcl"
  },
  {
    "mi", "gakleannelcniohpkolmbgfpnkjlbnll"
  },
  {
    "mr", "lmpaafjmphnclpkfcejgjbnieahkgemg"
  },
  {
    "mh", "fehmnfinbijjdacjgeofhojfdjhbehic"
  },
  {
    "mn", "aadlcogkfhaficfijoolhlajkngeecea"
  },
  {
    "na", "edcpinkoiknmjafcdpolkkeiieddmbab"
  },
  {
    "nv", "ooncphbdmekmhldbojgoboebnongkpic"
  },
  {
    "nd", "kemfolmmdooeepfhbpiemnnekfjlbnnd"
  },
  {
    "ne", "mihcpmkclenpfgielcipmdbcfpncfojc"
  },
  {
    "ng", "jpleaamlgnfhfemdcfmbknnhcbfnglkh"
  },
  {
    "nb", "gbigbbblbdmfhbpobcliehihdedicjfl"
  },
  {
    "nn", "fnmjhgcaloniglpngailbaillhbenela"
  },
  {
    "no", "fgicknpghikljlipmfibgghcdikjfjfj"
  },
  {
    "ii", "nodfcenkjehpafmbmgnapoieilnoijap"
  },
  {
    "nr", "dghnlalnaoekcligakadcmmioaieangj"
  },
  {
    "oc", "fbfeebiglbpbmgbefgmijdbcchmdfdhm"
  },
  {
    "oj", "gdkeabmpgllapbjgifgfmlfelpdlkapj"
  },
  {
    "cu", "fnhldinjahkdbngcnjfcmidhpjedinbg"
  },
  {
    "om", "aegokocmijocdgiddgjbjkdfiheijfpl"
  },
  {
    "or", "amkpggbpieoafkbmkijjnefikhjjfogn"
  },
  {
    "os", "adccmiokahgjhdbmhldbnkkplgcgfkpp"
  },
  {
    "pa", "ghikcfknmlkdjiiljfpgpmcfjinpollk"
  },
  {
    "pi", "hinecgnhkigghpncjnflaokadaclcfpm"
  },
  {
    "fa", "blaocfojieebnkolagngecdhnipocicj"
  },
  {
    "pl", "aijecnhpjljblhnogamehknbmljlbfgn"
  },
  {
    "ps", "fikmpfipjepnnhiolongfjimedlhaemk"
  },
  {
    "pt", "ikpplkdenofcphgejneekjmhepajgopf"
  },
  {
    "qu", "ndlciiakjgfcefimfjlfcjgohlgidpnc"
  },
  {
    "rm", "nlabdknfkecjaechkekdlkgnapljpfla"
  },
  {
    "rn", "piebpodmljdloocefikhekfjajloneij"
  },
  {
    "ro", "hffipkehifobjlkdjagndofmpjnpkgje"
  },
  {
    "ru", "nigmjcnboijpcoikglccmoncigioojpa"
  },
  {
    "sa", "inkmdnaeojfdngbdkbinoinflfahcjfc"
  },
  {
    "sc", "golaphmkhjkdmcakpigbjhneagiapkfh"
  },
  {
    "sd", "kcmiboiihhehobbffjebhgadbalknboh"
  },
  {
    "se", "cmomlghkjichjgbkakaoenfenincefnj"
  },
  {
    "sm", "mfaajikelgfodgcgnldapbgjdbncmibc"
  },
  {
    "sg", "gndfhmmkadfdhmchhljmcdhlicdmmlbn"
  },
  {
    "sr", "pdgppejghdoknllcnfikoaloobopajjo"
  },
  {
    "gd", "djmefhmnkffabdodgcfjmgffpindaaak"
  },
  {
    "sn", "bdepmnbdekgdgjimffimkfeoggmnlbbf"
  },
  {
    "si", "mogllbjhcnfhcejalaikleeogjmmfkdm"
  },
  {
    "sk", "gnhdcgmlejfbcccdjdhjalacjcimlkjh"
  },
  {
    "sl", "jifgjineejhedlmjnkcijoincbhelicp"
  },
  {
    "so", "doclofgiadjildnifgkajdlpngijbpop"
  },
  {
    "st", "mgdaglmilmjenimbkdmneckfbphfllic"
  },
  {
    "es", "ahiocclicnhmiobhocikfdamfccbehhn"
  },
  {
    "su", "aondicpcneldjpbfemaimbpomgaepjhg"
  },
  {
    "sw", "ccmmjlklhnoojaganaecljeecenhafok"
  },
  {
    "ss", "khbhchcpljcejlmijghlabpcmlkkfnid"
  },
  {
    "sv", "jpgndiehmchkacbfggdgkoohioocdhbp"
  },
  {
    "ta", "nbmbpelgpalcgdghkeafabljjbhmalhf"
  },
  {
    "te", "nonmahhknjgpnoamcdihefcbpdioimbh"
  },
  {
    "tg", "olopfkdcklijkianjbegdegilmhdgpcj"
  },
  {
    "th", "jllmphacilbjnfngcojfgmiimipclfbm"
  },
  {
    "ti", "hkeoedmbihkkglaobeembiogeofffpop"
  },
  {
    "bo", "ijgcgakmmgjaladckdppemjgdnjlcgpo"
  },
  {
    "tk", "liddcpbnodlgenmbmmfchepoebgfondk"
  },
  {
    "tl", "kcoilhabhhnfdakenmhddnhngngggcmp"
  },
  {
    "tn", "gjinficpofcocgaaogaiimhacbfkmjmj"
  },
  {
    "to", "hhliclmbfpdlpkdhmpkleicjnemheeea"
  },
  {
    "tr", "kpdcfihnokkbialolpedfamclbdlgopi"
  },
  {
    "ts", "nhbpjehmiofogaicflcjhcfdmmkgbohp"
  },
  {
    "tt", "mmmembcojnkgfclnogjfeeiicmiijcnk"
  },
  {
    "tw", "ldjaelhegioonaebajlgfnhcipdajfeo"
  },
  {
    "ty", "fnokionjijmckgggmhogifnocpabdafk"
  },
  {
    "ug", "ohcnbpfpchlcjchnljcdjebcjdbneecj"
  },
  {
    "uk", "pbegjfplhidokoonohceljofcpbojglg"
  },
  {
    "ur", "jaggpekahffhnhhchdkpnigfmdlhenpo"
  },
  {
    "uz", "jephmoboccidmbemhjckbcagagijgcef"
  },
  {
    "ve", "mbhiljiiffkobikkoechkpeaopagfhep"
  },
  {
    "vi", "pbjakpdfjkmcajeobebemnjglbjiniln"
  },
  {
    "vo", "bfljdbgfmdjgbomhiaoeoleidbfcmmpn"
  },
  {
    "wa", "fmiofedgokpciaklgakbnminmmkocgnd"
  },
  {
    "cy", "gpfmbdepojhpjlaelmnlbgginpgnbmfd"
  },
  {
    "wo", "mhdpccgjfkfkdbbpapbgcahhknmbdnjn"
  },
  {
    "fy", "eahefjeohmofagkliaddkmokbecfhclm"
  },
  {
    "xh", "gjigddoamjemfcahionjikmlfijoiecf"
  },
  {
    "yi", "jhnklldjooclfmgpkipaemehnngabckf"
  },
  {
    "yo", "fjfbodkpnkomodlcanacakhcfmjjgkdf"
  },
  {
    "za", "bncbapkadghlbephbogcmomlecfmdhnb"
  },
  {
    "zu", "dhlnknppkgfgehmmipicnlplhjgpnmnh"
  }
};

const std::map<std::string, std::string> kPurchaseIntentCountryCodes = {
  {
    "AF", "jememeholcpjpoahinnlafoiaknnmfgl"
  },
  {
    "AX", "hfonhokmgmjionconfpknjfphfahdklo"
  },
  {
    "AL", "anlkmbkbgleadcacchhgdoecllpllknb"
  },
  {
    "DZ", "imoolhehjnpebcjecoinphmohihmbccj"
  },
  {
    "AS", "kgnhcdjacgcanjnbdcmngdeoncckfmfh"
  },
  {
    "AD", "pmlmnjficamnkblapnohndlnhkkoaoco"
  },
  {
    "AO", "majdffglhcddbbanjnhfocagmmhobghd"
  },
  {
    "AI", "bhdlolcjjefaidodgffjhpbeeapabpgn"
  },
  {
    "AQ", "pbanoihfljabneihobeplbciopfilajn"
  },
  {
    "AG", "cbdjliajiakicmdohhbjbgggacbjdnmn"
  },
  {
    "AR", "enadnicbppimlbpilkeaogmmfpennphn"
  },
  {
    "AM", "cpnhinhihfnhhmoknpbkcifgadjbcjnf"
  },
  {
    "AW", "ocegkjjbmlnibhfjmeiaidplhcbdhomd"
  },
  {
    "AU", "kklfafolbojbonkjgifmmkdmaaimminj"
  },
  {
    "AT", "jmneklmcodckmpipiekkfaokobhkflep"
  },
  {
    "AZ", "llmikniomoddmmghodjfbncbidjlhhid"
  },
  {
    "BS", "aaoipmkakdldlippoaocoegnnfnpcokj"
  },
  {
    "BH", "megoieebjempmobckocciojgfhfmiejb"
  },
  {
    "BD", "ppkgobeickbpfmmkbhfgmgiloepdiagn"
  },
  {
    "BB", "ndfnmlonkimafoabeafnaignianecdhf"
  },
  {
    "BY", "apndmjdcfbhgfccccdmmeofpdgnlbbif"
  },
  {
    "BE", "lnbdfmpjjckjhnmahgdojnfnmdmpebfn"
  },
  {
    "BZ", "demegfhfekncneocdlflandgegpcoofj"
  },
  {
    "BJ", "jiodmgmkikfbkchgenlamoabbfnobnfh"
  },
  {
    "BM", "aeiffmlccgeaacefkfknodmnebanekbo"
  },
  {
    "BT", "hemccombjdaepjnhhdplhiaedaackooa"
  },
  {
    "BO", "dggplmjbjalpdgndkigojpikblaiflke"
  },
  {
    "BQ", "jbibpedjodeigdoimlgpikphaabhdbie"
  },
  {
    "BA", "iefeaaegnnpiadjfoeifapjmflnbjehg"
  },
  {
    "BW", "bfdagcnfmfaidkkjcmfjmogiefoiglem"
  },
  {
    "BV", "kfimhlhdlhimaheficomcahhbaicoele"
  },
  {
    "BR", "fbpmbjccnaaeogogeldlomcmlhllgaje"
  },
  {
    "IO", "cpbmgmnfoakodmmpppabghhckchppneg"
  },
  {
    "BN", "gcikmigghagkligpileoekfjmohokjhm"
  },
  {
    "BG", "ahhbponhjohgifhjbjggafboffbimmmg"
  },
  {
    "BF", "fjgjapaemndekhnfeopeoeajfpmlgemo"
  },
  {
    "BI", "cfbbahiimladdkhblpkkokkmemlmkbhe"
  },
  {
    "CV", "oeneodeckioghmhokkmcbijfanjbanop"
  },
  {
    "KH", "cmknopomfihgdpjlnjhjkmogaooeoeic"
  },
  {
    "CM", "mmiflfidlgoehkhhkeodfdjpbkkjadgi"
  },
  {
    "CA", "gpaihfendegmjoffnpngjjhbipbioknd"
  },
  {
    "KY", "efpgpbmpbkhadlnpdnjigldeebgacopp"
  },
  {
    "CF", "ljfeoddejgcdofgnpgljaeiaemfimgej"
  },
  {
    "TD", "oahnhemdhgogkhgljdkhbficecbplmdf"
  },
  {
    "CL", "gbbfjnnpelockljikcmjkeodlaebjokm"
  },
  {
    "CN", "gfccfpdijajblfnkmddbflphiiibifik"
  },
  {
    "CX", "mjennfbimaafgcoekloojmbhnkophgni"
  },
  {
    "CC", "pnfogoijegjkepejdabehdfadbkpgoed"
  },
  {
    "CO", "cegiaceckhbagmmfcoeebeghiddghbkk"
  },
  {
    "KM", "efcmpmpbmaehimomnmonodlpnghelnfi"
  },
  {
    "CG", "hkgnnbjmfcelmehmphhbjigedknjobaa"
  },
  {
    "CD", "kignebofcmcgmjfiapilgdfkbekmkdmg"
  },
  {
    "CK", "clcghlkineckendfhkgdpkplofknofjo"
  },
  {
    "CR", "hmmoibmjgckbeejmcfflnngeacaklchb"
  },
  {
    "CI", "nopcbglolocphcdeikfkoeppkhijacij"
  },
  {
    "HR", "mjhnpmgafkmildljebajibghemlbffni"
  },
  {
    "CU", "mdopdmalncfakkimlojioichjbebcaip"
  },
  {
    "CW", "boeecnnjahpahhockgdcbdlaimpcflig"
  },
  {
    "CY", "hknminnkgcjafipipbbalkakppehkpjn"
  },
  {
    "CZ", "iejekkikpddbbockoldagmfcdbffomfc"
  },
  {
    "DK", "kmfkbonhconlbieplamnikedgfbggail"
  },
  {
    "DJ", "phihhhnelfclomhodhgalooldjgejgfl"
  },
  {
    "DM", "obihnhimgbeenjgfnlcdbfdgkkgeogdp"
  },
  {
    "DO", "gciaobanmdlfkegiikhgdoogegeghhch"
  },
  {
    "EC", "imnpbmpnmlmkjpgfnfeplikingjklpej"
  },
  {
    "EG", "ojfkdbfibflfejobeodhoepghgoghkii"
  },
  {
    "SV", "adnhangbagjlobdeicimblgcnafegpfb"
  },
  {
    "GQ", "gncihmgakhljdlkadibldhdfnocfplci"
  },
  {
    "ER", "diacfpapelanfbkmehdpaaohmnkkngge"
  },
  {
    "EE", "jdigggiplmjlocdopbdmjibckpamigij"
  },
  {
    "SZ", "npacefioaofgbofilfnhliofkefklipp"
  },
  {
    "ET", "lbiagcddmfapjfbebccolcahfppaimmo"
  },
  {
    "FK", "aogeahmaehgnkobkmnmkhkimdjpgfgen"
  },
  {
    "FO", "molpidmcmbmhbicckmbopbmiojddebke"
  },
  {
    "FJ", "biobhkionbllnfljaapocfpdmhamedkf"
  },
  {
    "FI", "ecneelfoifpgnignhipdebhbkgcphmic"
  },
  {
    "FR", "bgifagoclclhhoflocdefiklgodpihog"
  },
  {
    "GF", "mhmfclafeemiphfebhnobinkplbmpocm"
  },
  {
    "PF", "mjaianjdadeiocokpoanbgjhegficcce"
  },
  {
    "TF", "jbjodokafbafhemocebljdblgnfajabi"
  },
  {
    "GA", "nchncleokkkkdfgbgmenkpkmhnlbibmg"
  },
  {
    "GM", "alebifccfdpcgpandngmalclpbjpaiak"
  },
  {
    "GE", "kaikhlldfkdjgddjdkangjobahokefeb"
  },
  {
    "DE", "dgkplhfdbkdogfblcghcfcgfalanhomi"
  },
  {
    "GH", "panlkcipneniikplpjnoddnhonjljbdp"
  },
  {
    "GI", "pibapallcmncjajdoamgdnmgcbefekgn"
  },
  {
    "GR", "ochemplgmlglilaflfjnmngpfmpmjgnb"
  },
  {
    "GL", "gjknekmnjalchmfjbecifkoohcmlolhp"
  },
  {
    "GD", "kbllnlfaaoonbliendlhmoleflgdekki"
  },
  {
    "GP", "keggdlgkcfmmopgnogknokiocjjieapm"
  },
  {
    "GU", "mfnbeofelbcabhloibhpklffiifjcdfl"
  },
  {
    "GT", "jdhabeecpolnjdiplbcpjlohmlgdjgjh"
  },
  {
    "GG", "ncldbgolkmlgfkoignkdjehiadnfmlid"
  },
  {
    "GN", "gcfgdkmegcjaceeofigbabmjjiojdgnb"
  },
  {
    "GW", "kheejcjggceghjdinbcklehfkobloboc"
  },
  {
    "GY", "fehpbhdbjpnaibhncpeedfkogiiajcne"
  },
  {
    "HT", "pkpmecljhbjbiicbnfcfgpgmpehldemm"
  },
  {
    "HM", "kfjeoeekjccibpockdjcdjbgagaaopdj"
  },
  {
    "VA", "ljhceaiogeejahjjblnfaaocgogchpkb"
  },
  {
    "HN", "llmmfofcojgcignfnaodhdhdhphhmfmf"
  },
  {
    "HK", "plpcpclbpkilccbegfbpediidmejaahc"
  },
  {
    "HU", "pofhnfhkonpjephlcjlmbjmlikiaddoc"
  },
  {
    "IS", "cljplhmgochlncaelcabfgeefebhmfnk"
  },
  {
    "IN", "kdkakecfnmlfifkhlekmfkmmkpgeckcl"
  },
  {
    "ID", "lanimmipljlbdnajkabaoiklnpcaoakp"
  },
  {
    "IR", "mhiehehcmljjmpibmpiadoeefnchmbdm"
  },
  {
    "IQ", "oejhcomgcgcofdfchkdhkjohnioijofn"
  },
  {
    "IE", "fbmfelhaipnlicodibhjkmeafbcgpfnm"
  },
  {
    "IM", "blofecpleppfodonanffdbepbiijmklm"
  },
  {
    "IL", "fiodhmddlgkgajbhohfdmkliikidmaom"
  },
  {
    "IT", "gjkhegliajlngffafldbadcnpfegmkmb"
  },
  {
    "JM", "ncfbonfnhophngmkkihoieoclepddfhm"
  },
  {
    "JP", "ienmdlgalnmefnpjggommgdilkklopof"
  },
  {
    "JE", "lfgnglkpngeffaijiomlppnobpchhcgf"
  },
  {
    "JO", "gnkmfghehkoegoabhndflbdmfnlgeind"
  },
  {
    "KZ", "jadlfgggcfdhicaoacokfpmccbmedkim"
  },
  {
    "KE", "bfhpiebgnciehokapeppcinalnibbnan"
  },
  {
    "KI", "dkghhhflbpfknidjbhlophapheggpahk"
  },
  {
    "KP", "pnokpaenadbgpjpmlnoamnmpjbjlfoaf"
  },
  {
    "KR", "clgbjhhcdihjgbomhpmfdjdiagejadja"
  },
  {
    "KW", "ehkeinmpkojiiacjohbalbnhloiaifig"
  },
  {
    "KG", "hehalbiboicjbbcfhckdfeijjjppdhij"
  },
  {
    "LA", "lhjcndbhldpnapjddfgohdcdmfibgpon"
  },
  {
    "LV", "pooflbdadogbmjmnnppfmklfjbmoblfa"
  },
  {
    "LB", "hkengofpokonjepdafjdeclejledncdj"
  },
  {
    "LS", "mdojkinfephdfhbfadcnnfcjfniefael"
  },
  {
    "LR", "alenpijagefjamgompebcjhbfhepnphh"
  },
  {
    "LY", "mnhglgpnnohpipdeinibpbnlnpledicf"
  },
  {
    "LI", "onhaidkdpiboaolhnaddeddfaabomchd"
  },
  {
    "LT", "aokfbnlokidoepkhilbmfdkdhajkpbli"
  },
  {
    "LU", "gnmaofjfninpeccldcmlkbinhhohmbck"
  },
  {
    "MO", "ncmdondkaofghlnhiabnfilafhmabong"
  },
  {
    "MG", "lapgbedoccnchodbgfmafpkkhlfmcehe"
  },
  {
    "MW", "dhmcaoadkmoejegjpjgkjhibioemkfni"
  },
  {
    "MY", "dadpenhnclbbkjfbkgkgecknfjggpbmm"
  },
  {
    "MV", "ggclalmmmmgjcoleeficgnnjkpgeinfd"
  },
  {
    "ML", "flocoipmnbpcodjfhmkjecjpbkcmkecp"
  },
  {
    "MT", "emckddclmcjoilbadmdjdakabpnkdkhk"
  },
  {
    "MH", "cpjafhooepmhnflmjabfeaiopfbljhpo"
  },
  {
    "MQ", "chbeaiccoofemohdajloflfkblbgdiih"
  },
  {
    "MR", "dfmnoondmnbngeilibiicikjenjjeigi"
  },
  {
    "MU", "iobofpagkcicpcijjfmnghgppbghnpdo"
  },
  {
    "YT", "lcnaekpkllhpljanlibochejknjflodn"
  },
  {
    "MX", "dclpeadnefbjogjcamdglgmmbbgnjcob"
  },
  {
    "FM", "pjiglaefpchnekpbkbfngjnfhlcmhgbk"
  },
  {
    "MD", "paiickhoniddnnlhhdmhjcfemgkgfohn"
  },
  {
    "MC", "iloglofhibeghkfbocphifnfpccmplgd"
  },
  {
    "MN", "pclbpikpdcdondhappcgloeohjgammia"
  },
  {
    "ME", "dkjadbekoidbnlmaomlcjjgkofkechlo"
  },
  {
    "MS", "mknfcplgmgbfkklaiimdakgjbeonapeh"
  },
  {
    "MA", "pmbhpljpfciommdigfblnenpdiapdafl"
  },
  {
    "MZ", "gilieeicpdnkcjbohfhjhpmpjocapbko"
  },
  {
    "MM", "bbeoopklmfincipdlffbbphpjefmimmp"
  },
  {
    "NA", "paoffgbbehbibcihhmboiaebgojdnibj"
  },
  {
    "NR", "jpejbbflggaiaflclgomjcolpomjmhlh"
  },
  {
    "NP", "ohodaiianeochclnnobadfikohciggno"
  },
  {
    "NL", "choggjlbfndjppfiidbhmefapnlhcdhe"
  },
  {
    "NC", "apmipakgigaapfahiojgjgkfgcdekbpp"
  },
  {
    "NZ", "dlbokjgcdlhkgfeklggoncjhihaebnai"
  },
  {
    "NI", "jajkonoepahongnlnfbfmlnpnkjkchof"
  },
  {
    "NE", "mmhmpjfgnhibhfccegfohnibkpooppkn"
  },
  {
    "NG", "bhkddokohamnindobkmifljnpgijdjdh"
  },
  {
    "NU", "celbcocehclbnblfndjfjleagcbbpooc"
  },
  {
    "NF", "bcnnffpigdndbdohgifflckehcoofigc"
  },
  {
    "MK", "njlgnoebifbjpafbmnkkchknkinmeljm"
  },
  {
    "MP", "cpjjnbhhjohkkmkkplcfkobjfbjlildd"
  },
  {
    "NO", "ciibjdmjfejjghmnlonlihnjodfckfbo"
  },
  {
    "OM", "cobdmgempkofdfhgmbhfckemppmlbjob"
  },
  {
    "PK", "aiaabcbklimkipbpalfoaehfdebbainb"
  },
  {
    "PW", "ejlnmikcbnjpaaolkheodefhahiabjga"
  },
  {
    "PS", "iienfoenehmoepjgljgjdkenjedjfjol"
  },
  {
    "PA", "aafjalakdldginkbeobaiamnfphcdmko"
  },
  {
    "PG", "monkjbjmhjepdcaedlejhmjjjcjpiiaa"
  },
  {
    "PY", "aoidaoefdchfhdjfdffjnnlbfepfkadj"
  },
  {
    "PE", "pmbmbglgbofljclfopjkkompfgedgjhi"
  },
  {
    "PH", "ocmnmegmbhbfmdnjoppmlbhfcpmedacn"
  },
  {
    "PN", "ccpkbhegiebckfidhnoihgdmddhnmdfh"
  },
  {
    "PL", "feeklcgpaolphdiamjaolkkcpbeihkbh"
  },
  {
    "PT", "gchnahcajhccobheiedkgdpfboljkhge"
  },
  {
    "PR", "bpjdfagamlhoojajkeifbendedaikinl"
  },
  {
    "QA", "jicllaljbaldhopinkkegkfpmmnnhmbc"
  },
  {
    "RE", "aeglmpapdhfhdicbifhnmaoehffffmie"
  },
  {
    "RO", "jpapeieehcilkcfpljhopohigdhbnjck"
  },
  {
    "RU", "nfcegbjbohhjljcdogkmookngaiplbna"
  },
  {
    "RW", "djjoaejcadmjbgadeijpbokipgmolnih"
  },
  {
    "BL", "fjefhohmfmokjmnibamjnpiafikmmlef"
  },
  {
    "SH", "dpodaelfodkebmgmmdoecleopjggboln"
  },
  {
    "KN", "idmipdncpnfbfonogngaimigpbpnenpb"
  },
  {
    "LC", "lhlajkngiihbjjaakfgkencpppeahhfi"
  },
  {
    "MF", "hihpbgpfcelklhigbkfnbdgjmpbnabmo"
  },
  {
    "PM", "cpkbkgenaaododkijfnfmgmpekbcfjcg"
  },
  {
    "VC", "bnonnlpingklaggdalihppicgpaghpop"
  },
  {
    "WS", "jfckclnlfaekpfklphjagmjiphjcchmj"
  },
  {
    "SM", "lneikknijgnijfnpoahmfkefboallgin"
  },
  {
    "ST", "djlblammehomffbplemhekjeghekglpc"
  },
  {
    "SA", "gmhojjgbbfachddbgojljenplnhialka"
  },
  {
    "SN", "haalbaecaigldhgnjfmjbedegjipkdfb"
  },
  {
    "RS", "dlfdepidpnibdoblimabdmgpobophapn"
  },
  {
    "SC", "dmdapbmagehdijbdhbdbfjijgmcppjml"
  },
  {
    "SL", "piajfdpbabffhdlmpkaejndbdnohljfn"
  },
  {
    "SG", "jilipkheolgjanjhhhdmbaleiiblnepe"
  },
  {
    "SX", "igdomgnppdmcglgohoamnpegjelohlkj"
  },
  {
    "SK", "obponfmfefkaeehakbehbnnlcbebebhd"
  },
  {
    "SI", "dckjbnoilglapbgmniiagempimbaicmn"
  },
  {
    "SB", "mlbgbnccloeobccglpaachnaabgegcni"
  },
  {
    "SO", "hnfmhdkkmcgeppiiohpondjgibepgfeb"
  },
  {
    "ZA", "jadaiaonajphgnbamppeenldepoajgbf"
  },
  {
    "GS", "ghclfflogdfhnjciileceoofmhkneggp"
  },
  {
    "SS", "kkfngpdjfcddimfgkgibaccaoehjplkn"
  },
  {
    "ES", "ganmbmiebelpdlnohnabgkkocholelbp"
  },
  {
    "LK", "gmahgggkpliaoidcaknflpbgpehcjmhc"
  },
  {
    "SD", "dhcfofldcefkohnjcnfodlbiakgedidd"
  },
  {
    "SR", "khgbibnjdanhnoebnfjgpnfbkohdngph"
  },
  {
    "SJ", "kchkidfjkghdocdicfpmbckmjfgnlndb"
  },
  {
    "SE", "clncjboijmbkcjgkechfhalginbnplpp"
  },
  {
    "CH", "gnamhdlealpfbanappoephfdjeoehggd"
  },
  {
    "SY", "hnhakbhiamjojdoajhebemlajheokinm"
  },
  {
    "TW", "jejmkjlhckkijknapfhfoogakgoelhen"
  },
  {
    "TJ", "nfpgpnagpefhcijfnabpdejiiejplonp"
  },
  {
    "TZ", "jnlkpmlmfdipllbnjmjomkddplafclch"
  },
  {
    "TH", "mjkmkfbpiegjkbeolgpomaloeiiffodm"
  },
  {
    "TL", "kmdanbbapegbkpjkfdldmekconhnmmmo"
  },
  {
    "TG", "alinepjaedjagibhfjcejemabijbohmi"
  },
  {
    "TK", "bbobjkhpggphapdpcchkbklglkindkcc"
  },
  {
    "TO", "jdkdhebphdakbabdbgefjkdbdoagmdec"
  },
  {
    "TT", "nbmopmgpfmalleghhbkablkoamofibpk"
  },
  {
    "TN", "hgmkfpcpppjheoknjjfpajfmibkndjdf"
  },
  {
    "TR", "fahflofbglhemnakgdmillobeencekne"
  },
  {
    "TM", "fhbmmefncojhnjhbckninoliclloeeac"
  },
  {
    "TC", "hbiblijpgfgphhfoajjmcgdbhmobbfba"
  },
  {
    "TV", "kennokhomgefcfjjjhckbiidlhmkinca"
  },
  {
    "UG", "bolcbpmofcabjoflcmljongimpbpeagb"
  },
  {
    "UA", "enkpeojckjlmehbniegocfffdkanjhef"
  },
  {
    "AE", "ajdiilmgienedlgohldjicpcofnckdmn"
  },
  {
    "GB", "cdjnpippjnphaeahihhpafnneefcnnfh"
  },
  {
    "US", "kkjipiepeooghlclkedllogndmohhnhi"
  },
  {
    "UM", "ocikkcmgfagemkpbbkjlngjomkdobgpp"
  },
  {
    "UY", "cejbfkalcdepkoekifpidloabepihogd"
  },
  {
    "UZ", "chpbioaimigllimaalbibcmjjcfnbpid"
  },
  {
    "VU", "ogbkgicanbpgkemjooahemnoihlihonp"
  },
  {
    "VE", "okopabpainkagabcmkfnnchaakhimaoe"
  },
  {
    "VN", "jcffalbkohmmfjmgkdcphlimplejkmon"
  },
  {
    "VG", "jlfjphoakpnmhpldmdkdhekopbjmkljn"
  },
  {
    "VI", "infpagefbmdijbaigeagmldkjnjdhhfa"
  },
  {
    "WF", "hefgpgfflbaepfgbafaaadffchekggfg"
  },
  {
    "EH", "fjhkoeiglahhpcmgfpalgckcaoaifjkn"
  },
  {
    "YE", "cijopjkddpagbkjpnnbjcecfamjbkakp"
  },
  {
    "ZM", "inmfjchmafaondfnpgffefgbdmmfgenb"
  },
  {
    "ZW", "fmobbdfaoifmdjonfklmapdliabjdmjp"
  }
};

const std::set<std::string> kPurchaseIntentUserModelIds = {
  "jememeholcpjpoahinnlafoiaknnmfgl",
  "hfonhokmgmjionconfpknjfphfahdklo",
  "anlkmbkbgleadcacchhgdoecllpllknb",
  "imoolhehjnpebcjecoinphmohihmbccj",
  "kgnhcdjacgcanjnbdcmngdeoncckfmfh",
  "pmlmnjficamnkblapnohndlnhkkoaoco",
  "majdffglhcddbbanjnhfocagmmhobghd",
  "bhdlolcjjefaidodgffjhpbeeapabpgn",
  "pbanoihfljabneihobeplbciopfilajn",
  "cbdjliajiakicmdohhbjbgggacbjdnmn",
  "enadnicbppimlbpilkeaogmmfpennphn",
  "cpnhinhihfnhhmoknpbkcifgadjbcjnf",
  "ocegkjjbmlnibhfjmeiaidplhcbdhomd",
  "kklfafolbojbonkjgifmmkdmaaimminj",
  "jmneklmcodckmpipiekkfaokobhkflep",
  "llmikniomoddmmghodjfbncbidjlhhid",
  "aaoipmkakdldlippoaocoegnnfnpcokj",
  "megoieebjempmobckocciojgfhfmiejb",
  "ppkgobeickbpfmmkbhfgmgiloepdiagn",
  "ndfnmlonkimafoabeafnaignianecdhf",
  "apndmjdcfbhgfccccdmmeofpdgnlbbif",
  "lnbdfmpjjckjhnmahgdojnfnmdmpebfn",
  "demegfhfekncneocdlflandgegpcoofj",
  "jiodmgmkikfbkchgenlamoabbfnobnfh",
  "aeiffmlccgeaacefkfknodmnebanekbo",
  "hemccombjdaepjnhhdplhiaedaackooa",
  "dggplmjbjalpdgndkigojpikblaiflke",
  "jbibpedjodeigdoimlgpikphaabhdbie",
  "iefeaaegnnpiadjfoeifapjmflnbjehg",
  "bfdagcnfmfaidkkjcmfjmogiefoiglem",
  "kfimhlhdlhimaheficomcahhbaicoele",
  "fbpmbjccnaaeogogeldlomcmlhllgaje",
  "cpbmgmnfoakodmmpppabghhckchppneg",
  "gcikmigghagkligpileoekfjmohokjhm",
  "ahhbponhjohgifhjbjggafboffbimmmg",
  "fjgjapaemndekhnfeopeoeajfpmlgemo",
  "cfbbahiimladdkhblpkkokkmemlmkbhe",
  "oeneodeckioghmhokkmcbijfanjbanop",
  "cmknopomfihgdpjlnjhjkmogaooeoeic",
  "mmiflfidlgoehkhhkeodfdjpbkkjadgi",
  "gpaihfendegmjoffnpngjjhbipbioknd",
  "efpgpbmpbkhadlnpdnjigldeebgacopp",
  "ljfeoddejgcdofgnpgljaeiaemfimgej",
  "oahnhemdhgogkhgljdkhbficecbplmdf",
  "gbbfjnnpelockljikcmjkeodlaebjokm",
  "gfccfpdijajblfnkmddbflphiiibifik",
  "mjennfbimaafgcoekloojmbhnkophgni",
  "pnfogoijegjkepejdabehdfadbkpgoed",
  "cegiaceckhbagmmfcoeebeghiddghbkk",
  "efcmpmpbmaehimomnmonodlpnghelnfi",
  "hkgnnbjmfcelmehmphhbjigedknjobaa",
  "kignebofcmcgmjfiapilgdfkbekmkdmg",
  "clcghlkineckendfhkgdpkplofknofjo",
  "hmmoibmjgckbeejmcfflnngeacaklchb",
  "nopcbglolocphcdeikfkoeppkhijacij",
  "mjhnpmgafkmildljebajibghemlbffni",
  "mdopdmalncfakkimlojioichjbebcaip",
  "boeecnnjahpahhockgdcbdlaimpcflig",
  "hknminnkgcjafipipbbalkakppehkpjn",
  "iejekkikpddbbockoldagmfcdbffomfc",
  "kmfkbonhconlbieplamnikedgfbggail",
  "phihhhnelfclomhodhgalooldjgejgfl",
  "obihnhimgbeenjgfnlcdbfdgkkgeogdp",
  "gciaobanmdlfkegiikhgdoogegeghhch",
  "imnpbmpnmlmkjpgfnfeplikingjklpej",
  "ojfkdbfibflfejobeodhoepghgoghkii",
  "adnhangbagjlobdeicimblgcnafegpfb",
  "gncihmgakhljdlkadibldhdfnocfplci",
  "diacfpapelanfbkmehdpaaohmnkkngge",
  "jdigggiplmjlocdopbdmjibckpamigij",
  "npacefioaofgbofilfnhliofkefklipp",
  "lbiagcddmfapjfbebccolcahfppaimmo",
  "aogeahmaehgnkobkmnmkhkimdjpgfgen",
  "molpidmcmbmhbicckmbopbmiojddebke",
  "biobhkionbllnfljaapocfpdmhamedkf",
  "ecneelfoifpgnignhipdebhbkgcphmic",
  "bgifagoclclhhoflocdefiklgodpihog",
  "mhmfclafeemiphfebhnobinkplbmpocm",
  "mjaianjdadeiocokpoanbgjhegficcce",
  "jbjodokafbafhemocebljdblgnfajabi",
  "nchncleokkkkdfgbgmenkpkmhnlbibmg",
  "alebifccfdpcgpandngmalclpbjpaiak",
  "kaikhlldfkdjgddjdkangjobahokefeb",
  "dgkplhfdbkdogfblcghcfcgfalanhomi",
  "panlkcipneniikplpjnoddnhonjljbdp",
  "pibapallcmncjajdoamgdnmgcbefekgn",
  "ochemplgmlglilaflfjnmngpfmpmjgnb",
  "gjknekmnjalchmfjbecifkoohcmlolhp",
  "kbllnlfaaoonbliendlhmoleflgdekki",
  "keggdlgkcfmmopgnogknokiocjjieapm",
  "mfnbeofelbcabhloibhpklffiifjcdfl",
  "jdhabeecpolnjdiplbcpjlohmlgdjgjh",
  "ncldbgolkmlgfkoignkdjehiadnfmlid",
  "gcfgdkmegcjaceeofigbabmjjiojdgnb",
  "kheejcjggceghjdinbcklehfkobloboc",
  "fehpbhdbjpnaibhncpeedfkogiiajcne",
  "pkpmecljhbjbiicbnfcfgpgmpehldemm",
  "kfjeoeekjccibpockdjcdjbgagaaopdj",
  "ljhceaiogeejahjjblnfaaocgogchpkb",
  "llmmfofcojgcignfnaodhdhdhphhmfmf",
  "plpcpclbpkilccbegfbpediidmejaahc",
  "pofhnfhkonpjephlcjlmbjmlikiaddoc",
  "cljplhmgochlncaelcabfgeefebhmfnk",
  "kdkakecfnmlfifkhlekmfkmmkpgeckcl",
  "lanimmipljlbdnajkabaoiklnpcaoakp",
  "mhiehehcmljjmpibmpiadoeefnchmbdm",
  "oejhcomgcgcofdfchkdhkjohnioijofn",
  "fbmfelhaipnlicodibhjkmeafbcgpfnm",
  "blofecpleppfodonanffdbepbiijmklm",
  "fiodhmddlgkgajbhohfdmkliikidmaom",
  "gjkhegliajlngffafldbadcnpfegmkmb",
  "ncfbonfnhophngmkkihoieoclepddfhm",
  "ienmdlgalnmefnpjggommgdilkklopof",
  "lfgnglkpngeffaijiomlppnobpchhcgf",
  "gnkmfghehkoegoabhndflbdmfnlgeind",
  "jadlfgggcfdhicaoacokfpmccbmedkim",
  "bfhpiebgnciehokapeppcinalnibbnan",
  "dkghhhflbpfknidjbhlophapheggpahk",
  "pnokpaenadbgpjpmlnoamnmpjbjlfoaf",
  "clgbjhhcdihjgbomhpmfdjdiagejadja",
  "ehkeinmpkojiiacjohbalbnhloiaifig",
  "hehalbiboicjbbcfhckdfeijjjppdhij",
  "lhjcndbhldpnapjddfgohdcdmfibgpon",
  "pooflbdadogbmjmnnppfmklfjbmoblfa",
  "hkengofpokonjepdafjdeclejledncdj",
  "mdojkinfephdfhbfadcnnfcjfniefael",
  "alenpijagefjamgompebcjhbfhepnphh",
  "mnhglgpnnohpipdeinibpbnlnpledicf",
  "onhaidkdpiboaolhnaddeddfaabomchd",
  "aokfbnlokidoepkhilbmfdkdhajkpbli",
  "gnmaofjfninpeccldcmlkbinhhohmbck",
  "ncmdondkaofghlnhiabnfilafhmabong",
  "lapgbedoccnchodbgfmafpkkhlfmcehe",
  "dhmcaoadkmoejegjpjgkjhibioemkfni",
  "dadpenhnclbbkjfbkgkgecknfjggpbmm",
  "ggclalmmmmgjcoleeficgnnjkpgeinfd",
  "flocoipmnbpcodjfhmkjecjpbkcmkecp",
  "emckddclmcjoilbadmdjdakabpnkdkhk",
  "cpjafhooepmhnflmjabfeaiopfbljhpo",
  "chbeaiccoofemohdajloflfkblbgdiih",
  "dfmnoondmnbngeilibiicikjenjjeigi",
  "iobofpagkcicpcijjfmnghgppbghnpdo",
  "lcnaekpkllhpljanlibochejknjflodn",
  "dclpeadnefbjogjcamdglgmmbbgnjcob",
  "pjiglaefpchnekpbkbfngjnfhlcmhgbk",
  "paiickhoniddnnlhhdmhjcfemgkgfohn",
  "iloglofhibeghkfbocphifnfpccmplgd",
  "pclbpikpdcdondhappcgloeohjgammia",
  "dkjadbekoidbnlmaomlcjjgkofkechlo",
  "mknfcplgmgbfkklaiimdakgjbeonapeh",
  "pmbhpljpfciommdigfblnenpdiapdafl",
  "gilieeicpdnkcjbohfhjhpmpjocapbko",
  "bbeoopklmfincipdlffbbphpjefmimmp",
  "paoffgbbehbibcihhmboiaebgojdnibj",
  "jpejbbflggaiaflclgomjcolpomjmhlh",
  "ohodaiianeochclnnobadfikohciggno",
  "choggjlbfndjppfiidbhmefapnlhcdhe",
  "apmipakgigaapfahiojgjgkfgcdekbpp",
  "dlbokjgcdlhkgfeklggoncjhihaebnai",
  "jajkonoepahongnlnfbfmlnpnkjkchof",
  "mmhmpjfgnhibhfccegfohnibkpooppkn",
  "bhkddokohamnindobkmifljnpgijdjdh",
  "celbcocehclbnblfndjfjleagcbbpooc",
  "bcnnffpigdndbdohgifflckehcoofigc",
  "njlgnoebifbjpafbmnkkchknkinmeljm",
  "cpjjnbhhjohkkmkkplcfkobjfbjlildd",
  "ciibjdmjfejjghmnlonlihnjodfckfbo",
  "cobdmgempkofdfhgmbhfckemppmlbjob",
  "aiaabcbklimkipbpalfoaehfdebbainb",
  "ejlnmikcbnjpaaolkheodefhahiabjga",
  "iienfoenehmoepjgljgjdkenjedjfjol",
  "aafjalakdldginkbeobaiamnfphcdmko",
  "monkjbjmhjepdcaedlejhmjjjcjpiiaa",
  "aoidaoefdchfhdjfdffjnnlbfepfkadj",
  "pmbmbglgbofljclfopjkkompfgedgjhi",
  "ocmnmegmbhbfmdnjoppmlbhfcpmedacn",
  "ccpkbhegiebckfidhnoihgdmddhnmdfh",
  "feeklcgpaolphdiamjaolkkcpbeihkbh",
  "gchnahcajhccobheiedkgdpfboljkhge",
  "bpjdfagamlhoojajkeifbendedaikinl",
  "jicllaljbaldhopinkkegkfpmmnnhmbc",
  "aeglmpapdhfhdicbifhnmaoehffffmie",
  "jpapeieehcilkcfpljhopohigdhbnjck",
  "nfcegbjbohhjljcdogkmookngaiplbna",
  "djjoaejcadmjbgadeijpbokipgmolnih",
  "fjefhohmfmokjmnibamjnpiafikmmlef",
  "dpodaelfodkebmgmmdoecleopjggboln",
  "idmipdncpnfbfonogngaimigpbpnenpb",
  "lhlajkngiihbjjaakfgkencpppeahhfi",
  "hihpbgpfcelklhigbkfnbdgjmpbnabmo",
  "cpkbkgenaaododkijfnfmgmpekbcfjcg",
  "bnonnlpingklaggdalihppicgpaghpop",
  "jfckclnlfaekpfklphjagmjiphjcchmj",
  "lneikknijgnijfnpoahmfkefboallgin",
  "djlblammehomffbplemhekjeghekglpc",
  "gmhojjgbbfachddbgojljenplnhialka",
  "haalbaecaigldhgnjfmjbedegjipkdfb",
  "dlfdepidpnibdoblimabdmgpobophapn",
  "dmdapbmagehdijbdhbdbfjijgmcppjml",
  "piajfdpbabffhdlmpkaejndbdnohljfn",
  "jilipkheolgjanjhhhdmbaleiiblnepe",
  "igdomgnppdmcglgohoamnpegjelohlkj",
  "obponfmfefkaeehakbehbnnlcbebebhd",
  "dckjbnoilglapbgmniiagempimbaicmn",
  "mlbgbnccloeobccglpaachnaabgegcni",
  "hnfmhdkkmcgeppiiohpondjgibepgfeb",
  "jadaiaonajphgnbamppeenldepoajgbf",
  "ghclfflogdfhnjciileceoofmhkneggp",
  "kkfngpdjfcddimfgkgibaccaoehjplkn",
  "ganmbmiebelpdlnohnabgkkocholelbp",
  "gmahgggkpliaoidcaknflpbgpehcjmhc",
  "dhcfofldcefkohnjcnfodlbiakgedidd",
  "khgbibnjdanhnoebnfjgpnfbkohdngph",
  "kchkidfjkghdocdicfpmbckmjfgnlndb",
  "clncjboijmbkcjgkechfhalginbnplpp",
  "gnamhdlealpfbanappoephfdjeoehggd",
  "hnhakbhiamjojdoajhebemlajheokinm",
  "jejmkjlhckkijknapfhfoogakgoelhen",
  "nfpgpnagpefhcijfnabpdejiiejplonp",
  "jnlkpmlmfdipllbnjmjomkddplafclch",
  "mjkmkfbpiegjkbeolgpomaloeiiffodm",
  "kmdanbbapegbkpjkfdldmekconhnmmmo",
  "alinepjaedjagibhfjcejemabijbohmi",
  "bbobjkhpggphapdpcchkbklglkindkcc",
  "jdkdhebphdakbabdbgefjkdbdoagmdec",
  "nbmopmgpfmalleghhbkablkoamofibpk",
  "hgmkfpcpppjheoknjjfpajfmibkndjdf",
  "fahflofbglhemnakgdmillobeencekne",
  "fhbmmefncojhnjhbckninoliclloeeac",
  "hbiblijpgfgphhfoajjmcgdbhmobbfba",
  "kennokhomgefcfjjjhckbiidlhmkinca",
  "bolcbpmofcabjoflcmljongimpbpeagb",
  "enkpeojckjlmehbniegocfffdkanjhef",
  "ajdiilmgienedlgohldjicpcofnckdmn",
  "cdjnpippjnphaeahihhpafnneefcnnfh",
  "kkjipiepeooghlclkedllogndmohhnhi",
  "ocikkcmgfagemkpbbkjlngjomkdobgpp",
  "cejbfkalcdepkoekifpidloabepihogd",
  "chpbioaimigllimaalbibcmjjcfnbpid",
  "ogbkgicanbpgkemjooahemnoihlihonp",
  "okopabpainkagabcmkfnnchaakhimaoe",
  "jcffalbkohmmfjmgkdcphlimplejkmon",
  "jlfjphoakpnmhpldmdkdhekopbjmkljn",
  "infpagefbmdijbaigeagmldkjnjdhhfa",
  "hefgpgfflbaepfgbafaaadffchekggfg",
  "fjhkoeiglahhpcmgfpalgckcaoaifjkn",
  "cijopjkddpagbkjpnnbjcecfamjbkakp",
  "inmfjchmafaondfnpgffefgbdmmfgenb",
  "fmobbdfaoifmdjonfklmapdliabjdmjp"
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_LANGUAGE_CODES_H_
