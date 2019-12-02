/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_THIRD_PARTIES_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_THIRD_PARTIES_H_

#include <string>

namespace brave_perf_predictor {

const std::string static_third_party_config = R"--(
[
  {
    "name": "Google Analytics",
    "domains": [
      "www.google-analytics.com",
      "ssl.google-analytics.com",
      "google-analytics.com",
      "urchin.com"
    ]
  },
  {
    "name": "Facebook",
    "domains": [
      "www.facebook.com",
      "connect.facebook.net",
      "staticxx.facebook.com",
      "static.xx.fbcdn.net",
      "m.facebook.com",
      "atlassbx.com",
      "fbcdn-photos-e-a.akamaihd.net",
      "23.62.3.183",
      "akamai.net",
      "akamaiedge.net",
      "akamaitechnologies.com",
      "akamaitechnologies.fr",
      "akamaized.net",
      "edgefcs.net",
      "edgekey.net",
      "edgesuite.net",
      "srip.net",
      "cquotient.com",
      "demandware.net",
      "platform-lookaside.fbsbx.com"
    ]
  },
  {
    "name": "Google CDN",
    "domains": [
      "ajax.googleapis.com",
      "www.gstatic.com",
      "commondatastorage.googleapis.com"
    ]
  },
  {
    "name": "Google/Doubleclick Ads",
    "domains": [
      "pagead2.googlesyndication.com",
      "tpc.googlesyndication.com",
      "googleads.g.doubleclick.net",
      "securepubads.g.doubleclick.net",
      "cm.g.doubleclick.net",
      "s0.2mdn.net",
      "stats.g.doubleclick.net",
      "survey.g.doubleclick.net",
      "fls.doubleclick.net",
      "ad.doubleclick.net",
      "www.googleadservices.com",
      "adservice.google.se",
      "adservice.google.com",
      "adservice.google.de",
      "www.googletagservices.com"
    ]
  },
  {
    "name": "Google Tag Manager",
    "domains": [
      "www.googletagmanager.com"
    ]
  },
  {
    "name": "Other Google APIs/SDKs",
    "domains": [
      "www.google.com",
      "apis.google.com",
      "cse.google.com",
      "translate.googleapis.com",
      "storage.googleapis.com",
      "imasdk.googleapis.com",
      "lh3.googleusercontent.com",
      "calendar.google.com",
      "pay.google.com",
      "news.google.com",
      "payments.google.com",
      "clients2.google.com"
    ]
  },
  {
    "name": "Twitter",
    "domains": [
      "platform.twitter.com",
      "cdn.syndication.twimg.com",
      "twitpic.com",
      "vine.co"
    ]
  },
  {
    "name": "Yandex Metrica",
    "domains": [
      "mc.yandex.ru",
      "d31j93rd8oukbv.cloudfront.net"
    ]
  },
  {
    "name": "jQuery CDN",
    "domains": [
      "code.jquery.com"
    ]
  },
  {
    "name": "AddThis",
    "domains": [
      "s7.addthis.com",
      "addthiscdn.com",
      "addthisedge.com",
      "r.dlx.addthis.com",
      "su.addthis.com",
      "x.dlx.addthis.com"
    ]
  },
  {
    "name": "Google Maps",
    "domains": [
      "maps-api-ssl.google.com",
      "maps.google.com",
      "maps.gstatic.com",
      "maps.googleapis.com",
      "mts.googleapis.com"
    ]
  },
  {
    "name": "Hotjar",
    "domains": [
      "script.hotjar.com",
      "static.hotjar.com",
      "in.hotjar.com",
      "vc.hotjar.io",
      "vars.hotjar.com"
    ]
  },
  {
    "name": "Cloudflare CDN",
    "domains": [
      "cdnjs.cloudflare.com",
      "amp.cloudflare.com"
    ]
  },
  {
    "name": "WordPress",
    "domains": [
      "s0.wp.com",
      "s2.wp.com",
      "s.w.org",
      "wordpress.com",
      "stats.wp.com"
    ]
  },
  {
    "name": "Shopify",
    "domains": [
      "cdn.shopify.com",
      "productreviews.shopifycdn.com",
      "shopifyapps.com"
    ]
  },
  {
    "name": "Criteo",
    "domains": [
      "static.criteo.net",
      "bidder.criteo.com",
      "emailretargeting.com"
    ]
  },
  {
    "name": "ZenDesk",
    "domains": [
      "assets.zendesk.com",
      "static.zdassets.com",
      "v2.zopim.com"
    ]
  },
  {
    "name": "Tawk.to",
    "domains": [
      "embed.tawk.to"
    ]
  },
  {
    "name": "AMP",
    "domains": [
      "cdn.ampproject.org"
    ]
  },
  {
    "name": "Wix",
    "domains": [
      "static.parastorage.com",
      "static.wixstatic.com",
      "www.wix.com"
    ]
  },
  {
    "name": "Squarespace",
    "domains": [
      "static.squarespace.com",
      "static1.squarespace.com"
    ]
  },
  {
    "name": "YouTube",
    "domains": [
      "www.youtube.com",
      "s.ytimg.com",
      "yt3.ggpht.com",
      "youtube-nocookie.com"
    ]
  },
  {
    "name": "Jivochat",
    "domains": [
      "cdn-ca.jivosite.com",
      "code.jivosite.com"
    ]
  },
  {
    "name": "Amazon Web Services",
    "domains": [
      "s3.amazonaws.com",
      "amazonwebservices.com",
      "api-cdn.amazon.com",
      "ecx.images-amazon.com",
      "elasticbeanstalk.com",
      "amazonwebapps.com",
      "ws.amazon.co.uk",
      "payments-amazon.com"
    ]
  },
  {
    "name": "Adobe Tag Manager",
    "domains": [
      "assets.adobedtm.com",
      "dpm.demdex.net",
      "sync-tm.everesttech.net"
    ]
  },
  {
    "name": "PIXNET",
    "domains": [
      "front.pixfs.net",
      "falcon-asset.pixfs.net",
      "pixgame-asset.pixfs.net"
    ]
  },
  {
    "name": "JSDelivr CDN",
    "domains": [
      "cdn.jsdelivr.net"
    ]
  },
  {
    "name": "Yandex Ads",
    "domains": [
      "an.yandex.ru"
    ]
  },
  {
    "name": "Yandex Share",
    "domains": [
      "yastatic.net"
    ]
  },
  {
    "name": "Yandex APIs",
    "domains": [
      "api-maps.yandex.ru",
      "money.yandex.ru"
    ]
  },
  {
    "name": "Salesforce",
    "domains": [
      "cdn.krxd.net",
      "beacon.krxd.net",
      "consumer.krxd.net",
      "usermatch.krxd.net"
    ]
  },
  {
    "name": "Sumo",
    "domains": [
      "sumo.b-cdn.net",
      "load.sumo.com",
      "load.sumome.com"
    ]
  },
  {
    "name": "Beeketing",
    "domains": [
      "sdk-cdn.beeketing.com",
      "sdk.beeketing.com"
    ]
  },
  {
    "name": "FontAwesome CDN",
    "domains": [
      "use.fontawesome.com"
    ]
  },
  {
    "name": "ShareThis",
    "domains": [
      "w.sharethis.com",
      "ws.sharethis.com",
      "t.sharethis.com"
    ]
  },
  {
    "name": "Hatena Blog",
    "domains": [
      "cdn.blog.st-hatena.com",
      "cdn.pool.st-hatena.com",
      "cdn7.www.st-hatena.com",
      "s.hatena.ne.jp",
      "b.st-hatena.com"
    ]
  },
  {
    "name": "Mailchimp",
    "domains": [
      "downloads.mailchimp.com",
      "chimpstatic.com",
      "list-manage.com"
    ]
  },
  {
    "name": "Amazon Ads",
    "domains": [
      "s.amazon-adsystem.com",
      "c.amazon-adsystem.com",
      "aax.amazon-adsystem.com",
      "z-na.amazon-adsystem.com"
    ]
  },
  {
    "name": "Sentry",
    "domains": [
      "cdn.ravenjs.com",
      "browser.sentry-cdn.com",
      "getsentry.com"
    ]
  },
  {
    "name": "Pinterest",
    "domains": [
      "assets.pinterest.com",
      "pinimg.com",
      "ct.pinterest.com"
    ]
  },
  {
    "name": "Hubspot",
    "domains": [
      "forms.hubspot.com",
      "js.hsforms.net",
      "js.hs-analytics.net",
      "hscollectedforms.net",
      "hscta.net",
      "hsleadflows.net",
      "js.leadin.com",
      "hs-scripts.com",
      "hsstatic.net",
      "hubspot.net"
    ]
  },
  {
    "name": "LinkedIn",
    "domains": [
      "platform.linkedin.com",
      "bizographics.com",
      "slideshare.com",
      "slidesharecdn.com"
    ]
  },
  {
    "name": "Taboola",
    "domains": [
      "cdn.taboola.com",
      "trc.taboola.com",
      "vidstat.taboola.com",
      "taboolasyndication.com"
    ]
  },
  {
    "name": "Intercom",
    "domains": [
      "js.intercomcdn.com"
    ]
  },
  {
    "name": "Histats",
    "domains": [
      "s10.histats.com"
    ]
  },
  {
    "name": "Optimizely",
    "domains": [
      "cdn.optimizely.com",
      "cdn-pci.optimizely.com",
      "logx.optimizely.com",
      "cdn3.optimizely.com"
    ]
  },
  {
    "name": "Moat",
    "domains": [
      "z.moatads.com",
      "moatpixel.com",
      "px.moatads.com",
      "geo.moatads.com",
      "sejs.moatads.com",
      "mb.moatads.com",
      "v4.moatads.com"
    ]
  },
  {
    "name": "Tealium",
    "domains": [
      "tags.tiqcdn.com",
      "tealium.hs.llnwd.net",
      "aniview.com",
      "delvenetworks.com",
      "link.videoplatform.limelight.com"
    ]
  },
  {
    "name": "Distil Networks",
    "domains": [
      "n-cdn.areyouahuman.com"
    ]
  },
  {
    "name": "Scorecard Research",
    "domains": [
      "sb.scorecardresearch.com"
    ]
  },
  {
    "name": "Blogger",
    "domains": [
      "1.bp.blogspot.com",
      "www.blogger.com",
      "images-blogger-opensocial.googleusercontent.com"
    ]
  },
  {
    "name": "Wistia",
    "domains": [
      "fast.wistia.com",
      "fast.wistia.net"
    ]
  },
  {
    "name": "LiveChat",
    "domains": [
      "cdn.livechatinc.com",
      "secure.livechatinc.com"
    ]
  },
  {
    "name": "Adobe TypeKit",
    "domains": [
      "use.typekit.net",
      "typekit.com",
      "p.typekit.net"
    ]
  },
  {
    "name": "Shareaholic",
    "domains": [
      "dsms0mj1bbhn4.cloudfront.net",
      "shareaholic.com"
    ]
  },
  {
    "name": "OneSignal",
    "domains": [
      "cdn.onesignal.com"
    ]
  },
  {
    "name": "Cookiebot",
    "domains": [
      "consent.cookiebot.com"
    ]
  },
  {
    "name": "Tumblr",
    "domains": [
      "assets.tumblr.com",
      "static.tumblr.com"
    ]
  },
  {
    "name": "Cloudflare",
    "domains": [
      "ajax.cloudflare.com"
    ]
  },
  {
    "name": "Integral Ad Science",
    "domains": [
      "pixel.adsafeprotected.com",
      "static.adsafeprotected.com",
      "fw.adsafeprotected.com",
      "cdn.adsafeprotected.com",
      "dt.adsafeprotected.com",
      "iasds01.com"
    ]
  },
  {
    "name": "PayPal",
    "domains": [
      "www.paypalobjects.com",
      "paypal.com"
    ]
  },
  {
    "name": "Segment",
    "domains": [
      "cdn.segment.com",
      "api.segment.io"
    ]
  },
  {
    "name": "Baidu Analytics",
    "domains": [
      "hm.baidu.com"
    ]
  },
  {
    "name": "Dealer",
    "domains": [
      "static.dealer.com"
    ]
  },
  {
    "name": "Olark",
    "domains": [
      "static.olark.com"
    ]
  },
  {
    "name": "VK",
    "domains": [
      "vk.com"
    ]
  },
  {
    "name": "OpenX",
    "domains": [
      "uk-ads.openx.net",
      "us-ads.openx.net",
      "33across-d.openx.net",
      "rtb.openx.net",
      "us-u.openx.net",
      "eu-u.openx.net",
      "u.openx.net",
      "deliverimp.com",
      "jump-time.net",
      "openxadexchange.com",
      "servedbyopenx.com"
    ]
  },
  {
    "name": "Lucky Orange",
    "domains": [
      "d10lpsik1i8c69.cloudfront.net",
      "luckyorange.com",
      "luckyorange.net"
    ]
  },
  {
    "name": "OptinMonster",
    "domains": [
      "a.optmstr.com",
      "api.opmnstr.com",
      "a.optmnstr.com"
    ]
  },
  {
    "name": "Snapchat",
    "domains": [
      "tr.snapchat.com",
      "sc-static.net"
    ]
  },
  {
    "name": "33 Across",
    "domains": [
      "sic.33across.com",
      "cdn-sic.33across.com"
    ]
  },
  {
    "name": "Ensighten",
    "domains": [
      "nexus.ensighten.com"
    ]
  },
  {
    "name": "WordAds",
    "domains": [
      "s.pubmine.com"
    ]
  },
  {
    "name": "Snowplow",
    "domains": [
      "d32hwlnfiv2gyn.cloudfront.net"
    ]
  },
  {
    "name": "Brightcove",
    "domains": [
      "vjs.zencdn.net",
      "players.brightcove.net",
      "brightcove.com"
    ]
  },
  {
    "name": "Drift",
    "domains": [
      "js.driftt.com",
      "api.drift.com"
    ]
  },
  {
    "name": "Microsoft Hosted Libs",
    "domains": [
      "ajax.aspnetcdn.com"
    ]
  },
  {
    "name": "Stripe",
    "domains": [
      "m.stripe.network",
      "js.stripe.com",
      "stripecdn.com"
    ]
  },
  {
    "name": "Parking Crew",
    "domains": [
      "d1lxhc4jvstzrp.cloudfront.net",
      "parkingcrew.net"
    ]
  },
  {
    "name": "Popads",
    "domains": [
      "serve.popads.net",
      "c1.popads.net"
    ]
  },
  {
    "name": "DigiTrust",
    "domains": [
      "cdn.digitru.st"
    ]
  },
  {
    "name": "Mixpanel",
    "domains": [
      "cdn.mxpnl.com",
      "mixpanel.com"
    ]
  },
  {
    "name": "MediaVine",
    "domains": [
      "scripts.mediavine.com",
      "video.mediavine.com"
    ]
  },
  {
    "name": "FullStory",
    "domains": [
      "fullstory.com",
      "rs.fullstory.com"
    ]
  },
  {
    "name": "Sizmek",
    "domains": [
      "secure-ds.serving-sys.com",
      "ds.serving-sys.com",
      "peer39.net",
      "bs.serving-sys.com"
    ]
  },
  {
    "name": "CDK Dealer Management",
    "domains": [
      "media-cf.assets-cdk.com"
    ]
  },
  {
    "name": "Quantcast",
    "domains": [
      "pixel.quantserve.com",
      "secure.quantserve.com",
      "rules.quantcount.com",
      "brtstats.com",
      "ntv.io",
      "semantictec.com"
    ]
  },
  {
    "name": "Pubmatic",
    "domains": [
      "image6.pubmatic.com",
      "ads.pubmatic.com",
      "image2.pubmatic.com",
      "simage2.pubmatic.com",
      "image4.pubmatic.com",
      "simage4.pubmatic.com"
    ]
  },
  {
    "name": "RD Station",
    "domains": [
      "d335luupugsy2.cloudfront.net"
    ]
  },
  {
    "name": "MGID",
    "domains": [
      "servicer.mgid.com",
      "dt07.net"
    ]
  },
  {
    "name": "Media.net",
    "domains": [
      "contextual.media.net",
      "mnet-ad.net"
    ]
  },
  {
    "name": "New Relic",
    "domains": [
      "js-agent.newrelic.com",
      "bam.nr-data.net"
    ]
  },
  {
    "name": "Rubicon Project",
    "domains": [
      "pixel.rubiconproject.com",
      "fastlane.rubiconproject.com",
      "secure-assets.rubiconproject.com",
      "eus.rubiconproject.com",
      "pixel-us-east.rubiconproject.com",
      "token.rubiconproject.com",
      "ads.rubiconproject.com",
      "chango.com",
      "fimserve.com"
    ]
  },
  {
    "name": "VWO",
    "domains": [
      "dev.visualwebsiteoptimizer.com"
    ]
  },
  {
    "name": "Keen",
    "domains": [
      "d26b395fwzu5fz.cloudfront.net",
      "keen.io"
    ]
  },
  {
    "name": "Adroll",
    "domains": [
      "d.adroll.com",
      "s.adroll.com"
    ]
  },
  {
    "name": "Unpkg",
    "domains": [
      "unpkg.com"
    ]
  },
  {
    "name": "Parse.ly",
    "domains": [
      "d1z2jf7jlzjs58.cloudfront.net",
      "parsely.com"
    ]
  },
  {
    "name": "Searchanise",
    "domains": [
      "www.searchanise.com"
    ]
  },
  {
    "name": "AppNexus",
    "domains": [
      "acdn.adnxs.com",
      "secure.adnxs.com",
      "ctasnet.com",
      "ib.adnxs.com"
    ]
  },
  {
    "name": "uLogin",
    "domains": [
      "ulogin.ru"
    ]
  },
  {
    "name": "fam",
    "domains": [
      "fam-ad.com",
      "img.fam-ad.com"
    ]
  },
  {
    "name": "Yandex CDN",
    "domains": [
      "yandex.st"
    ]
  },
  {
    "name": "Albacross",
    "domains": [
      "serve.albacross.com"
    ]
  },
  {
    "name": "CreateJS CDN",
    "domains": [
      "code.createjs.com"
    ]
  },
  {
    "name": "Help Scout",
    "domains": [
      "djtflbt20bdde.cloudfront.net",
      "beacon-v2.helpscout.net"
    ]
  },
  {
    "name": "AppDynamics",
    "domains": [
      "cdn.appdynamics.com",
      "d3tjaysgumg9lf.cloudfront.net",
      "eum-appdynamics.com"
    ]
  },
  {
    "name": "Siteimprove",
    "domains": [
      "siteimprove.com",
      "siteimproveanalytics.com"
    ]
  },
  {
    "name": "DoubleVerify",
    "domains": [
      "cdn.doubleverify.com",
      "rtb0.doubleverify.com",
      "rtbcdn.doubleverify.com",
      "dvtps.com",
      "tm.iqfp1.com"
    ]
  },
  {
    "name": "AOL / Oath / Verizon Media",
    "domains": [
      "pixel.advertising.com",
      "dtm.advertising.com",
      "tag.sp.advertising.com",
      "service.sp.advertising.com",
      "adtech.advertising.com",
      "aol.co.uk",
      "aol.com",
      "aolcdn.com",
      "blogsmithmedia.com",
      "mighty.aol.net",
      "tacoda.net",
      "consent.cmp.oath.com"
    ]
  },
  {
    "name": "Alexa",
    "domains": [
      "d31qbv1cthcecs.cloudfront.net"
    ]
  },
  {
    "name": "SessionCam",
    "domains": [
      "d2oh4tlt9mrke9.cloudfront.net",
      "sessioncam.com"
    ]
  },
  {
    "name": "Foursixty",
    "domains": [
      "foursixty.com"
    ]
  },
  {
    "name": "Listrak",
    "domains": [
      "cdn.listrakbi.com",
      "s1.listrakbi.com",
      "listrak.com"
    ]
  },
  {
    "name": "mPulse",
    "domains": [
      "c.go-mpulse.net",
      "0211c83c.akstat.io",
      "mpstat.us",
      "mpulse.net"
    ]
  },
  {
    "name": "Opentag",
    "domains": [
      "d3c3cq33003psk.cloudfront.net",
      "opentag-stats.qutics.com"
    ]
  },
  {
    "name": "Market GID",
    "domains": [
      "jsc.marketgid.com"
    ]
  },
  {
    "name": "Freshdesk",
    "domains": [
      "d36mpcpuzc4ztk.cloudfront.net"
    ]
  },
  {
    "name": "IBM Digital Analytics",
    "domains": [
      "cmcore.com",
      "coremetrics.com",
      "data.coremetrics.com",
      "data.coremetrics.eu",
      "data.de.coremetrics.com",
      "iocdn.coremetrics.com",
      "libs.coremetrics.com",
      "libs.de.coremetrics.com",
      "s81c.com",
      "tmscdn.coremetrics.com",
      "tmscdn.de.coremetrics.com",
      "unica.com"
    ]
  },
  {
    "name": "Usabilla",
    "domains": [
      "w.usabilla.com",
      "d6tizftlrpuof.cloudfront.net"
    ]
  },
  {
    "name": "Bugsnag",
    "domains": [
      "d2wy8f7a9ursnm.cloudfront.net",
      "notify.bugsnag.com"
    ]
  },
  {
    "name": "AddShoppers",
    "domains": [
      "addshoppers.com",
      "d3rr3d0n31t48m.cloudfront.net",
      "shop.pe"
    ]
  },
  {
    "name": "Po.st",
    "domains": [
      "po.st"
    ]
  },
  {
    "name": "Disqus",
    "domains": [
      "c.disquscdn.com",
      "disqus.com"
    ]
  },
  {
    "name": "PhotoBucket",
    "domains": [
      "photobucket.com"
    ]
  },
  {
    "name": "GitHub",
    "domains": [
      "cdn.rawgit.com"
    ]
  },
  {
    "name": "Bootstrap CDN",
    "domains": [
      "maxcdn.bootstrapcdn.com",
      "stackpath.bootstrapcdn.com"
    ]
  },
  {
    "name": "Radar",
    "domains": [
      "radar.cedexis.com",
      "rpt.cedexis.com",
      "2-01-49cd-0002.cdx.cedexis.net",
      "a-cedexis.msedge.net",
      "bench.cedexis-test.com",
      "cedexis-radar.net",
      "cedexis-test01.insnw.net",
      "cedexis.leasewebcdn.com",
      "cedexisakamaitest.azureedge.net",
      "cedexispub.cdnetworks.net",
      "cs600.wac.alphacdn.net",
      "cs600.wpc.edgecastdns.net",
      "global2.cmdolb.com",
      "img-cedexis.mncdn.com",
      "zn3vgszfh.fastestcdn.net",
      "edgecastcdn.net"
    ]
  },
  {
    "name": "SnapWidget",
    "domains": [
      "snapwidget.com"
    ]
  },
  {
    "name": "Media Math",
    "domains": [
      "mathid.mathtag.com",
      "mathads.com",
      "sync.mathtag.com",
      "pixel.mathtag.com"
    ]
  },
  {
    "name": "ClickDesk",
    "domains": [
      "clickdesk.com",
      "d1gwclp1pmzk26.cloudfront.net"
    ]
  },
  {
    "name": "Google Plus",
    "domains": [
      "plus.google.com"
    ]
  },
  {
    "name": "LinkedIn Ads",
    "domains": [
      "ads.linkedin.com",
      "snap.licdn.com",
      "www.linkedin.com"
    ]
  },
  {
    "name": "Madison Logic",
    "domains": [
      "ml314.com"
    ]
  },
  {
    "name": "Smarter Click",
    "domains": [
      "smarterclick.co.uk",
      "smct.co"
    ]
  },
  {
    "name": "Bing Ads",
    "domains": [
      "bat.bing.com",
      "c.bing.com",
      "bat.r.msn.com",
      "ajax.microsoft.com",
      "msads.net",
      "msecnd.net",
      "s-msft.com",
      "s-msn.com",
      "windows.net"
    ]
  },
  {
    "name": "Fresh Relevance",
    "domains": [
      "d1y9qtn9cuc3xw.cloudfront.ne",
      "d1y9qtn9cuc3xw.cloudfront.net",
      "d81mfvml8p5ml.cloudfront.net",
      "dkpklk99llpj0.cloudfront.net",
      "freshrelevance.com"
    ]
  },
  {
    "name": "Browsealoud",
    "domains": [
      "www.browsealoud.com",
      "texthelp.com"
    ]
  },
  {
    "name": "Qubit Deliver",
    "domains": [
      "d1m54pdnjzjnhe.cloudfront.net",
      "d22rutvoghj3db.cloudfront.net",
      "dd6zx4ibq538k.cloudfront.net"
    ]
  },
  {
    "name": "Crazy Egg",
    "domains": [
      "dnn506yrbagrg.cloudfront.net",
      "cetrk.com",
      "crazyegg.com",
      "hellobar.com"
    ]
  },
  {
    "name": "Vox Media",
    "domains": [
      "cdn.vox-cdn.com",
      "voxmedia.com"
    ]
  },
  {
    "name": "SaleCycle",
    "domains": [
      "d16fk4ms6rqz1v.cloudfront.net",
      "d22j4fzzszoii2.cloudfront.net",
      "d30ke5tqu2tkyx.cloudfront.net",
      "salecycle.com"
    ]
  },
  {
    "name": "Sidecar",
    "domains": [
      "d3v27wwd40f0xu.cloudfront.net",
      "getsidecar.com"
    ]
  },
  {
    "name": "Concert",
    "domains": [
      "cdn.concert.io"
    ]
  },
  {
    "name": "Kaizen Platform",
    "domains": [
      "cdn.kaizenplatform.net",
      "log-v4.kaizenplatform.net"
    ]
  },
  {
    "name": "unpkg",
    "domains": [
      "npmcdn.com"
    ]
  },
  {
    "name": "Edge Web Fonts",
    "domains": [
      "use.edgefonts.net"
    ]
  },
  {
    "name": "The Trade Desk",
    "domains": [
      "js.adsrvr.org",
      "match.adsrvr.org",
      "d1eoo1tco6rr5e.cloudfront.net",
      "snap.adsrvr.org"
    ]
  },
  {
    "name": "Ipify",
    "domains": [
      "api.ipify.org",
      "geo.ipify.org"
    ]
  },
  {
    "name": "Permutive",
    "domains": [
      "d3alqb8vzo7fun.cloudfront.net",
      "permutive.com"
    ]
  },
  {
    "name": "Pingdom RUM",
    "domains": [
      "rum-static.pingdom.net",
      "rum-collector-2.pingdom.net"
    ]
  },
  {
    "name": "Clicktripz",
    "domains": [
      "static.clicktripz.com",
      "www.clicktripz.com"
    ]
  },
  {
    "name": "Talkable",
    "domains": [
      "d2jjzw81hqbuqv.cloudfront.net",
      "www.talkable.com"
    ]
  },
  {
    "name": "GoSquared",
    "domains": [
      "data.gosquared.com",
      "d1l6p2sc9645hc.cloudfront.net",
      "data2.gosquared.com"
    ]
  },
  {
    "name": "Republer",
    "domains": [
      "sync.republer.com"
    ]
  },
  {
    "name": "TrackJS",
    "domains": [
      "d2zah9y47r7bi2.cloudfront.net",
      "usage.trackjs.com"
    ]
  },
  {
    "name": "Vimeo",
    "domains": [
      "f.vimeocdn.com",
      "player.vimeo.com"
    ]
  },
  {
    "name": "Yieldify",
    "domains": [
      "d33wq5gej88ld6.cloudfront.net",
      "dwmvwp56lzq5t.cloudfront.net",
      "geo.yieldifylabs.com",
      "yieldify.com"
    ]
  },
  {
    "name": "DialogTech SourceTrak",
    "domains": [
      "d31y97ze264gaa.cloudfront.net"
    ]
  },
  {
    "name": "Twitter Online Conversion Tracking",
    "domains": [
      "static.ads-twitter.com",
      "analytics.twitter.com"
    ]
  },
  {
    "name": "CleverTap",
    "domains": [
      "d2r1yp2w7bby2u.cloudfront.net"
    ]
  },
  {
    "name": "Omniconvert",
    "domains": [
      "d2tgfbvjf3q6hn.cloudfront.net",
      "d3vbj265bmdenw.cloudfront.net",
      "omniconvert.com"
    ]
  },
  {
    "name": "Underdog Media",
    "domains": [
      "udmserve.net",
      "underdog.media"
    ]
  },
  {
    "name": "[24]7",
    "domains": [
      "247-inc.net",
      "247inc.net",
      "d1af033869koo7.cloudfront.net"
    ]
  },
  {
    "name": "Reflektion",
    "domains": [
      "d26opx5dl8t69i.cloudfront.net",
      "reflektion.com"
    ]
  },
  {
    "name": "Auto Link Maker",
    "domains": [
      "autolinkmaker.itunes.apple.com"
    ]
  },
  {
    "name": "Friendbuy",
    "domains": [
      "djnf6e5yyirys.cloudfront.net",
      "friendbuy.com"
    ]
  },
  {
    "name": "Opinion Stage",
    "domains": [
      "www.opinionstage.com"
    ]
  },
  {
    "name": "LongTail Ad Solutions",
    "domains": [
      "jwpcdn.com",
      "jwplatform.com",
      "jwplayer.com",
      "jwpltx.com",
      "jwpsrv.com",
      "longtailvideo.com"
    ]
  },
  {
    "name": "Marketo",
    "domains": [
      "munchkin.marketo.net",
      "marketo.com",
      "mktoresp.com"
    ]
  },
  {
    "name": "Sourcepoint",
    "domains": [
      "d2lv4zbk7v5f93.cloudfront.net",
      "d3qxwzhswv93jk.cloudfront.net",
      "www.decenthat.com",
      "www.fallingfalcon.com"
    ]
  },
  {
    "name": "Net Reviews",
    "domains": [
      "www.avis-verifies.com"
    ]
  },
  {
    "name": "Tag Inspector",
    "domains": [
      "d22xmn10vbouk4.cloudfront.net"
    ]
  },
  {
    "name": "Polldaddy",
    "domains": [
      "polldaddy.com"
    ]
  },
  {
    "name": "Freespee",
    "domains": [
      "analytics.freespee.com"
    ]
  },
  {
    "name": "KISSmetrics",
    "domains": [
      "doug1izaerwt3.cloudfront.net",
      "dsyszv14g9ymi.cloudfront.net",
      "kissmetrics.com"
    ]
  },
  {
    "name": "Adthink",
    "domains": [
      "d.adxcore.com",
      "dcoengine.com"
    ]
  },
  {
    "name": "Extole",
    "domains": [
      "extole.com",
      "origin.extole.io"
    ]
  },
  {
    "name": "AnswerDash",
    "domains": [
      "p1.answerdash.com"
    ]
  },
  {
    "name": "Cookie-Script.com",
    "domains": [
      "cookie-script.com"
    ]
  },
  {
    "name": "Fastly Insights",
    "domains": [
      "www.fastly-insights.com"
    ]
  },
  {
    "name": "Amplitude Mobile Analytics",
    "domains": [
      "amplitude.com",
      "d24n15hnbwhuhn.cloudfront.net"
    ]
  },
  {
    "name": "MLveda",
    "domains": [
      "www.mlveda.com"
    ]
  },
  {
    "name": "CNET Content Solutions",
    "domains": [
      "cdn.cnetcontent.com",
      "cnetcontent.com",
      "ws.cnetcontent.com"
    ]
  },
  {
    "name": "Browser-Update.org",
    "domains": [
      "browser-update.org"
    ]
  },
  {
    "name": "Triblio",
    "domains": [
      "tribl.io"
    ]
  },
  {
    "name": "Fonecall",
    "domains": [
      "web-call-analytics.com"
    ]
  },
  {
    "name": "LoyaltyLion",
    "domains": [
      "dg1f2pfrgjxdq.cloudfront.net",
      "loyaltylion.com"
    ]
  },
  {
    "name": "StatCounter",
    "domains": [
      "statcounter.com"
    ]
  },
  {
    "name": "Curalate",
    "domains": [
      "curalate.com",
      "d116tqlcqfmz3v.cloudfront.net"
    ]
  },
  {
    "name": "piano",
    "domains": [
      "tinypass.com",
      "www.npttech.com"
    ]
  },
  {
    "name": "UpSellit",
    "domains": [
      "www.upsellit.com"
    ]
  },
  {
    "name": "Soundest",
    "domains": [
      "soundest.net",
      "soundestlink.com"
    ]
  },
  {
    "name": "Micropat",
    "domains": [
      "addtoany.com"
    ]
  },
  {
    "name": "Weebly",
    "domains": [
      "editmysite.com"
    ]
  },
  {
    "name": "Tynt",
    "domains": [
      "tynt.com"
    ]
  },
  {
    "name": "Datacamp",
    "domains": [
      "cdn77.org"
    ]
  },
  {
    "name": "Treasure Data",
    "domains": [
      "treasuredata.com"
    ]
  },
  {
    "name": "Nielsen NetRatings SiteCensus",
    "domains": [
      "imrworldwide.com"
    ]
  },
  {
    "name": "iubenda",
    "domains": [
      "www.iubenda.com"
    ]
  },
  {
    "name": "Yotpo",
    "domains": [
      "yotpo.com"
    ]
  },
  {
    "name": "Privy",
    "domains": [
      "privy.com",
      "privymktg.com"
    ]
  },
  {
    "name": "VigLink",
    "domains": [
      "viglink.com"
    ]
  },
  {
    "name": "Kakao",
    "domains": [
      "daum.net",
      "daumcdn.net"
    ]
  },
  {
    "name": "Chartbeat",
    "domains": [
      "chartbeat.com",
      "chartbeat.net"
    ]
  },
  {
    "name": "Klaviyo",
    "domains": [
      "klaviyo.com"
    ]
  },
  {
    "name": "BrightTag / Signal",
    "domains": [
      "btstatic.com",
      "thebrighttag.com"
    ]
  },
  {
    "name": "fluct",
    "domains": [
      "adingo.jp"
    ]
  },
  {
    "name": "AudienceSearch",
    "domains": [
      "im-apps.net"
    ]
  },
  {
    "name": "Inspectlet",
    "domains": [
      "inspectlet.com"
    ]
  },
  {
    "name": "Skimbit",
    "domains": [
      "redirectingat.com",
      "skimresources.com",
      "skimresources.net"
    ]
  },
  {
    "name": "DTSCOUT",
    "domains": [
      "dtscout.com"
    ]
  },
  {
    "name": "Rambler",
    "domains": [
      "rambler.ru"
    ]
  },
  {
    "name": "Bigcommerce",
    "domains": [
      "bigcommerce.com"
    ]
  },
  {
    "name": "Tidio Live Chat",
    "domains": [
      "tidiochat.com"
    ]
  },
  {
    "name": "CallRail",
    "domains": [
      "callrail.com"
    ]
  },
  {
    "name": "Teads",
    "domains": [
      "teads.tv"
    ]
  },
  {
    "name": "Instagram",
    "domains": [
      "scontent.cdninstagram.com",
      "instagram.com"
    ]
  },
  {
    "name": "Fastly",
    "domains": [
      "fastly.net"
    ]
  },
  {
    "name": "MailMunch",
    "domains": [
      "mailmunch.co"
    ]
  },
  {
    "name": "LivePerson",
    "domains": [
      "liveperson.com",
      "liveperson.net",
      "look.io",
      "lpsnmedia.net"
    ]
  },
  {
    "name": "Monotype",
    "domains": [
      "fonts.com",
      "fonts.net"
    ]
  },
  {
    "name": "Outbrain",
    "domains": [
      "outbrain.com",
      "visualrevenue.com"
    ]
  },
  {
    "name": "Pure Chat",
    "domains": [
      "purechat.com"
    ]
  },
  {
    "name": "LiveJournal",
    "domains": [
      "livejournal.com",
      "livejournal.net"
    ]
  },
  {
    "name": "PushCrew",
    "domains": [
      "pushcrew.com"
    ]
  },
  {
    "name": "Index Exchange",
    "domains": [
      "casalemedia.com",
      "indexww.com"
    ]
  },
  {
    "name": "StickyADS.tv",
    "domains": [
      "stickyadstv.com"
    ]
  },
  {
    "name": "GumGum",
    "domains": [
      "gumgum.com"
    ]
  },
  {
    "name": "AB Tasty",
    "domains": [
      "abtasty.com",
      "d1447tq2m68ekg.cloudfront.net"
    ]
  },
  {
    "name": "Trust Pilot",
    "domains": [
      "trustpilot.com"
    ]
  },
  {
    "name": "Embedly",
    "domains": [
      "embed.ly",
      "embedly.com"
    ]
  },
  {
    "name": "Adform",
    "domains": [
      "adform.net",
      "adformdsp.net"
    ]
  },
  {
    "name": "sovrn",
    "domains": [
      "lijit.com"
    ]
  },
  {
    "name": "iPerceptions",
    "domains": [
      "iperceptions.com"
    ]
  },
  {
    "name": "Cxense",
    "domains": [
      "cxense.com",
      "cxpublic.com",
      "emediate.dk",
      "emediate.eu"
    ]
  },
  {
    "name": "Bold Commerce",
    "domains": [
      "boldapps.net",
      "shappify-cdn.com",
      "shappify.com"
    ]
  },
  {
    "name": "Marchex",
    "domains": [
      "marchex.io",
      "voicestar.com"
    ]
  },
  {
    "name": "BlueKai",
    "domains": [
      "bkrtx.com",
      "bluekai.com"
    ]
  },
  {
    "name": "PubNation",
    "domains": [
      "pubnation.com"
    ]
  },
  {
    "name": "Infolinks",
    "domains": [
      "infolinks.com"
    ]
  },
  {
    "name": "Ezoic",
    "domains": [
      "ezoic.net"
    ]
  },
  {
    "name": "Sharethrough",
    "domains": [
      "sharethrough.com"
    ]
  },
  {
    "name": "Ve",
    "domains": [
      "veinteractive.com"
    ]
  },
  {
    "name": "Roxr Software",
    "domains": [
      "getclicky.com"
    ]
  },
  {
    "name": "LiveTex",
    "domains": [
      "livetex.ru"
    ]
  },
  {
    "name": "Crowd Control",
    "domains": [
      "crwdcntrl.net"
    ]
  },
  {
    "name": "Digital ad Consortium",
    "domains": [
      "impact-ad.jp"
    ]
  },
  {
    "name": "GetSiteControl",
    "domains": [
      "getsitecontrol.com"
    ]
  },
  {
    "name": "Mapbox",
    "domains": [
      "mapbox.com"
    ]
  },
  {
    "name": "Adloox",
    "domains": [
      "adlooxtracking.com"
    ]
  },
  {
    "name": "Nosto",
    "domains": [
      "nosto.com"
    ]
  },
  {
    "name": "Gigya",
    "domains": [
      "gigya.com"
    ]
  },
  {
    "name": "Heap",
    "domains": [
      "heapanalytics.com"
    ]
  },
  {
    "name": "ExoClick",
    "domains": [
      "exoclick.com"
    ]
  },
  {
    "name": "OpenTable",
    "domains": [
      "opentable.co.uk",
      "opentable.com",
      "www.toptable.co.uk"
    ]
  },
  {
    "name": "iBillboard",
    "domains": [
      "ibillboard.com"
    ]
  },
  {
    "name": "Microad",
    "domains": [
      "microad.jp"
    ]
  },
  {
    "name": "Rakuten Marketing",
    "domains": [
      "rakuten-static.com",
      "rmtag.com"
    ]
  },
  {
    "name": "JuicyAds",
    "domains": [
      "juicyads.com"
    ]
  },
  {
    "name": "Mouseflow",
    "domains": [
      "mouseflow.com"
    ]
  },
  {
    "name": "Swiftype",
    "domains": [
      "swiftype.com",
      "swiftypecdn.com"
    ]
  },
  {
    "name": "Yahoo!",
    "domains": [
      "ads.yahoo.com",
      "analytics.yahoo.com",
      "bluelithium.com",
      "hostingprod.com",
      "lexity.com",
      "yahoo.net",
      "yahooapis.com",
      "yimg.com",
      "zenfs.com",
      "geo.yahoo.com"
    ]
  },
  {
    "name": "etracker",
    "domains": [
      "etracker.de",
      "www.etracker.com"
    ]
  },
  {
    "name": "Accuweather",
    "domains": [
      "accuweather.com"
    ]
  },
  {
    "name": "Feefo.com",
    "domains": [
      "feefo.com"
    ]
  },
  {
    "name": "Smart AdServer",
    "domains": [
      "sasqos.com",
      "smartadserver.com"
    ]
  },
  {
    "name": "Medium",
    "domains": [
      "medium.com"
    ]
  },
  {
    "name": "Trusted Shops",
    "domains": [
      "trustedshops.com"
    ]
  },
  {
    "name": "Constant Contact",
    "domains": [
      "ctctcdn.com"
    ]
  },
  {
    "name": "AdMatic",
    "domains": [
      "admatic.com.tr"
    ]
  },
  {
    "name": "Unbounce",
    "domains": [
      "d2xxq4ijfwetlm.cloudfront.net",
      "d9hhrg4mnvzow.cloudfront.net",
      "ubembed.com",
      "unbounce.com"
    ]
  },
  {
    "name": "Evidon",
    "domains": [
      "evidon.com"
    ]
  },
  {
    "name": "SmartAdServer",
    "domains": [
      "sascdn.com",
      "securite.01net.com"
    ]
  },
  {
    "name": "Gemius",
    "domains": [
      "gemius.pl"
    ]
  },
  {
    "name": "SocialShopWave",
    "domains": [
      "socialshopwave.com"
    ]
  },
  {
    "name": "Sift Science",
    "domains": [
      "siftscience.com"
    ]
  },
  {
    "name": "Affirm",
    "domains": [
      "affirm.com"
    ]
  },
  {
    "name": "Admixer for Publishers",
    "domains": [
      "admixer.net"
    ]
  },
  {
    "name": "LKQD",
    "domains": [
      "lkqd.net"
    ]
  },
  {
    "name": "Hotmart",
    "domains": [
      "launchermodule.hotmart.com"
    ]
  },
  {
    "name": "Secomapp",
    "domains": [
      "secomapp.com"
    ]
  },
  {
    "name": "Sortable",
    "domains": [
      "deployads.com"
    ]
  },
  {
    "name": "Bazaarvoice",
    "domains": [
      "bazaarvoice.com",
      "feedmagnet.com"
    ]
  },
  {
    "name": "Seznam",
    "domains": [
      "imedia.cz"
    ]
  },
  {
    "name": "Vidible",
    "domains": [
      "vidible.tv"
    ]
  },
  {
    "name": "Affiliate Window",
    "domains": [
      "dwin1.com"
    ]
  },
  {
    "name": "OptiMonk",
    "domains": [
      "optimonk.com"
    ]
  },
  {
    "name": "Refersion",
    "domains": [
      "refersion.com"
    ]
  },
  {
    "name": "Pagely",
    "domains": [
      "optnmstr.com"
    ]
  },
  {
    "name": "BounceX",
    "domains": [
      "bounceexchange.com",
      "events.bouncex.net"
    ]
  },
  {
    "name": "TrafficStars",
    "domains": [
      "trafficstars.com",
      "tsyndicate.com"
    ]
  },
  {
    "name": "SnapEngage",
    "domains": [
      "snapengage.com"
    ]
  },
  {
    "name": "Esri ArcGIS",
    "domains": [
      "arcgis.com",
      "arcgisonline.com"
    ]
  },
  {
    "name": "ForeSee",
    "domains": [
      "4seeresults.com",
      "answerscloud.com",
      "foresee.com",
      "foreseeresults.com"
    ]
  },
  {
    "name": "TagCommander",
    "domains": [
      "commander1.com",
      "tagcommander.com"
    ]
  },
  {
    "name": "Convert Insights",
    "domains": [
      "convertexperiments.com"
    ]
  },
  {
    "name": "iovation",
    "domains": [
      "iesnare.com"
    ]
  },
  {
    "name": "Clicktale",
    "domains": [
      "clicktale.net",
      "clicktalecdn.sslcs.cdngc.net"
    ]
  },
  {
    "name": "Comm100",
    "domains": [
      "comm100.com"
    ]
  },
  {
    "name": "Yieldmo",
    "domains": [
      "yieldmo.com"
    ]
  },
  {
    "name": "IPONWEB",
    "domains": [
      "company-target.com",
      "liadm.com",
      "p161.net",
      "pool.udsp.iponweb.net"
    ]
  },
  {
    "name": "Nend",
    "domains": [
      "nend.net"
    ]
  },
  {
    "name": "Perfect Market",
    "domains": [
      "perfectmarket.com"
    ]
  },
  {
    "name": "Fraudlogix",
    "domains": [
      "yabidos.com"
    ]
  },
  {
    "name": "Symantec",
    "domains": [
      "extended-validation-ssl.websecurity.symantec.com",
      "norton.com",
      "symantec.com",
      "symcb.com",
      "symcd.com"
    ]
  },
  {
    "name": "Bizible",
    "domains": [
      "bizible.com"
    ]
  },
  {
    "name": "Between Digital",
    "domains": [
      "betweendigital.com"
    ]
  },
  {
    "name": "Maxymiser",
    "domains": [
      "maxymiser.net"
    ]
  },
  {
    "name": "Branch Metrics",
    "domains": [
      "app.link",
      "branch.io"
    ]
  },
  {
    "name": "Tradelab",
    "domains": [
      "tradelab.fr"
    ]
  },
  {
    "name": "Digioh",
    "domains": [
      "lightboxcdn.com"
    ]
  },
  {
    "name": "Tail Target",
    "domains": [
      "tailtarget.com"
    ]
  },
  {
    "name": "GetResponse",
    "domains": [
      "getresponse.com"
    ]
  },
  {
    "name": "OwnerIQ",
    "domains": [
      "owneriq.net"
    ]
  },
  {
    "name": "Dynamic Yield",
    "domains": [
      "dynamicyield.com"
    ]
  },
  {
    "name": "Fort Awesome",
    "domains": [
      "fortawesome.com"
    ]
  },
  {
    "name": "Clerk.io ApS",
    "domains": [
      "clerk.io"
    ]
  },
  {
    "name": "Adyoulike",
    "domains": [
      "adyoulike.com",
      "adyoulike.net",
      "omnitagjs.com"
    ]
  },
  {
    "name": "iAdvize SAS",
    "domains": [
      "iadvize.com"
    ]
  },
  {
    "name": "Ecwid",
    "domains": [
      "d3fi9i0jj23cau.cloudfront.net",
      "d3j0zfs7paavns.cloudfront.net",
      "ecwid.com",
      "shopsettings.com"
    ]
  },
  {
    "name": "issuu",
    "domains": [
      "issuu.com",
      "isu.pub"
    ]
  },
  {
    "name": "Effective Measure",
    "domains": [
      "effectivemeasure.net"
    ]
  },
  {
    "name": "Geniee",
    "domains": [
      "cs.gssprt.jp",
      "genieessp.jp",
      "genieesspv.jp",
      "gssprt.jp",
      "href.asia"
    ]
  },
  {
    "name": "Bronto Software",
    "domains": [
      "bm23.com",
      "bronto.com",
      "brontops.com"
    ]
  },
  {
    "name": "eBay",
    "domains": [
      "ebay.com",
      "ebayimg.com",
      "fetchback.com"
    ]
  },
  {
    "name": "Elastic Ad",
    "domains": [
      "elasticad.net"
    ]
  },
  {
    "name": "PowerReviews",
    "domains": [
      "powerreviews.com"
    ]
  },
  {
    "name": "Okas Concepts",
    "domains": [
      "okasconcepts.com"
    ]
  },
  {
    "name": "Media Management Technologies",
    "domains": [
      "speedshiftmedia.com"
    ]
  },
  {
    "name": "Blindado",
    "domains": [
      "siteblindado.com"
    ]
  },
  {
    "name": "Nativo",
    "domains": [
      "postrelease.com"
    ]
  },
  {
    "name": "Autopilot",
    "domains": [
      "autopilothq.com"
    ]
  },
  {
    "name": "Picreel",
    "domains": [
      "pcrl.co",
      "picreel.com"
    ]
  },
  {
    "name": "Celtra",
    "domains": [
      "celtra.com"
    ]
  },
  {
    "name": "UserReport",
    "domains": [
      "userreport.com"
    ]
  },
  {
    "name": "Adverline Board",
    "domains": [
      "adnext.fr",
      "adverline.com"
    ]
  },
  {
    "name": "The ADEX",
    "domains": [
      "theadex.com"
    ]
  },
  {
    "name": "Mather Economics",
    "domains": [
      "matheranalytics.com"
    ]
  },
  {
    "name": "Decibel Insight",
    "domains": [
      "decibelinsight.net"
    ]
  },
  {
    "name": "Revcontent",
    "domains": [
      "revcontent.com"
    ]
  },
  {
    "name": "LightWidget",
    "domains": [
      "lightwidget.com"
    ]
  },
  {
    "name": "Wishpond Technologies",
    "domains": [
      "wishpond.com",
      "wishpond.net"
    ]
  },
  {
    "name": "Riskified",
    "domains": [
      "riskified.com"
    ]
  },
  {
    "name": "Kaltura Video Platform",
    "domains": [
      "cdnsecakmi.kaltura.com"
    ]
  },
  {
    "name": "TRUSTe",
    "domains": [
      "truste.com"
    ]
  },
  {
    "name": "Navegg",
    "domains": [
      "navdmp.com"
    ]
  },
  {
    "name": "LoopMe",
    "domains": [
      "loopme.biz",
      "loopme.com",
      "loopme.me",
      "vntsm.com"
    ]
  },
  {
    "name": "Weborama",
    "domains": [
      "weborama.com",
      "weborama.fr"
    ]
  },
  {
    "name": "Polar Mobile Group",
    "domains": [
      "mediavoice.com",
      "polarmobile.com"
    ]
  },
  {
    "name": "Interpublic Group",
    "domains": [
      "mbww.com"
    ]
  },
  {
    "name": "Sekindo",
    "domains": [
      "sekindo.com"
    ]
  },
  {
    "name": "WebEngage",
    "domains": [
      "d23nd6ymopvz52.cloudfront.net",
      "d3701cc9l7v9a6.cloudfront.net",
      "webengage.co",
      "webengage.com"
    ]
  },
  {
    "name": "Cross Pixel Media",
    "domains": [
      "crsspxl.com"
    ]
  },
  {
    "name": "plista",
    "domains": [
      "plista.com"
    ]
  },
  {
    "name": "Kampyle",
    "domains": [
      "kampyle.com"
    ]
  },
  {
    "name": "Tribal Fusion",
    "domains": [
      "tribalfusion.com"
    ]
  },
  {
    "name": "Gleam",
    "domains": [
      "gleam.io"
    ]
  },
  {
    "name": "Forensiq",
    "domains": [
      "fqtag.com"
    ]
  },
  {
    "name": "Audience 360",
    "domains": [
      "dpmsrv.com"
    ]
  },
  {
    "name": "Flowplayer",
    "domains": [
      "flowplayer.org"
    ]
  },
  {
    "name": "Sooqr Search",
    "domains": [
      "sooqr.com"
    ]
  },
  {
    "name": "MaxCDN Enterprise",
    "domains": [
      "netdna-cdn.com",
      "netdna-ssl.com"
    ]
  },
  {
    "name": "Shopgate",
    "domains": [
      "shopgate.com"
    ]
  },
  {
    "name": "BoldChat",
    "domains": [
      "boldchat.com"
    ]
  },
  {
    "name": "smartclip",
    "domains": [
      "smartclip.net"
    ]
  },
  {
    "name": "rewardStyle.com",
    "domains": [
      "rewardstyle.com"
    ]
  },
  {
    "name": "Chitika",
    "domains": [
      "chitika.net"
    ]
  },
  {
    "name": "WisePops",
    "domains": [
      "wisepops.com"
    ]
  },
  {
    "name": "Monetate",
    "domains": [
      "monetate.net"
    ]
  },
  {
    "name": "SpotXchange",
    "domains": [
      "spotx.tv",
      "spotxcdn.com",
      "spotxchange.com"
    ]
  },
  {
    "name": "Zanox",
    "domains": [
      "zanox.com",
      "zanox.ws"
    ]
  },
  {
    "name": "SublimeSkinz",
    "domains": [
      "ayads.co"
    ]
  },
  {
    "name": "Adocean",
    "domains": [
      "adocean.pl"
    ]
  },
  {
    "name": "Meetrics",
    "domains": [
      "meetrics.net",
      "mxcdn.net",
      "research.de.com"
    ]
  },
  {
    "name": "Booking.com",
    "domains": [
      "bstatic.com"
    ]
  },
  {
    "name": "Sparkflow",
    "domains": [
      "sparkflow.net"
    ]
  },
  {
    "name": "Lytics",
    "domains": [
      "lytics.io"
    ]
  },
  {
    "name": "ResponsiveVoice",
    "domains": [
      "responsivevoice.org"
    ]
  },
  {
    "name": "Ooyala",
    "domains": [
      "ooyala.com"
    ]
  },
  {
    "name": "Snacktools",
    "domains": [
      "bannersnack.com"
    ]
  },
  {
    "name": "linkpulse",
    "domains": [
      "lp4.io"
    ]
  },
  {
    "name": "Tencent",
    "domains": [
      "qq.com",
      "ywxi.net"
    ]
  },
  {
    "name": "Rocket Fuel",
    "domains": [
      "rfihub.com",
      "rfihub.net",
      "ru4.com"
    ]
  },
  {
    "name": "Adnium",
    "domains": [
      "adnium.com"
    ]
  },
  {
    "name": "Appier",
    "domains": [
      "appier.net"
    ]
  },
  {
    "name": "Stackla PTY",
    "domains": [
      "stackla.com"
    ]
  },
  {
    "name": "Hupso Website Analyzer",
    "domains": [
      "hupso.com"
    ]
  },
  {
    "name": "ReadSpeaker",
    "domains": [
      "sf1-eu.readspeaker.com"
    ]
  },
  {
    "name": "ShopiMind",
    "domains": [
      "shopimind.com"
    ]
  },
  {
    "name": "DialogTech",
    "domains": [
      "dialogtech.com"
    ]
  },
  {
    "name": "FoxyCart",
    "domains": [
      "foxycart.com"
    ]
  },
  {
    "name": "Neodata",
    "domains": [
      "neodatagroup.com"
    ]
  },
  {
    "name": "WebpageFX",
    "domains": [
      "leadmanagerfx.com"
    ]
  },
  {
    "name": "Smart Insight Tracking",
    "domains": [
      "scarabresearch.com"
    ]
  },
  {
    "name": "Feedbackify",
    "domains": [
      "feedbackify.com"
    ]
  },
  {
    "name": "Survicate",
    "domains": [
      "survicate.com"
    ]
  },
  {
    "name": "Aggregate Knowledge",
    "domains": [
      "agkn.com"
    ]
  },
  {
    "name": "Exponea",
    "domains": [
      "exponea.com"
    ]
  },
  {
    "name": "eXelate",
    "domains": [
      "exelator.com"
    ]
  },
  {
    "name": "Adition",
    "domains": [
      "dsp.adfarm1.adition.com"
    ]
  },
  {
    "name": "Highcharts",
    "domains": [
      "highcharts.com"
    ]
  },
  {
    "name": "FirstImpression",
    "domains": [
      "firstimpression.io"
    ]
  },
  {
    "name": "LiveHelpNow",
    "domains": [
      "livehelpnow.net"
    ]
  },
  {
    "name": "SearchSpring",
    "domains": [
      "searchspring.net"
    ]
  },
  {
    "name": "Pardot",
    "domains": [
      "pardot.com"
    ]
  },
  {
    "name": "JustPremium Ads",
    "domains": [
      "justpremium.com"
    ]
  },
  {
    "name": "DMD Marketing",
    "domains": [
      "medtargetsystem.com"
    ]
  },
  {
    "name": "The Hut Group",
    "domains": [
      "thcdn.com"
    ]
  },
  {
    "name": "Cloudinary",
    "domains": [
      "cloudinary.com"
    ]
  },
  {
    "name": "Technorati",
    "domains": [
      "technoratimedia.com"
    ]
  },
  {
    "name": "Github",
    "domains": [
      "github.com",
      "github.io",
      "raw.githubusercontent.com"
    ]
  },
  {
    "name": "Bootstrap Chinese network",
    "domains": [
      "bootcss.com"
    ]
  },
  {
    "name": "Typepad",
    "domains": [
      "typepad.com"
    ]
  },
  {
    "name": "Keywee",
    "domains": [
      "keywee.co"
    ]
  },
  {
    "name": "Skype",
    "domains": [
      "skype.com"
    ]
  },
  {
    "name": "Opta",
    "domains": [
      "opta.net"
    ]
  },
  {
    "name": "Livefyre",
    "domains": [
      "fyre.co",
      "livefyre.com"
    ]
  },
  {
    "name": "ReTargeter",
    "domains": [
      "retargeter.com"
    ]
  },
  {
    "name": "TruConversion",
    "domains": [
      "truconversion.com"
    ]
  },
  {
    "name": "fifty-five",
    "domains": [
      "55labs.com"
    ]
  },
  {
    "name": "Time",
    "domains": [
      "timeinc.net"
    ]
  },
  {
    "name": "Pixlee",
    "domains": [
      "pixlee.com"
    ]
  },
  {
    "name": "Reevoo",
    "domains": [
      "reevoo.com"
    ]
  },
  {
    "name": "Accordant Media",
    "domains": [
      "segment.a3cloud.net"
    ]
  },
  {
    "name": "Evergage",
    "domains": [
      "evergage.com"
    ]
  },
  {
    "name": "Exponential Interactive",
    "domains": [
      "exponential.com"
    ]
  },
  {
    "name": "Best Of Media S.A.",
    "domains": [
      "servebom.com"
    ]
  },
  {
    "name": "Steelhouse",
    "domains": [
      "steelhousemedia.com"
    ]
  },
  {
    "name": "Dailymotion",
    "domains": [
      "ad.pxlad.io",
      "dm.gg",
      "dmcdn.net",
      "sublimevideo.net",
      "www.dailymotion.com"
    ]
  },
  {
    "name": "TripleLift",
    "domains": [
      "3lift.com"
    ]
  },
  {
    "name": "DemandBase",
    "domains": [
      "demandbase.com"
    ]
  },
  {
    "name": "One by AOL",
    "domains": [
      "adtech.de",
      "adtechjp.com"
    ]
  },
  {
    "name": "Adkontekst",
    "domains": [
      "adkontekst.pl"
    ]
  },
  {
    "name": "Profitshare",
    "domains": [
      "profitshare.ro"
    ]
  },
  {
    "name": "Drip",
    "domains": [
      "getdrip.com"
    ]
  },
  {
    "name": "Ghostery Enterprise",
    "domains": [
      "betrad.com"
    ]
  },
  {
    "name": "Socialphotos",
    "domains": [
      "slpht.com"
    ]
  },
  {
    "name": "SoundCloud",
    "domains": [
      "widget.sndcdn.com",
      "soundcloud.com",
      "stratus.sc"
    ]
  },
  {
    "name": "Rackspace",
    "domains": [
      "rackcdn.com",
      "rackspacecloud.com",
      "raxcdn.com",
      "websitetestlink.com"
    ]
  },
  {
    "name": "NetAffiliation",
    "domains": [
      "metaffiliation.com"
    ]
  },
  {
    "name": "CPEx",
    "domains": [
      "cpex.cz"
    ]
  },
  {
    "name": "Sweet Tooth",
    "domains": [
      "sweettooth.io"
    ]
  },
  {
    "name": "Playbuzz",
    "domains": [
      "playbuzz.com"
    ]
  },
  {
    "name": "Civic",
    "domains": [
      "civiccomputing.com"
    ]
  },
  {
    "name": "Sajari Pty",
    "domains": [
      "sajari.com"
    ]
  },
  {
    "name": "PerimeterX Bot Defender",
    "domains": [
      "perimeterx.net",
      "pxi.pub"
    ]
  },
  {
    "name": "Marketplace Web Service",
    "domains": [
      "ssl-images-amazon.com"
    ]
  },
  {
    "name": "Ambassador",
    "domains": [
      "getambassador.com"
    ]
  },
  {
    "name": "Pictela (AOL)",
    "domains": [
      "pictela.net"
    ]
  },
  {
    "name": "AdSniper",
    "domains": [
      "adsniper.ru",
      "sniperlog.ru"
    ]
  },
  {
    "name": "Adscale",
    "domains": [
      "adscale.de"
    ]
  },
  {
    "name": "Signyfyd",
    "domains": [
      "signifyd.com"
    ]
  },
  {
    "name": "Connatix",
    "domains": [
      "connatix.com"
    ]
  },
  {
    "name": "Zarget",
    "domains": [
      "zarget.com"
    ]
  },
  {
    "name": "Woopra",
    "domains": [
      "woopra.com"
    ]
  },
  {
    "name": "Infinity Tracking",
    "domains": [
      "infinity-tracking.net"
    ]
  },
  {
    "name": "ResponseTap",
    "domains": [
      "adinsight.com",
      "responsetap.com"
    ]
  },
  {
    "name": "Sirv",
    "domains": [
      "sirv.com"
    ]
  },
  {
    "name": "Salesforce.com",
    "domains": [
      "force.com",
      "salesforce.com",
      "secure.force.com"
    ]
  },
  {
    "name": "Conversant Tag Manager",
    "domains": [
      "mplxtms.com"
    ]
  },
  {
    "name": "Petametrics",
    "domains": [
      "petametrics.com"
    ]
  },
  {
    "name": "BannerFlow",
    "domains": [
      "bannerflow.com"
    ]
  },
  {
    "name": "Pixalate",
    "domains": [
      "adrta.com"
    ]
  },
  {
    "name": "reEmbed",
    "domains": [
      "reembed.com"
    ]
  },
  {
    "name": "FreakOut",
    "domains": [
      "fout.jp"
    ]
  },
  {
    "name": "VoiceFive",
    "domains": [
      "voicefive.com"
    ]
  },
  {
    "name": "Impact Radius",
    "domains": [
      "7eer.net",
      "a.impactradius-go.com",
      "d3cxv97fi8q177.cloudfront.net",
      "impactradius-event.com",
      "microsoft-uk.evyy.net",
      "ojrq.net"
    ]
  },
  {
    "name": "AWeber",
    "domains": [
      "aweber.com"
    ]
  },
  {
    "name": "Simpli.fi",
    "domains": [
      "simpli.fi"
    ]
  },
  {
    "name": "Unruly Media",
    "domains": [
      "unrulymedia.com"
    ]
  },
  {
    "name": "Hola Networks",
    "domains": [
      "h-cdn.com"
    ]
  },
  {
    "name": "Customer.io",
    "domains": [
      "customer.io"
    ]
  },
  {
    "name": "Delta Projects AB",
    "domains": [
      "de17a.com"
    ]
  },
  {
    "name": "Advance Magazine Group",
    "domains": [
      "condenast.co.uk",
      "condenastdigital.com",
      "condenet.com"
    ]
  },
  {
    "name": "Key CDN",
    "domains": [
      "kxcdn.com"
    ]
  },
  {
    "name": "ThreatMetrix",
    "domains": [
      "online-metrix.net"
    ]
  },
  {
    "name": "Adtech (AOL)",
    "domains": [
      "adtechus.com"
    ]
  },
  {
    "name": "News",
    "domains": [
      "news-static.com",
      "news.com.au",
      "newsanalytics.com.au",
      "newsapi.com.au",
      "newscdn.com.au",
      "newsdata.com.au",
      "newsdiscover.com.au"
    ]
  },
  {
    "name": "AvantLink",
    "domains": [
      "avmws.com"
    ]
  },
  {
    "name": "CyberSource (Visa)",
    "domains": [
      "authorize.net"
    ]
  },
  {
    "name": "Vibrant Media",
    "domains": [
      "intellitxt.com",
      "picadmedia.com"
    ]
  },
  {
    "name": "FLXone",
    "domains": [
      "d2hlpp31teaww3.cloudfront.net",
      "flx1.com",
      "pangolin.blue"
    ]
  },
  {
    "name": "Adobe Marketing Cloud",
    "domains": [
      "adobetag.com"
    ]
  },
  {
    "name": "WebSpectator",
    "domains": [
      "webspectator.com"
    ]
  },
  {
    "name": "Intercept Interactive",
    "domains": [
      "undertone.com"
    ]
  },
  {
    "name": "Simplicity Marketing",
    "domains": [
      "flashtalking.com"
    ]
  },
  {
    "name": "AdRiver",
    "domains": [
      "adriver.ru"
    ]
  },
  {
    "name": "Mobify",
    "domains": [
      "mobify.com",
      "mobify.net"
    ]
  },
  {
    "name": "Apester",
    "domains": [
      "apester.com",
      "qmerce.com"
    ]
  },
  {
    "name": "Covert Pics",
    "domains": [
      "covet.pics"
    ]
  },
  {
    "name": "CleverDATA",
    "domains": [
      "1dmp.io"
    ]
  },
  {
    "name": "SecuredVisit",
    "domains": [
      "securedvisit.com"
    ]
  },
  {
    "name": "SlimCut Media Outstream",
    "domains": [
      "freeskreen.com"
    ]
  },
  {
    "name": "Exactag",
    "domains": [
      "exactag.com"
    ]
  },
  {
    "name": "Postcode Anywhere (Holdings)",
    "domains": [
      "postcodeanywhere.co.uk"
    ]
  },
  {
    "name": "Flickr",
    "domains": [
      "flickr.com",
      "staticflickr.com"
    ]
  },
  {
    "name": "bRealTime",
    "domains": [
      "brealtime.com"
    ]
  },
  {
    "name": "Research Online",
    "domains": [
      "www.researchonline.org.uk"
    ]
  },
  {
    "name": "Swoop",
    "domains": [
      "swoop.com"
    ]
  },
  {
    "name": "Widespace",
    "domains": [
      "sync.widespace.com"
    ]
  },
  {
    "name": "Eyeota",
    "domains": [
      "eyeota.net"
    ]
  },
  {
    "name": "Pagefair",
    "domains": [
      "pagefair.com",
      "pagefair.net"
    ]
  },
  {
    "name": "Wow Analytics",
    "domains": [
      "wowanalytics.co.uk"
    ]
  },
  {
    "name": "Rakuten LinkShare",
    "domains": [
      "linksynergy.com"
    ]
  },
  {
    "name": "Transifex",
    "domains": [
      "transifex.com"
    ]
  },
  {
    "name": "Ziff Davis Tech",
    "domains": [
      "adziff.com",
      "zdbb.net"
    ]
  },
  {
    "name": "Betgenius",
    "domains": [
      "connextra.com"
    ]
  },
  {
    "name": "AIR.TV",
    "domains": [
      "air.tv"
    ]
  },
  {
    "name": "MaxMind",
    "domains": [
      "maxmind.com"
    ]
  },
  {
    "name": "Expedia",
    "domains": [
      "travel-assets.com",
      "trvl-media.com",
      "www.trvl-px.com",
      "www.uciservice.com"
    ]
  },
  {
    "name": "ContextWeb",
    "domains": [
      "contextweb.com"
    ]
  },
  {
    "name": "Pusher",
    "domains": [
      "stats.pusher.com",
      "pusherapp.com"
    ]
  },
  {
    "name": "LeasdBoxer",
    "domains": [
      "leadboxer.com"
    ]
  },
  {
    "name": "SkyScanner",
    "domains": [
      "api.skyscanner.net"
    ]
  },
  {
    "name": "WalkMe",
    "domains": [
      "walkme.com"
    ]
  },
  {
    "name": "AdTrue",
    "domains": [
      "adtrue.com"
    ]
  },
  {
    "name": "Resonance Insights",
    "domains": [
      "res-x.com"
    ]
  },
  {
    "name": "Hull.js",
    "domains": [
      "hull.io",
      "hullapp.io"
    ]
  },
  {
    "name": "Video Media Groep",
    "domains": [
      "inpagevideo.nl",
      "vmg.host"
    ]
  },
  {
    "name": "MonetizeMore",
    "domains": [
      "m2.ai"
    ]
  },
  {
    "name": "Fanplayr",
    "domains": [
      "d38nbbai6u794i.cloudfront.net",
      "fanplayr.com"
    ]
  },
  {
    "name": "Onet",
    "domains": [
      "onet.pl"
    ]
  },
  {
    "name": "Boomtrain",
    "domains": [
      "boomtrain.com",
      "boomtrain.net"
    ]
  },
  {
    "name": "Proper Media",
    "domains": [
      "proper.io"
    ]
  },
  {
    "name": "StumbleUpon",
    "domains": [
      "stumble-upon.com",
      "stumbleupon.com"
    ]
  },
  {
    "name": "Zmags",
    "domains": [
      "zmags.com"
    ]
  },
  {
    "name": "Vee24",
    "domains": [
      "vee24.com"
    ]
  },
  {
    "name": "Sailthru",
    "domains": [
      "sail-horizon.com",
      "sail-personalize.com",
      "sail-track.com"
    ]
  },
  {
    "name": "Klevu Search",
    "domains": [
      "klevu.com"
    ]
  },
  {
    "name": "Cedato",
    "domains": [
      "algovid.com",
      "vdoserv.com"
    ]
  },
  {
    "name": "Trip Advisor",
    "domains": [
      "tacdn.com",
      "tripadvisor.co.uk",
      "tripadvisor.com",
      "viator.com",
      "www.jscache.com",
      "www.tamgrt.com"
    ]
  },
  {
    "name": "Captify Media",
    "domains": [
      "cpx.to"
    ]
  },
  {
    "name": "Yottaa",
    "domains": [
      "yottaa.com",
      "yottaa.net"
    ]
  },
  {
    "name": "PERFORM",
    "domains": [
      "performgroup.com"
    ]
  },
  {
    "name": "Vindico",
    "domains": [
      "vindicosuite.com"
    ]
  },
  {
    "name": "Snack Media",
    "domains": [
      "snack-media.com"
    ]
  },
  {
    "name": "FuelX",
    "domains": [
      "fuelx.com"
    ]
  },
  {
    "name": "OnScroll",
    "domains": [
      "onscroll.com"
    ]
  },
  {
    "name": "Alliance for Audited Media",
    "domains": [
      "aamsitecertifier.com"
    ]
  },
  {
    "name": "ShopRunner",
    "domains": [
      "s-9.us",
      "shoprunner.com"
    ]
  },
  {
    "name": "Janrain",
    "domains": [
      "d3hmp0045zy3cs.cloudfront.net",
      "janrain.com",
      "janrainbackplane.com",
      "rpxnow.com"
    ]
  },
  {
    "name": "AliveChat",
    "domains": [
      "websitealive.com",
      "websitealive7.com"
    ]
  },
  {
    "name": "SpringServer",
    "domains": [
      "springserve.com"
    ]
  },
  {
    "name": "Global-e",
    "domains": [
      "global-e.com"
    ]
  },
  {
    "name": "cloudIQ",
    "domains": [
      "cloud-iq.com"
    ]
  },
  {
    "name": "ZEDO",
    "domains": [
      "zedo.com"
    ]
  },
  {
    "name": "Forter",
    "domains": [
      "forter.com"
    ]
  },
  {
    "name": "Silverpop",
    "domains": [
      "mkt51.net",
      "mkt61.net",
      "mkt912.com",
      "mkt922.com",
      "mkt932.com",
      "mkt941.com",
      "pages01.net",
      "pages02.net",
      "pages03.net",
      "pages04.net",
      "pages05.net"
    ]
  },
  {
    "name": "Polyfill service",
    "domains": [
      "polyfill.io"
    ]
  },
  {
    "name": "epoq internet services",
    "domains": [
      "epoq.de"
    ]
  },
  {
    "name": "CDN.net",
    "domains": [
      "uk.cdn-net.com"
    ]
  },
  {
    "name": "Kameleoon",
    "domains": [
      "kameleoon.com"
    ]
  },
  {
    "name": "Council ad Network",
    "domains": [
      "counciladvertising.net"
    ]
  },
  {
    "name": "Oracle Recommendations On Demand",
    "domains": [
      "atgsvcs.com"
    ]
  },
  {
    "name": "Viacom",
    "domains": [
      "mtvnservices.com"
    ]
  },
  {
    "name": "Optimove",
    "domains": [
      "optimove.net"
    ]
  },
  {
    "name": "Cookie Reports",
    "domains": [
      "cookiereports.com"
    ]
  },
  {
    "name": "Storygize",
    "domains": [
      "www.storygize.net"
    ]
  },
  {
    "name": "Revolver Maps",
    "domains": [
      "revolvermaps.com"
    ]
  },
  {
    "name": "Reactful",
    "domains": [
      "reactful.com"
    ]
  },
  {
    "name": "NaviStone",
    "domains": [
      "murdoog.com"
    ]
  },
  {
    "name": "Vertical Mass",
    "domains": [
      "vmweb.net"
    ]
  },
  {
    "name": "Conversant",
    "domains": [
      "dotomi.com",
      "dtmpub.com",
      "emjcd.com",
      "fastclick.net",
      "mediaplex.com",
      "www.tqlkg.com"
    ]
  },
  {
    "name": "BlueCava",
    "domains": [
      "bluecava.com"
    ]
  },
  {
    "name": "VidPulse",
    "domains": [
      "vidpulse.com"
    ]
  },
  {
    "name": "LoginRadius",
    "domains": [
      "loginradius.com"
    ]
  },
  {
    "name": "Byside",
    "domains": [
      "byce2.byside.com",
      "wce2.byside.com"
    ]
  },
  {
    "name": "MailPlus",
    "domains": [
      "mailplus.nl"
    ]
  },
  {
    "name": "Touch Commerce",
    "domains": [
      "inq.com",
      "touchcommerce.com"
    ]
  },
  {
    "name": "Netlify",
    "domains": [
      "netlify.com",
      "cloud.netlifyusercontent.com"
    ]
  },
  {
    "name": "Kargo",
    "domains": [
      "kargo.com"
    ]
  },
  {
    "name": "SurveyMonkey",
    "domains": [
      "surveymonkey.com"
    ]
  },
  {
    "name": "User Replay",
    "domains": [
      "userreplay.net"
    ]
  },
  {
    "name": "Catchpoint",
    "domains": [
      "3gl.net"
    ]
  },
  {
    "name": "Conversio",
    "domains": [
      "conversio.com"
    ]
  },
  {
    "name": "AdvertServe",
    "domains": [
      "advertserve.com"
    ]
  },
  {
    "name": "PrintFriendly",
    "domains": [
      "printfriendly.com"
    ]
  },
  {
    "name": "Mopinion",
    "domains": [
      "mopinion.com"
    ]
  },
  {
    "name": "Barilliance",
    "domains": [
      "barilliance.net",
      "dn3y71tq7jf07.cloudfront.net"
    ]
  },
  {
    "name": "Flockler",
    "domains": [
      "flockler.com"
    ]
  },
  {
    "name": "Attribution",
    "domains": [
      "attributionapp.com"
    ]
  },
  {
    "name": "Vergic AB",
    "domains": [
      "psplugin.com"
    ]
  },
  {
    "name": "CANDDi",
    "domains": [
      "canddi.com"
    ]
  },
  {
    "name": "PebblePost",
    "domains": [
      "pbbl.co"
    ]
  },
  {
    "name": "Braintree Payments",
    "domains": [
      "braintreegateway.com"
    ]
  },
  {
    "name": "InSkin Media",
    "domains": [
      "inskinad.com",
      "inskinmedia.com"
    ]
  },
  {
    "name": "StreamRail",
    "domains": [
      "streamrail.com",
      "streamrail.net"
    ]
  },
  {
    "name": "Site24x7 Real User Monitoring",
    "domains": [
      "site24x7rum.com"
    ]
  },
  {
    "name": "YoYo",
    "domains": [
      "goadservices.com"
    ]
  },
  {
    "name": "Adunity",
    "domains": [
      "adunity.com"
    ]
  },
  {
    "name": "PlayAd Media Group",
    "domains": [
      "youplay.se"
    ]
  },
  {
    "name": "BuySellAds",
    "domains": [
      "buysellads.com"
    ]
  },
  {
    "name": "Moovweb",
    "domains": [
      "moovweb.net"
    ]
  },
  {
    "name": "Bookatable",
    "domains": [
      "bookatable.com",
      "livebookings.com"
    ]
  },
  {
    "name": "Raygun",
    "domains": [
      "raygun.io"
    ]
  },
  {
    "name": "Sociomantic Labs",
    "domains": [
      "sociomantic.com"
    ]
  },
  {
    "name": "Borderfree",
    "domains": [
      "borderfree.com",
      "fiftyone.com"
    ]
  },
  {
    "name": "Dynamic Converter",
    "domains": [
      "dynamicconverter.com"
    ]
  },
  {
    "name": "C3 Metrics",
    "domains": [
      "c3tag.com"
    ]
  },
  {
    "name": "eGain",
    "domains": [
      "analytics-egain.com",
      "egain.com"
    ]
  },
  {
    "name": "TechTarget",
    "domains": [
      "techtarget.com",
      "ttgtmedia.com"
    ]
  },
  {
    "name": "Adobe Scene7",
    "domains": [
      "everestads.net",
      "everestjs.net",
      "scene7.com",
      "wwwimages.adobe.com"
    ]
  },
  {
    "name": "HotelsCombined",
    "domains": [
      "datahc.com"
    ]
  },
  {
    "name": "StackAdapt",
    "domains": [
      "stackadapt.com"
    ]
  },
  {
    "name": "The Publisher Desk",
    "domains": [
      "206ads.com",
      "publisherdesk.com"
    ]
  },
  {
    "name": "Ekm Systems",
    "domains": [
      "ekmpinpoint.co.uk",
      "ekmsecure.com",
      "globalstats.ekmsecure.com"
    ]
  },
  {
    "name": "DistroScale",
    "domains": [
      "jsrdn.com"
    ]
  },
  {
    "name": "Knight Lab",
    "domains": [
      "knightlab.com"
    ]
  },
  {
    "name": "Vergic Engage Platform",
    "domains": [
      "vergic.com"
    ]
  },
  {
    "name": "AdCurve",
    "domains": [
      "shop2market.com"
    ]
  },
  {
    "name": "StackExchange",
    "domains": [
      "sstatic.net"
    ]
  },
  {
    "name": "MathJax",
    "domains": [
      "mathjax.org"
    ]
  },
  {
    "name": "RebelMouse",
    "domains": [
      "rbl.ms",
      "www.rebelmouse.com"
    ]
  },
  {
    "name": "ShopStorm",
    "domains": [
      "shopstorm.com"
    ]
  },
  {
    "name": "Ad6Media",
    "domains": [
      "ad6media.fr"
    ]
  },
  {
    "name": "OCSP",
    "domains": [
      "ocsp.godaddy.com",
      "seal.godaddy.com"
    ]
  },
  {
    "name": "Bluecore",
    "domains": [
      "www.bluecore.com"
    ]
  },
  {
    "name": "Cachefly",
    "domains": [
      "cachefly.net"
    ]
  },
  {
    "name": "Nanorep",
    "domains": [
      "nanorep.com"
    ]
  },
  {
    "name": "AdSpruce",
    "domains": [
      "adspruce.com"
    ]
  },
  {
    "name": "content.ad",
    "domains": [
      "content.ad"
    ]
  },
  {
    "name": "Improve Digital",
    "domains": [
      "360yield.com"
    ]
  },
  {
    "name": "Fastest Forward",
    "domains": [
      "gaug.es"
    ]
  },
  {
    "name": "RichRelevance",
    "domains": [
      "richrelevance.com"
    ]
  },
  {
    "name": "ARM",
    "domains": [
      "tag4arm.com"
    ]
  },
  {
    "name": "Webtrends",
    "domains": [
      "d1q62gfb8siqnm.cloudfront.net",
      "webtrends.com",
      "webtrendslive.com"
    ]
  },
  {
    "name": "Click4Assistance",
    "domains": [
      "click4assistance.co.uk"
    ]
  },
  {
    "name": "Realytics",
    "domains": [
      "dcniko1cv0rz.cloudfront.net",
      "realytics.net"
    ]
  },
  {
    "name": "Xaxis",
    "domains": [
      "t.mookie1.com",
      "247realmedia.com",
      "gmads.net",
      "odr.mookie1.com"
    ]
  },
  {
    "name": "UPS i-parcel",
    "domains": [
      "i-parcel.com"
    ]
  },
  {
    "name": "Qualtrics",
    "domains": [
      "qualtrics.com"
    ]
  },
  {
    "name": "Adobe Test & Target",
    "domains": [
      "tt.omtrdc.net"
    ]
  }
]
)--";

}

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_THIRD_PARTIES_H_