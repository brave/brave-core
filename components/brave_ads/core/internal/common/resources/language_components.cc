/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/resources/language_components.h"

#include <string_view>

#include "base/containers/fixed_flat_set.h"

namespace {

constexpr auto kLanguageComponentIds = base::MakeFixedFlatSet<std::string_view>(
    base::sorted_unique,
    {
        "aadlcogkfhaficfijoolhlajkngeecea", "acdmbpdfmekdichgebponnjloihkdejf",
        "adccmiokahgjhdbmhldbnkkplgcgfkpp", "aegokocmijocdgiddgjbjkdfiheijfpl",
        "alaghdpgppakjfjcipokbaempndnglfk", "amedpgedagedjlkgjajebgecfkldmdfa",
        "amkpggbpieoafkbmkijjnefikhjjfogn", "anbffbdnbfabjafoemkhoaelpodojknn",
        "anhpkncejedojfopbigplmbfmbhkagao", "anikcbbbhcobgockdcemaboadbdcnhlg",
        "aoljgchlinejchjbbkicamhfdapghahp", "aondicpcneldjpbfemaimbpomgaepjhg",
        "apaklaabmoggbjglopdnboibkipdindg", "apmkijnjnhdabkckmkkejgdnbgdglocb",
        "bbefpembgddgdihpkcidgdgiojjlchji", "bdepmnbdekgdgjimffimkfeoggmnlbbf",
        "bfljdbgfmdjgbomhiaoeoleidbfcmmpn", "biklnlngpkiolabpfdiokhnokoomfldp",
        "bjhkalknmdllcnkcjdjncplkbbeigklb", "bkicjkclpdihdjbpajnegckopabcebff",
        "blaocfojieebnkolagngecdhnipocicj", "blcjdmhlkddhompnlbjlpccpelipihem",
        "bncbapkadghlbephbogcmomlecfmdhnb", "ccmmjlklhnoojaganaecljeecenhafok",
        "cgpdnipmfmbfceokoadgbliclnoeddef", "chnbfebpflegknnjiikofmnebcbphead",
        "clegognodnfcpbmhpfgbpckebppbaebp", "clemenahenllbgeilmkllcpfcfhloebp",
        "cmggjadifnmfdfeohgomnaggonihmbli", "cmkgcbedakcdopgjdhbbpjjaodjcdbdp",
        "cmomlghkjichjgbkakaoenfenincefnj", "copfpkggfedomijbmceepmahananponb",
        "cppbcboljlmfdgeehadijemhkifhcpnl", "dafhhkffcifanfgpjlgejcahkidpbnfj",
        "danmahhfdmncbiipbefmjdkembceajdk", "ddekfkhnjcpbankekbelkeekibbhmgnh",
        "deadocmlegcgnokbhhpgblpofkpkeocg", "dedchpogdooaakplmpjobpkiggalnlif",
        "dghnlalnaoekcligakadcmmioaieangj", "dhhkjdedjghadoekfklpheblplmlpdec",
        "dhigelnhdnmhffomicjdmdhefhedgijm", "dhlnknppkgfgehmmipicnlplhjgpnmnh",
        "djmefhmnkffabdodgcfjmgffpindaaak", "doclofgiadjildnifgkajdlpngijbpop",
        "eahefjeohmofagkliaddkmokbecfhclm", "eakppbikiomjdnligoccikcdipojiljk",
        "ebgmbleidclecpicaccgpjdhndholiel", "edcpinkoiknmjafcdpolkkeiieddmbab",
        "edjnpechdkjgcfjepfnnabdkcfcfllpd", "ejioajnkmjfjfbbanmgfbagjbdlfodge",
        "elecgkckipdmnkkgndidemmdhdcdfhnp", "emopjfcnkbjfedjbfgajdajnkkfekcbl",
        "eociikjclgmjinkgeoehleofopogehja", "fabdaiigiipinlpgkkakcffbhkmfnfek",
        "fbdjobfnceggaokdnlebbaplnhednlhl", "fbfeebiglbpbmgbefgmijdbcchmdfdhm",
        "fehmnfinbijjdacjgeofhojfdjhbehic", "fgicknpghikljlipmfibgghcdikjfjfj",
        "fikmpfipjepnnhiolongfjimedlhaemk", "fimpfhgllgkaekhbpkakjchdogecjflf",
        "fioklomfllflofcdiklpgabemclgkkdh", "fjfbodkpnkomodlcanacakhcfmjjgkdf",
        "fllhkadnpidionapphjicdkfdiloghad", "fmiofedgokpciaklgakbnminmmkocgnd",
        "fnhldinjahkdbngcnjfcmidhpjedinbg", "fnmjhgcaloniglpngailbaillhbenela",
        "fnokionjijmckgggmhogifnocpabdafk", "fojhemdeemkcacelmecilmibcjallejo",
        "gakleannelcniohpkolmbgfpnkjlbnll", "gaompbafbaolhddgoafjhkgmnjpjpbip",
        "gbigbbblbdmfhbpobcliehihdedicjfl", "gbmolmcnbhegkhjhbhjehcpbjonjlgfg",
        "gdkeabmpgllapbjgifgfmlfelpdlkapj", "gdmbpbmoiogajaogpajfhonmlepcdokn",
        "gfoibbmiaikelajlipoffiklghlbapjf", "ghgfdmhmhifphaokjfplffppmlfchofm",
        "ghikcfknmlkdjiiljfpgpmcfjinpollk", "gjigddoamjemfcahionjikmlfijoiecf",
        "gjinficpofcocgaaogaiimhacbfkmjmj", "gndfhmmkadfdhmchhljmcdhlicdmmlbn",
        "gnhdcgmlejfbcccdjdhjalacjcimlkjh", "golaphmkhjkdmcakpigbjhneagiapkfh",
        "gpfmbdepojhpjlaelmnlbgginpgnbmfd", "halbpcgpgcafebkldcfhllomekophick",
        "hddanjaanmjbdklklpldpgpmbdmaiihb", "hfboaaehpnfpnpompagbamoknlnladfn",
        "hgcnnimnfelflnbdfbdngikednoidhmg", "hhejjhncnckfmpkpkhkbknekhkepcill",
        "hhliclmbfpdlpkdhmpkleicjnemheeea", "hinecgnhkigghpncjnflaokadaclcfpm",
        "hjfgajachhnlgjfjgbjbehgcoecfigap", "hkeoedmbihkkglaobeembiogeofffpop",
        "hkfkdejbdionnjdhgfhpkcmknmamddde", "hkfnnljkbmadknefddjfligobbiagmea",
        "hlipecdabdcghhdkhfmhiclaobjjmhbo", "hoeihggfhgnfpdnaeocfpmoelcenhfla",
        "hpkelamfnimiapfbeeelpfkhkgfoejil", "ifbdofecjcadpnkokmdfahhjadcppmko",
        "igdpeoeohlgknkdnjhbijpfeenfiepdc", "ijgcgakmmgjaladckdppemjgdnjlcgpo",
        "ijgkfgmfiinppefbonemjidmkhgbonei", "ijmgabghpbflfadffhpmjklamndnonha",
        "inkmdnaeojfdngbdkbinoinflfahcjfc", "jaggpekahffhnhhchdkpnigfmdlhenpo",
        "jajlkohoekhghdbclekclenlahcjplec", "jamflccjbegjmghgaljipcjjbipgojbn",
        "jcfelbpkigpapilbogpidbfdffgdfafe", "jdfafcdnmjeadcohbmjeeijgheobldng",
        "jephmoboccidmbemhjckbcagagijgcef", "jginkacioplimdjobccplmgiphpjjigl",
        "jhnklldjooclfmgpkipaemehnngabckf", "jifgjineejhedlmjnkcijoincbhelicp",
        "jknflnmanienedkoeoginjmbpmkpclki", "jllmphacilbjnfngcojfgmiimipclfbm",
        "jmakhmnfhlhioagnmgnakhigadgkpkcl", "jpleaamlgnfhfemdcfmbknnhcbfnglkh",
        "kcmiboiihhehobbffjebhgadbalknboh", "keahfifnfebhoaaghffigjnhgbkkaibd",
        "keghklelbpkaiogcinbjnnjideedfmdd", "kemfolmmdooeepfhbpiemnnekfjlbnnd",
        "khbhchcpljcejlmijghlabpcmlkkfnid", "khgpojhhoikmhodflkppdcakhbkaojpi",
        "klifniioldbebiedppmbobpdiombacge", "knjanagkmnjgjjiekhmhclcbcdbjajmk",
        "knnpciecjoakhokllbgioaceglldlgan", "lbbgedbjaoehfaoklcebbepkbmljanhc",
        "lcahllbcfbhghpjjocdhmilokfbpbekn", "ldjaelhegioonaebajlgfnhcipdajfeo",
        "lflklgkjbemnncejeanindnikiaicpod", "liddcpbnodlgenmbmmfchepoebgfondk",
        "ligbalgipjnajannojamijenlackcioc", "ljambfnmibabkhcpgppfblodipceahab",
        "ljllkgialkdmamlacenmjkhjmimndfil", "lkcfaihllkinjhmdjgaccjhgdobabpbj",
        "lldcgicjgpomllohjncpcjkijjklooji", "lmpaafjmphnclpkfcejgjbnieahkgemg",
        "lnhckckgfdgjgkoelimnmpbnnognpmfb", "mbcmenffnlanegglgpdgbmmldfpclnkm",
        "mbhiljiiffkobikkoechkpeaopagfhep", "mdacdgffhlidpdmhckokeoajhojeibmp",
        "mdpcebhdjplkegddkiodmbjcmlnhbpog", "mfaajikelgfodgcgnldapbgjdbncmibc",
        "mgdaglmilmjenimbkdmneckfbphfllic", "mhdpccgjfkfkdbbpapbgcahhknmbdnjn",
        "mhobnohaonkecggnifnffofnihbakjic", "mihcpmkclenpfgielcipmdbcfpncfojc",
        "mmmembcojnkgfclnogjfeeiicmiijcnk", "mocihammffaleonaomjleikagemilaoj",
        "mogllbjhcnfhcejalaikleeogjmmfkdm", "nbmbpelgpalcgdghkeafabljjbhmalhf",
        "nchaailfhkbnlnaobgjmoamdfnclhdoo", "ncjeinjdknabbgmaijmnipndbggmchff",
        "ndlciiakjgfcefimfjlfcjgohlgidpnc", "neglbnegiidighiifljiphcldmgibifn",
        "nggdckgfnooehkpnmjdodbknekmhcdeg", "ngnfehahlmgkoclalhldboigojplccbl",
        "nhbpjehmiofogaicflcjhcfdmmkgbohp", "njimokpabbaelbbpohoflcbjhdgonbmf",
        "nkajjfpapgfhlcpmmoipafbfnnmojaep", "nlabdknfkecjaechkekdlkgnapljpfla",
        "nllaodpgkekajbabhjphhekenlnlpdmd", "nnbaaidlgckbmfdlnioepikbcmjmbadb",
        "nodfcenkjehpafmbmgnapoieilnoijap", "nonmahhknjgpnoamcdihefcbpdioimbh",
        "obdagonejiaflgbifdloghkllgdjpcdj", "ocilmpijebaopmdifcomolmpigakocmo",
        "ogmkkmobemflinkielcngdanaojkamcc", "ohcnbpfpchlcjchnljcdjebcjdbneecj",
        "ohhkknigpeehdnkeccopplflnodppkme", "oiihbhoknlbjonghmcplpbpkcdeinlfg",
        "olopfkdcklijkianjbegdegilmhdgpcj", "onjbjcnjheabeghbflckfekjnnjgfabn",
        "onmakacikbbnhmanodpjhijljadlpbda", "ookcfboifajbeojnnebiaoplifgiinof",
        "ooncphbdmekmhldbojgoboebnongkpic", "papnfbjjblebaaeenodbmllepiijfmhn",
        "pbegjfplhidokoonohceljofcpbojglg", "pbjakpdfjkmcajeobebemnjglbjiniln",
        "pdgppejghdoknllcnfikoaloobopajjo", "pecokcgeeiabdlkfkfjpmfldfhhjlmom",
        "pgkommhmfkkkfbbcnnfhlkagbodoimjm", "piebpodmljdloocefikhekfjajloneij",
        "pkmkhkpglegjekkfabaelkbgkfegnmde", "pppjaeenalohmnhjpemhdkkkghfddbfp",
    });

}  // namespace

namespace brave_ads {

bool IsValidLanguageComponentId(const std::string& id) {
  return kLanguageComponentIds.contains(id);
}

}  // namespace brave_ads
