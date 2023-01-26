/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/language_components.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_piece.h"

namespace {

constexpr auto kLanguageComponentIds = base::MakeFixedFlatSet<
    base::StringPiece>(
    {"ijmgabghpbflfadffhpmjklamndnonha", "hddanjaanmjbdklklpldpgpmbdmaiihb",
     "blcjdmhlkddhompnlbjlpccpelipihem", "pecokcgeeiabdlkfkfjpmfldfhhjlmom",
     "pgkommhmfkkkfbbcnnfhlkagbodoimjm", "emopjfcnkbjfedjbfgajdajnkkfekcbl",
     "knjanagkmnjgjjiekhmhclcbcdbjajmk", "onjbjcnjheabeghbflckfekjnnjgfabn",
     "ghgfdmhmhifphaokjfplffppmlfchofm", "mbcmenffnlanegglgpdgbmmldfpclnkm",
     "clemenahenllbgeilmkllcpfcfhloebp", "cmggjadifnmfdfeohgomnaggonihmbli",
     "fabdaiigiipinlpgkkakcffbhkmfnfek", "cgpdnipmfmbfceokoadgbliclnoeddef",
     "hjfgajachhnlgjfjgbjbehgcoecfigap", "keghklelbpkaiogcinbjnnjideedfmdd",
     "lldcgicjgpomllohjncpcjkijjklooji", "ogmkkmobemflinkielcngdanaojkamcc",
     "ligbalgipjnajannojamijenlackcioc", "bkicjkclpdihdjbpajnegckopabcebff",
     "dafhhkffcifanfgpjlgejcahkidpbnfj", "ngnfehahlmgkoclalhldboigojplccbl",
     "hkfnnljkbmadknefddjfligobbiagmea", "igdpeoeohlgknkdnjhbijpfeenfiepdc",
     "mdacdgffhlidpdmhckokeoajhojeibmp", "keahfifnfebhoaaghffigjnhgbkkaibd",
     "fllhkadnpidionapphjicdkfdiloghad", "eakppbikiomjdnligoccikcdipojiljk",
     "ddekfkhnjcpbankekbelkeekibbhmgnh", "clegognodnfcpbmhpfgbpckebppbaebp",
     "obdagonejiaflgbifdloghkllgdjpcdj", "apmkijnjnhdabkckmkkejgdnbgdglocb",
     "gdmbpbmoiogajaogpajfhonmlepcdokn", "amedpgedagedjlkgjajebgecfkldmdfa",
     "ncjeinjdknabbgmaijmnipndbggmchff", "nllaodpgkekajbabhjphhekenlnlpdmd",
     "klifniioldbebiedppmbobpdiombacge", "aoljgchlinejchjbbkicamhfdapghahp",
     "neglbnegiidighiifljiphcldmgibifn", "jginkacioplimdjobccplmgiphpjjigl",
     "ocilmpijebaopmdifcomolmpigakocmo", "halbpcgpgcafebkldcfhllomekophick",
     "onmakacikbbnhmanodpjhijljadlpbda", "bjhkalknmdllcnkcjdjncplkbbeigklb",
     "jamflccjbegjmghgaljipcjjbipgojbn", "gfoibbmiaikelajlipoffiklghlbapjf",
     "lbbgedbjaoehfaoklcebbepkbmljanhc", "ijgkfgmfiinppefbonemjidmkhgbonei",
     "anhpkncejedojfopbigplmbfmbhkagao", "ejioajnkmjfjfbbanmgfbagjbdlfodge",
     "hlipecdabdcghhdkhfmhiclaobjjmhbo", "bbefpembgddgdihpkcidgdgiojjlchji",
     "hgcnnimnfelflnbdfbdngikednoidhmg", "ebgmbleidclecpicaccgpjdhndholiel",
     "mdpcebhdjplkegddkiodmbjcmlnhbpog", "hpkelamfnimiapfbeeelpfkhkgfoejil",
     "khgpojhhoikmhodflkppdcakhbkaojpi", "biklnlngpkiolabpfdiokhnokoomfldp",
     "pkmkhkpglegjekkfabaelkbgkfegnmde", "acdmbpdfmekdichgebponnjloihkdejf",
     "cmkgcbedakcdopgjdhbbpjjaodjcdbdp", "ifbdofecjcadpnkokmdfahhjadcppmko",
     "hoeihggfhgnfpdnaeocfpmoelcenhfla", "gbmolmcnbhegkhjhbhjehcpbjonjlgfg",
     "fioklomfllflofcdiklpgabemclgkkdh", "oiihbhoknlbjonghmcplpbpkcdeinlfg",
     "nchaailfhkbnlnaobgjmoamdfnclhdoo", "fbdjobfnceggaokdnlebbaplnhednlhl",
     "nkajjfpapgfhlcpmmoipafbfnnmojaep", "dhhkjdedjghadoekfklpheblplmlpdec",
     "apaklaabmoggbjglopdnboibkipdindg", "eociikjclgmjinkgeoehleofopogehja",
     "anbffbdnbfabjafoemkhoaelpodojknn", "jdfafcdnmjeadcohbmjeeijgheobldng",
     "jknflnmanienedkoeoginjmbpmkpclki", "nggdckgfnooehkpnmjdodbknekmhcdeg",
     "mhobnohaonkecggnifnffofnihbakjic", "hhejjhncnckfmpkpkhkbknekhkepcill",
     "ljllkgialkdmamlacenmjkhjmimndfil", "dhigelnhdnmhffomicjdmdhefhedgijm",
     "jcfelbpkigpapilbogpidbfdffgdfafe", "ookcfboifajbeojnnebiaoplifgiinof",
     "njimokpabbaelbbpohoflcbjhdgonbmf", "danmahhfdmncbiipbefmjdkembceajdk",
     "lcahllbcfbhghpjjocdhmilokfbpbekn", "deadocmlegcgnokbhhpgblpofkpkeocg",
     "hfboaaehpnfpnpompagbamoknlnladfn", "cppbcboljlmfdgeehadijemhkifhcpnl",
     "knnpciecjoakhokllbgioaceglldlgan", "chnbfebpflegknnjiikofmnebcbphead",
     "hkfkdejbdionnjdhgfhpkcmknmamddde", "nnbaaidlgckbmfdlnioepikbcmjmbadb",
     "dedchpogdooaakplmpjobpkiggalnlif", "alaghdpgppakjfjcipokbaempndnglfk",
     "copfpkggfedomijbmceepmahananponb", "ljambfnmibabkhcpgppfblodipceahab",
     "lflklgkjbemnncejeanindnikiaicpod", "lkcfaihllkinjhmdjgaccjhgdobabpbj",
     "anikcbbbhcobgockdcemaboadbdcnhlg", "gaompbafbaolhddgoafjhkgmnjpjpbip",
     "pppjaeenalohmnhjpemhdkkkghfddbfp", "papnfbjjblebaaeenodbmllepiijfmhn",
     "jmakhmnfhlhioagnmgnakhigadgkpkcl", "gakleannelcniohpkolmbgfpnkjlbnll",
     "lmpaafjmphnclpkfcejgjbnieahkgemg", "fehmnfinbijjdacjgeofhojfdjhbehic",
     "aadlcogkfhaficfijoolhlajkngeecea", "edcpinkoiknmjafcdpolkkeiieddmbab",
     "ooncphbdmekmhldbojgoboebnongkpic", "kemfolmmdooeepfhbpiemnnekfjlbnnd",
     "mihcpmkclenpfgielcipmdbcfpncfojc", "jpleaamlgnfhfemdcfmbknnhcbfnglkh",
     "gbigbbblbdmfhbpobcliehihdedicjfl", "fnmjhgcaloniglpngailbaillhbenela",
     "fgicknpghikljlipmfibgghcdikjfjfj", "nodfcenkjehpafmbmgnapoieilnoijap",
     "dghnlalnaoekcligakadcmmioaieangj", "fbfeebiglbpbmgbefgmijdbcchmdfdhm",
     "gdkeabmpgllapbjgifgfmlfelpdlkapj", "fnhldinjahkdbngcnjfcmidhpjedinbg",
     "aegokocmijocdgiddgjbjkdfiheijfpl", "amkpggbpieoafkbmkijjnefikhjjfogn",
     "adccmiokahgjhdbmhldbnkkplgcgfkpp", "ghikcfknmlkdjiiljfpgpmcfjinpollk",
     "hinecgnhkigghpncjnflaokadaclcfpm", "blaocfojieebnkolagngecdhnipocicj",
     "fojhemdeemkcacelmecilmibcjallejo", "fikmpfipjepnnhiolongfjimedlhaemk",
     "fimpfhgllgkaekhbpkakjchdogecjflf", "ndlciiakjgfcefimfjlfcjgohlgidpnc",
     "nlabdknfkecjaechkekdlkgnapljpfla", "piebpodmljdloocefikhekfjajloneij",
     "ohhkknigpeehdnkeccopplflnodppkme", "jajlkohoekhghdbclekclenlahcjplec",
     "inkmdnaeojfdngbdkbinoinflfahcjfc", "golaphmkhjkdmcakpigbjhneagiapkfh",
     "kcmiboiihhehobbffjebhgadbalknboh", "cmomlghkjichjgbkakaoenfenincefnj",
     "mfaajikelgfodgcgnldapbgjdbncmibc", "gndfhmmkadfdhmchhljmcdhlicdmmlbn",
     "pdgppejghdoknllcnfikoaloobopajjo", "djmefhmnkffabdodgcfjmgffpindaaak",
     "bdepmnbdekgdgjimffimkfeoggmnlbbf", "mogllbjhcnfhcejalaikleeogjmmfkdm",
     "gnhdcgmlejfbcccdjdhjalacjcimlkjh", "jifgjineejhedlmjnkcijoincbhelicp",
     "doclofgiadjildnifgkajdlpngijbpop", "mgdaglmilmjenimbkdmneckfbphfllic",
     "elecgkckipdmnkkgndidemmdhdcdfhnp", "aondicpcneldjpbfemaimbpomgaepjhg",
     "ccmmjlklhnoojaganaecljeecenhafok", "khbhchcpljcejlmijghlabpcmlkkfnid",
     "lnhckckgfdgjgkoelimnmpbnnognpmfb", "nbmbpelgpalcgdghkeafabljjbhmalhf",
     "nonmahhknjgpnoamcdihefcbpdioimbh", "olopfkdcklijkianjbegdegilmhdgpcj",
     "jllmphacilbjnfngcojfgmiimipclfbm", "hkeoedmbihkkglaobeembiogeofffpop",
     "ijgcgakmmgjaladckdppemjgdnjlcgpo", "liddcpbnodlgenmbmmfchepoebgfondk",
     "mocihammffaleonaomjleikagemilaoj", "gjinficpofcocgaaogaiimhacbfkmjmj",
     "hhliclmbfpdlpkdhmpkleicjnemheeea", "edjnpechdkjgcfjepfnnabdkcfcfllpd",
     "nhbpjehmiofogaicflcjhcfdmmkgbohp", "mmmembcojnkgfclnogjfeeiicmiijcnk",
     "ldjaelhegioonaebajlgfnhcipdajfeo", "fnokionjijmckgggmhogifnocpabdafk",
     "ohcnbpfpchlcjchnljcdjebcjdbneecj", "pbegjfplhidokoonohceljofcpbojglg",
     "jaggpekahffhnhhchdkpnigfmdlhenpo", "jephmoboccidmbemhjckbcagagijgcef",
     "mbhiljiiffkobikkoechkpeaopagfhep", "pbjakpdfjkmcajeobebemnjglbjiniln",
     "bfljdbgfmdjgbomhiaoeoleidbfcmmpn", "fmiofedgokpciaklgakbnminmmkocgnd",
     "gpfmbdepojhpjlaelmnlbgginpgnbmfd", "mhdpccgjfkfkdbbpapbgcahhknmbdnjn",
     "eahefjeohmofagkliaddkmokbecfhclm", "gjigddoamjemfcahionjikmlfijoiecf",
     "jhnklldjooclfmgpkipaemehnngabckf", "fjfbodkpnkomodlcanacakhcfmjjgkdf",
     "bncbapkadghlbephbogcmomlecfmdhnb", "dhlnknppkgfgehmmipicnlplhjgpnmnh"});

}  // namespace

namespace ads {

bool IsValidLanguageComponentId(const std::string& id) {
  return base::Contains(kLanguageComponentIds, id);
}

}  // namespace ads
