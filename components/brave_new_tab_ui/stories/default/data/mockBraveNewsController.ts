// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as BraveNews from '../../../../brave_news/browser/resources/shared/api'
import { BraveNewsControllerRemote } from '../../../../brave_news/browser/resources/shared/api'

// Generate feed page from real data in devtools:
// let pids = [
//   "5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5",
//   "4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5",
//   "a5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5",
//   "b4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5",
//   "c5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5",
//   "d4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5",
//   "eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5",
//   "fc5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5",
//   "gd4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5"
// ]
//
// copy(newState.feed.pages[0].items.map( i => ({ ...i, items: i.items.map(ii => {
//   let data = ii.article?.data || ii.promotedArticle?.data || ii.deal?.data
//   if (ii.article) { data = ii.article.data }
//   if (ii.promotedArticle) { data = ii.promotedArticle.data }
//   data.publishTime.internalValue = data.publishTime.internalValue.toString()
//   data.publisherId = pids[Math.floor(Math.random() * (pids.length - 1))]
//   if (!ii.article) ii.article = "undefined"
//   if (!ii.promotedArticle) ii.promotedArticle = "undefined"
//   if (!ii.deal) ii.deal = "undefined"
//   return ii
// })})))
export const publishers: BraveNews.Publishers = {
  'direct:https://example.com/feed': {
    publisherId: 'direct:https://example.com/feed1',
    publisherName: 'My Custom Feed 1',
    categoryName: 'User feeds',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['User feeds'], rank: 0 }],
    type: BraveNews.PublisherType.DIRECT_SOURCE,
    isEnabled: true,
    userEnabledStatus: BraveNews.UserEnabled.ENABLED
  },
  'direct:https://example2.com/feed': {
    publisherId: 'direct:https://example.com/feed2',
    publisherName: 'My Custom Feed 2',
    categoryName: 'User feeds',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['User feeds'], rank: 0 }],
    type: BraveNews.PublisherType.DIRECT_SOURCE,
    isEnabled: true,
    userEnabledStatus: BraveNews.UserEnabled.ENABLED
  },
  '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5': {
    publisherId: '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
    publisherName: 'Test Publisher 1',
    categoryName: 'Tech',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['Tech'], rank: 0 }],
    type: BraveNews.PublisherType.COMBINED_SOURCE,
    isEnabled: false,
    userEnabledStatus: BraveNews.UserEnabled.ENABLED
  },
  '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5': {
    publisherId: '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
    publisherName: 'Test Publisher 2',
    categoryName: 'Top News',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['Top News'], rank: 0 }],
    type: BraveNews.PublisherType.COMBINED_SOURCE,
    isEnabled: false,
    userEnabledStatus: BraveNews.UserEnabled.NOT_MODIFIED
  },
  'a5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5': {
    publisherId: '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
    publisherName: 'Test Publisher 3',
    categoryName: 'Tech 2',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['Tech 2'], rank: 0 }],
    type: BraveNews.PublisherType.COMBINED_SOURCE,
    isEnabled: false,
    userEnabledStatus: BraveNews.UserEnabled.ENABLED
  },
  'b4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5': {
    publisherId: '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
    publisherName: 'Test Publisher 4',
    categoryName: 'Top News 1',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['Top News 1'], rank: 0 }],
    type: BraveNews.PublisherType.COMBINED_SOURCE,
    isEnabled: false,
    userEnabledStatus: BraveNews.UserEnabled.NOT_MODIFIED
  },
  'c5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5': {
    publisherId: '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
    publisherName: 'Test Publisher 5 has A very very very very very very very very very very very very very very very very very very very very long publisher name',
    categoryName: 'Tech 2',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['Tech 2'], rank: 0 }],
    type: BraveNews.PublisherType.COMBINED_SOURCE,
    isEnabled: false,
    userEnabledStatus: BraveNews.UserEnabled.ENABLED
  },
  'd4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5': {
    publisherId: '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
    publisherName: 'Test Publisher 6',
    categoryName: 'Top News 2',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['Top News 2'], rank: 0 }],
    type: BraveNews.PublisherType.COMBINED_SOURCE,
    isEnabled: false,
    userEnabledStatus: BraveNews.UserEnabled.NOT_MODIFIED
  },
  'eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5': {
    publisherId: '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
    publisherName: 'Test Publisher 7',
    categoryName: 'Top News 3',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['Top News 3'], rank: 0 }],
    type: BraveNews.PublisherType.COMBINED_SOURCE,
    isEnabled: false,
    userEnabledStatus: BraveNews.UserEnabled.NOT_MODIFIED
  },
  'fc5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5': {
    publisherId: '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
    publisherName: 'Test Publisher 8',
    categoryName: 'Tech 3',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['Tech 3'], rank: 0 }],
    type: BraveNews.PublisherType.COMBINED_SOURCE,
    isEnabled: false,
    userEnabledStatus: BraveNews.UserEnabled.ENABLED
  },
  'gd4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5': {
    publisherId: '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
    publisherName: 'Test Publisher 9',
    categoryName: 'Top News 4',
    feedSource: { url: 'http://www.example.com/feed' },
    backgroundColor: undefined,
    coverUrl: undefined,
    faviconUrl: undefined,
    siteUrl: { url: 'https://www.example.com' },
    locales: [{ locale: 'en_US', channels: ['Top News 4'], rank: 0 }],
    type: BraveNews.PublisherType.COMBINED_SOURCE,
    isEnabled: false,
    userEnabledStatus: BraveNews.UserEnabled.NOT_MODIFIED
  }
}

export const feed: BraveNews.Feed = {
  hash: '123abc',
  featuredItem: {
    promotedArticle: undefined,
    deal: undefined,
    article: {
      isDiscover: true,
      data: {
        categoryName: 'Top News',
        channels: ['Top News'],
        description: 'Here\'s everything you need to know about the Haunted Hallows event, including how to unlock the Batmobile.',
        image: { imageUrl: { url: 'https://placekitten.com/1360/912' }, paddedImageUrl: undefined },
        publishTime: { internalValue: BigInt('13278618001000000') },
        publisherId: 'd75d65f0f747650ef1ea11adb0029f9d577c629a080b5f60ec80d125b2bf205b',
        publisherName: 'Newsweek',
        relativeTimeDescription: '1 hour ago',
        urlHash: '',
        popScore: 0,
        score: 14.200669212327124,
        title: '\'Rocket League\' Haunted Hallows 2021: Details Revealed and How to Unlock Batmobile Cars',
        url: { url: 'https://www.newsweek.com/rocket-league-haunted-hallows-halloween-2021-batmobile-price-date-1638020' }
      }
    }
  },
  pages: [
    {
      items: [
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Sports',
                  'channels': ['Sports'],
                  'publishTime': {
                    internalValue: BigInt('13278621116000000')
                  },
                  'title': 'The agony of the feet: Why turf toe is such a dreaded injury in the NFL',
                  'description': 'A misunderstood and often dismissed condition, a big toe hyperextension can cause crippling pain with every step, creating mental anguish and fatigue that take a huge toll on an athlete. After decades of occurrences, it\'s finally being taken seriously.',
                  'url': {
                    'url': 'https://www.espn.com/nfl/story/_/id/32379942/why-turf-toe-such-dreaded-injury-nfl'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/944ee78cb12ebda3d46c51a4fb91db1a961d3dee13b8eb1034798ae5ca2150dc.jpg.pad'
                    }
                  },
                  'publisherId': '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'ESPN',
                  popScore: 0,
                  'score': 10.00343277763486,
                  'relativeTimeDescription': '31 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Sports',
                  'channels': ['Sports'],
                  'publishTime': {
                    internalValue: BigInt('13278620426000000')
                  },
                  'title': 'Spain didn\'t win Nations League but Oyarzabal  is a star',
                  'description': 'Spain\'s latest super-group of teenagers has caught the imagination this week, but 24-year-old Real Sociedad winger Mikel Oyarzabal might be the one to lead them.',
                  'url': {
                    'url': 'https://www.espn.com/soccer/spain-esp/story/4495432/spain-didnt-win-nations-league-but-they-have-a-gem-in-mikel-oyarzabal'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/9bd848550d4f968a1f93c46a1c4f142cb293ad8220092d8d3017b32d38b08c67.jpg.pad'
                    }
                  },
                  'publisherId': 'fc5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'ESPN - Football',
                  popScore: 0,
                  'score': 10.649119373004728,
                  'relativeTimeDescription': '43 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Technology',
                  'channels': ['Technology'],
                  'publishTime': {
                    internalValue: BigInt('13278621733000000')
                  },
                  'title': 'Twitter is launching a Spaces accelerator program to pay live audio creators',
                  'description': '\n\nIllustration by Alex Castro / The Verge\n\nTwitter announced on Tuesday that it plans to support Twitter Spaces creators through a new three-month accelerator program called the Twitter Spaces Spark Program. Twitter’s plans follow a similar creator three-month program that Clubhouse launched in March 2021.\nThe Spark Program is designed to “discover and reward” around 150 Spaces creators with technical, financial, and marketing support, Twitter says. For anyone who applies and gets in, that inclu',
                  'url': {
                    'url': 'https://www.theverge.com/2021/10/13/22724450/twitter-spaces-accelerator-spark-clubhouse-creators'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/023e9e849e2a2af24c5271faa0ba5b25b15f4c90481a3583e948829ef40e881a.jpg.pad'
                    }
                  },
                  'publisherId': '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'The Verge',
                  popScore: 0,
                  'score': 14.170018256553691,
                  'relativeTimeDescription': '21 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Science',
                  'channels': ['Science'],
                  'publishTime': {
                    internalValue: BigInt('13278621699000000')
                  },
                  'title': 'Widespread masking nudges people to follow the crowd',
                  'description': 'When wearing a mask to defend against the spread of COVID-19 becomes a trend, more people mask up themselves, a new study shows.',
                  'url': {
                    'url': 'https://www.futurity.org/covid-19-mask-viruses-2641802-2/?utm_source=rss&utm_medium=rss&utm_campaign=covid-19-mask-viruses-2641802-2'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/b735cb231cc73ce065177509ecda0ef35203e55bee5bfa798401a7bc67893182.jpg.pad'
                    }
                  },
                  'publisherId': '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Futurity',
                  popScore: 0,
                  'score': 14.226233174417557,
                  'relativeTimeDescription': '22 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 6,
          'items': [
            {
              article: undefined,
              deal: undefined,
              promotedArticle: {
                'data': {
                  'categoryName': 'Brave Partners',
                  'channels': ['Brave Partners'],
                  'publishTime': {
                    internalValue: BigInt('13278621628000000')
                  },
                  'title': 'Audiovox (VOXX) Q2 2022 Earnings Call Transcript',
                  'description': 'VOXX earnings call for the period ending September 30, 2021.',
                  'url': {
                    'url': 'https://www.fool.com/earnings/call-transcripts/2021/10/13/audiovox-voxx-q2-2022-earnings-call-transcript/?source=thebrave&utm_source=foo&utm_medium=feed&utm_campaign=article'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/5b3d8da219eee17bce800689085994a6a851545aa99b35c374874f42a93c672b.jpg.pad'
                    }
                  },
                  'publisherId': 'a5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'The Motley Fool',
                  popScore: 0,
                  'score': 14.338672770645763,
                  'relativeTimeDescription': '23 minutes ago'
                },
                'creativeInstanceId': 'd2d506aa-5531-4069-8f85-7d9052f1b640'
              }
            }
          ]
        },
        {
          'cardType': 2,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Top News',
                  'channels': ['Top News'],
                  'publishTime': {
                    internalValue: BigInt('13278621613000000')
                  },
                  'title': 'Brexit: Most NI checks on British goods to be scrapped',
                  'description': 'The proposals are a \"genuine response\" to address Brexit trade issues, says the European Commission.',
                  'url': {
                    'url': 'https://www.bbc.co.uk/news/uk-northern-ireland-58871221?at_medium=RSS&at_campaign=KARANGA'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/1d465c5238d91f25be576c554f691bd651be6d1346382888a9636c52258f2d67.jpg.pad'
                    }
                  },
                  'publisherId': '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'BBC',
                  popScore: 0,
                  'score': 14.361647694838718,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Top News',
                  'channels': ['Top News'],
                  'publishTime': {
                    internalValue: BigInt('13278621600000000')
                  },
                  'title': 'What needs to be done to fix the tax system?',
                  'description': 'Death duties, hiking the GST and more taxes on housing are on the wish lists of the nation’s top economists.',
                  'url': {
                    'url': 'https://www.smh.com.au/politics/federal/what-needs-to-be-done-to-fix-the-tax-system-20211004-p58x26.html?ref=rss&utm_medium=rss&utm_source=rss_feed'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/40a3ae95c9d10d6988236d4e17e0533f2528259d67ae3ed4b44f8243b65f764e.jpg.pad'
                    }
                  },
                  'publisherId': 'd4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Sydney Morning Herald',
                  popScore: 0,
                  'score': 14.381381289249747,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Top News',
                  'channels': ['Top News'],
                  'publishTime': {
                    internalValue: BigInt('13278621533000000')
                  },
                  'title': 'Woman Parades Through Airport Completely Naked, Makes Small Talk With Travelers',
                  'description': '\'The woman asked travelers how they were doing and where they are from\'',
                  'url': {
                    'url': 'https://dailycaller.com/2021/10/13/denver-airport-naked-woman/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/e9f94baedccb0fe6aa4679dba6762ded76b6d4a412149c55b29d5090fff182c5.jpg.pad'
                    }
                  },
                  'publisherId': 'eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Daily Caller',
                  popScore: 0,
                  'score': 14.479957341206271,
                  'relativeTimeDescription': '24 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Business',
                  'channels': ['Business'],
                  'publishTime': {
                    internalValue: BigInt('13278621684000000')
                  },
                  'title': 'William Shatner emotionally describes spaceflight to Jeff Bezos: \'The most profound experience\'',
                  'description': 'William Shatner, after returning to Earth, recounted his experience in an emotional talk with Blue Origin founder Jeff Bezos.',
                  'url': {
                    'url': 'https://www.cnbc.com/2021/10/13/william-shatner-speech-to-jeff-bezos-after-blue-origin-launch.html'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/c02a9c39546df2890a411e6afdac5f8a10ceded30947c4688fcfde43010c1d84.jpg.pad'
                    }
                  },
                  'publisherId': 'eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'CNBC',
                  popScore: 0,
                  'score': 14.250519019150758,
                  'relativeTimeDescription': '22 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Cars',
                  'channels': ['Cars'],
                  'publishTime': {
                    internalValue: BigInt('13278621683000000')
                  },
                  'title': 'Lucid Air’s DreamDrive ADAS Suite Has LiDAR, 14 Cameras, And 32 Sensors For Future Proofing',
                  'description': 'Lucid says that their overengineered tech suite will eventually be updated to include a \"hands-off, eyes-off\" driver assistance system.',
                  'url': {
                    'url': 'https://www.carscoops.com/2021/10/lucid-airs-dreamdrive-adas-suite-has-lidar-14-cameras-and-32-sensors-for-future-proofing/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/31b73f47c42c5323b6db05ff9907eee4b5217ca9a097e4aa810272609aebb06b.jpg.pad'
                    }
                  },
                  'publisherId': 'eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Carscoops',
                  popScore: 0,
                  'score': 14.252130607208834,
                  'relativeTimeDescription': '22 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Home',
                  'channels': ['Home'],
                  'publishTime': {
                    internalValue: BigInt('13278621634000000')
                  },
                  'title': 'DMTV Milkshake: Cultivating Elegance at Home With Melissa Lee',
                  'description': 'Melissa Lee, a self-titled aesthete, shares the invisible element that can change a space drastically when it comes to interior design.',
                  'url': {
                    'url': 'https://design-milk.com/dmtv-milkshake-cultivating-elegance-at-home-with-melissa-lee/?utm_source=feedburner&utm_campaign=Feed%3A+design-milk+%28Design+Milk%29'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/d00bc1449477f06cf7640231fd898be6c4e6fbd9f6eda99f1b845bb739419376.jpg.pad'
                    }
                  },
                  'publisherId': 'eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Design Milk',
                  popScore: 0,
                  'score': 14.329404416919667,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Entertainment',
                  'channels': ['Entertainment'],
                  'publishTime': {
                    internalValue: BigInt('13278621632000000')
                  },
                  'title': '‘You’ Renewed For Season 4 By Netflix',
                  'description': 'Ahead of the Season 3 premiere on Friday, Netflix has handed an early fourth season renewal to its hit drama series You. Casting news for the new season will be announced at a later date. Starring Penn Badgley and Victoria Pedretti, You is developed by Sera Gamble and Greg Berlanti, and Gamble also serves as […]',
                  'url': {
                    'url': 'https://deadline.com/2021/10/you-renewed-season-4-netflix-1234855244/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/3b6d9b50f065b320acd02393d96199d9f45028b14399c2b971a85590da89ae33.jpg.pad'
                    }
                  },
                  'publisherId': 'fc5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Deadline',
                  popScore: 0,
                  'score': 14.332498680036917,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Entertainment',
                  'channels': ['Entertainment'],
                  'publishTime': {
                    internalValue: BigInt('13278621625000000')
                  },
                  'title': 'AFI Fest Full Lineup: 2021 Festival Adds Pedro Almodovar’s ‘Parallel Mothers’ and More',
                  'description': 'As previously announced, the awards-facing festival will open with the premiere of Lin-Manuel Miranda\'s \"Tick Tick Boom.\"',
                  'url': {
                    'url': 'https://www.indiewire.com/2021/10/afi-fest-full-lineup-2021-festival-1234671528/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/d42fe1b86e568d6dca5dc3623a2bb982f3fb337ff79ae338a3544b3cc4cab1b9.jpg.pad'
                    }
                  },
                  'publisherId': 'c5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'IndieWire',
                  popScore: 0,
                  'score': 14.343289731415563,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Entertainment',
                  'channels': ['Entertainment'],
                  'publishTime': {
                    internalValue: BigInt('13278621617000000')
                  },
                  'title': 'YOU Renewed for Season 4 at Netflix — Watch Announcement Video',
                  'description': 'Netflix’s Joe Goldberg obsession continues with a Season 4 renewal of YOU, TVLine has learned. This news comes just two days before the Penn Badgley thriller is set to premiere its third season on Friday, Oct. 15. Based on Caroline Kepnes’ series of novels, YOU is developed by executive producers Greg Berlanti and Sera Gamble, […]',
                  'url': {
                    'url': 'https://tvline.com/2021/10/13/you-renewed-season-4-teaser-video-netflix/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/cbf6594866c44ef06f3cd74ba057559fac2e8dc659e33fa406d11a3583f4b2ed.jpg.pad'
                    }
                  },
                  'publisherId': 'a5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'TVLine',
                  popScore: 0,
                  'score': 14.355544195642704,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 5,
          'items': []
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Science',
                  'channels': ['Science'],
                  'publishTime': {
                    internalValue: BigInt('13278621608000000')
                  },
                  'title': 'First evidence of microtubules\' mechanosensitive behavior',
                  'description': 'Inside cells, microtubules not only serve as a component of the cytoskeleton (cell skeleton) but also play a role in intracellular transport. In intracellular transport, microtubules act as rails for motor proteins such as kinesin and dynein. Microtubules, the most rigid cytoskeletal component, are constantly subjected to various mechanical stresses such as compression, tension, and bending during cellular activities. It has been hypothesized that microtubules also function as mechanosensors tha',
                  'url': {
                    'url': 'https://phys.org/news/2021-10-evidence-microtubules-mechanosensitive-behavior.html'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/66475a510a759c80d3ed6bfac562fa8c4dd802b510326c8d0cae28eaae250444.jpg.pad'
                    }
                  },
                  'publisherId': 'c5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Phys.org',
                  popScore: 0,
                  'score': 14.369246557105962,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Sports',
                  'channels': ['Sports'],
                  'publishTime': {
                    internalValue: BigInt('13278621606000000')
                  },
                  'title': 'Fantasy Football Week 6 Rankings: Updated Overview for All Positions',
                  'description': 'We\'ve reached a critical point in the  NFL  season for fantasy managers. Bye weeks are here, and the Atlanta Falcons, New Orleans Saints, New York Jets and San Francisco 49ers have the first off rotation of the year...',
                  'url': {
                    'url': 'https://bleacherreport.com/articles/2949345-fantasy-football-week-6-rankings-updated-overview-for-all-positions'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/191782169a17e5df634109ecd2bb76c24cb48faa48f876edb3a5b2694fa497ba.jpg.pad'
                    }
                  },
                  'publisherId': 'd4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Bleacher Report',
                  popScore: 0,
                  'score': 14.372285656733615,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 3,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Entertainment',
                  'channels': ['Entertainment'],
                  'publishTime': {
                    internalValue: BigInt('13278621605000000')
                  },
                  'title': 'Alt-Rock Singer SK8 Shares How ‘Girl Next Door’ Represents The Evolution Of His Sound',
                  'description': 'After first making a splash with hip-hop, SK8 has reconnected with his punk roots on \'Girls Next Door,\' and he shares how this new direction came about, what it\'s like running a label, and what\'s next.',
                  'url': {
                    'url': 'https://hollywoodlife.com/2021/10/13/sk8-girl-next-door-interview/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/2904d64d82725230da6b1531aab85f54ff91cc55328e61f35319bcb3e5ba5abf.jpg.pad'
                    }
                  },
                  'publisherId': 'b4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Hollywood Life',
                  popScore: 0,
                  'score': 14.373802033258967,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Entertainment',
                  'channels': ['Entertainment'],
                  'publishTime': {
                    internalValue: BigInt('13278618574000000')
                  },
                  'title': 'Laverne Cox, 49, Rocks Plunging Black Swimsuit On Vacation: ‘Trans Is Beautiful’',
                  'description': 'Laverne Cox made a trans-positive statement while looking fiery hot in a sexy swimsuit on a luxury vacation.',
                  'url': {
                    'url': 'https://hollywoodlife.com/2021/10/13/laverne-cox-plunging-swimsuit-pool-video/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/a071a47e75db68c74fc1eaf6931d9420b2345a0c12b1247cdb05da135c0d5735.jpg.pad'
                    }
                  },
                  'publisherId': 'd4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Hollywood Life',
                  popScore: 0,
                  'score': 67.03023810006357,
                  'relativeTimeDescription': '1 hour ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Entertainment',
                  'channels': ['Entertainment'],
                  'publishTime': {
                    internalValue: BigInt('13278617426000000')
                  },
                  'title': 'Elizabeth Warren Urges Congress To ‘Step Up’ & Protect Roe V. Wade Amidst Texas Abortion Law',
                  'description': 'The Massachusetts senator also explained that the new law will be most harmful to people who don\'t have easy access to abortion while appearing on \'The View.\'',
                  'url': {
                    'url': 'https://hollywoodlife.com/2021/10/13/elizabeth-warren-roe-v-wade-the-view/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/d302f28efc872481f715f40ef457e419b7c35c03f514ca4a624aed099a8ec814.jpg.pad'
                    }
                  },
                  'publisherId': 'c5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Hollywood Life',
                  popScore: 0,
                  'score': 137.80582500043627,
                  'relativeTimeDescription': '2 hours ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Technology',
                  'channels': ['Technology'],
                  'publishTime': {
                    internalValue: BigInt('13278621600000000')
                  },
                  'title': 'Here are the best resin options for your SLA/DLP 3D printer',
                  'description': 'Resin printing is a little more complex than standard filament printing. Not only do you need a few must-have 3D printing accessories, but you also need to pick the right resin. When faced with multiple colors and multiple types, it\'s also easy to get turned around when choosing the right 3D printing resin. You want it to print quickly but stay strong without becoming brittle. We\'ve used as many as possible to bring you some of the best you can buy, but our favorite is Siraya Tech Fast, which pr',
                  'url': {
                    'url': 'https://www.windowscentral.com/best-resin-your-3d-printer?utm_source=feedburner&utm_medium=feed&utm_campaign=Feed%3A+wmexperts+%28Windows+Central%29'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/5afe29078068e7413a7f15a16657453217fc8ad630e8291ee1c66d69c2df6f48.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Windows Central',
                  popScore: 0,
                  'score': 14.381363717462444,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Technology',
                  'channels': ['Technology'],
                  'publishTime': {
                    internalValue: BigInt('13278621600000000')
                  },
                  'title': 'These are the best office chairs you can buy at any budget',
                  'description': 'The adage \"you get what you pay for\" definitely holds for certain categories of products, like shoes, mattresses, and yes, office chairs. The simple fact is, the more you have available to spend (up to a point), the better chair you can buy. The range of options for the best office chairs available is quite diverse and includes styles and features for just about any taste and preference. Our top pick is the AmazonBasics High-Back Leather Executive Chair. It\'s got the looks to fit in a corporate ',
                  'url': {
                    'url': 'https://www.androidcentral.com/best-office-chairs'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/b406b13e1fe493f9ab11140f5c33809a19125da6d5a3f1125810d299a9539da2.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Android Central',
                  popScore: 0,
                  'score': 14.381366799582283,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Technology',
                  'channels': ['Technology'],
                  'publishTime': {
                    internalValue: BigInt('13278621600000000')
                  },
                  'title': 'Stop Using Playlists to Look Cool and Start Using Them to Share Your Feelings',
                  'description': 'Do you struggle to express yourself with words? Consider turning to the one universal language we all share: emo playlists. I believe we, as a society, waste time trying to show off “aesthetic” music tastes. Instead, we should spend more time creating emotionally-charged, hyper-specific playlists. What’s more, we need…Read more...',
                  'url': {
                    'url': 'https://lifehacker.com/stop-using-playlists-to-look-cool-and-start-using-them-1847855875'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/87267b91579ebbf8ead3f305759376022fd50d3273209e851727cdf24b142304.jpg.pad'
                    }
                  },
                  'publisherId': 'c5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Lifehacker',
                  popScore: 0,
                  'score': 14.38136972344708,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 4,
          'items': [
            {
              article: undefined,
              promotedArticle: undefined,
              deal: {
                'data': {
                  'categoryName': 'Brave',
                  'channels': ['Brave'],
                  'publishTime': {
                    internalValue: BigInt('13258305902000000')
                  },
                  'title': 'Audible Plus',
                  'description': 'Listen anytime, anywhere to an unmatched selection of audiobooks, premium podcasts, and more',
                  'url': {
                    'url': 'https://www.amazon.com/hz/audible/mlp/mdp/discovery?ref_=assoc_tag_ph_1524216631897&_encoding=UTF8&camp=1789&creative=9325&linkCode=pf4&tag=bravesoftware-20&linkId=c6d187d14da9ca69e1a1a950348e100e'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/a29e3a601efa77ba5f2f35e58b40037b527f4e577112ea9967ced44741dcce32.jpg.pad'
                    }
                  },
                  'publisherId': '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Brave Offers',
                  popScore: 0,
                  'score': 33.65394030598059,
                  'relativeTimeDescription': '235 days ago'
                },
                'offersCategory': 'Discounts'
              }
            },
            {
              article: undefined,
              promotedArticle: undefined,
              deal: {
                'data': {
                  'categoryName': 'Brave',
                  'channels': ['Brave'],
                  'publishTime': {
                    internalValue: BigInt('13258305902000000')
                  },
                  'title': 'Amazon Prime Music',
                  'description': 'Listen to your favourite songs online from Brave.',
                  'url': {
                    'url': 'https://www.amazon.com/music/prime'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/04256e526b5cc73ddf7679ed907ac0f89e01d7d6af3a9d1c9faba288468c03ff.jpg.pad'
                    }
                  },
                  'publisherId': 'c5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Brave Offers',
                  popScore: 0,
                  'score': 67.3078806123441,
                  'relativeTimeDescription': '235 days ago'
                },
                'offersCategory': 'Discounts'
              }
            },
            {
              article: undefined,
              promotedArticle: undefined,
              deal: {
                'data': {
                  'categoryName': 'Brave',
                  'channels': ['Brave'],
                  'publishTime': {
                    internalValue: BigInt('13258305902000000')
                  },
                  'title': 'Amazon Prime',
                  'description': 'Enjoy exclusive Amazon Originals as well as popular movies and TV shows.',
                  'url': {
                    'url': 'https://www.amazon.com/amazonprime/146-1781179-3199520?_encoding=UTF8&camp=1789&creative=9325&linkCode=pf4&linkId=a402d5b2ca72ea0a267707ef10878979&primeCampaignId=prime_assoc_ft&ref_=assoc_tag_ph_1427739975520&tag=bravesoftware-20'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/c12d5200d342e72919e8420ccea6581ee4bf8e7ab510dd58bbc24d49ef22c36f.jpg.pad'
                    }
                  },
                  'publisherId': 'd4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Brave Offers',
                  popScore: 0,
                  'score': 134.6157612254411,
                  'relativeTimeDescription': '235 days ago'
                },
                'offersCategory': 'Discounts'
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Top News',
                  'channels': ['Top News'],
                  'publishTime': {
                    internalValue: BigInt('13278609481000000')
                  },
                  'title': '\'Havana Syndrome\' mystery expands with new cases at U.S. Embassy in Colombia',
                  'description': 'U.S. Embassy personnel in Bogota, Colombia, have reported symptoms aligned with the mysterious “Havana Syndrome” that continues to plague U.S. spies and diplomats around the globe. U.S. officials said Tuesday that two cases were initially reported by embassy personnel in the capital city, but said several others may have been ...',
                  'url': {
                    'url': 'https://www.washingtontimes.com/news/2021/oct/13/havana-syndrome-mystery-expands-new-cases-us-embas/?utm_source=RSS_Feed&utm_medium=RSS'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/4caf1b03489028604884ce33a6ac6f427f117de3670f4fefb047d92dccd3706c.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'The Washington Times',
                  popScore: 0,
                  'score': 304.21157985954886,
                  'relativeTimeDescription': '4 hours ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Health',
                  'channels': ['Health'],
                  'publishTime': {
                    internalValue: BigInt('13278602700000000')
                  },
                  'title': 'Tom Hardy Says He Was \'Really Overweight\' as Bane in \'Dark Knight Rises\'',
                  'description': '\"I was just bald, slightly porky, and with pencil arms.\"',
                  'url': {
                    'url': 'https://www.menshealth.com/weight-loss/a37947676/tom-hardy-overweight-bane-transformation-dark-knight-rises/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/2e81e99a21735bb5f0d3b7c8ac030a368b6e35711809a3475ba2bc6bdf7e300a.jpg.pad'
                    }
                  },
                  'publisherId': 'b4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Men\'s Health',
                  popScore: 0,
                  'score': 81223.05671641101,
                  'relativeTimeDescription': '6 hours ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Entertainment',
                  'channels': ['Entertainment'],
                  'publishTime': {
                    internalValue: BigInt('13278618011000000')
                  },
                  'title': 'Jodi! Tina! \'The Challenge: All Stars\' Brings in Heavy Hitters for Season 2',
                  'description': 'Welcome back! After the massive success of The Challenge: All Stars earlier this year, Paramount+’s reality show is back with another season and even more vets. TJ Lavin will return to host season 2, Parmount+ announced on Wednesday, October 13, with 24 cast members — some of whom haven’t competed in nearly 20 years. “With […]',
                  'url': {
                    'url': 'https://www.usmagazine.com/entertainment/pictures/the-challenge-all-stars-season-2-cast-includes-tina-jodi-and-more/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/c972ec42d40a1d2a5d2b180095407f119cd2c1b17ff41b5305c5b7987b7c0280.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Us Weekly',
                  popScore: 0,
                  'score': 544.0267160210122,
                  'relativeTimeDescription': '1 hour ago'
                }
              }
            }
          ]
        }
      ]
    },
    {
      items: [
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Sports',
                  'channels': ['Sports'],
                  'publishTime': {
                    internalValue: BigInt('13278621600000000')
                  },
                  'title': 'USMNT vs Costa Rica: TV channel, live stream, team news & preview',
                  'description': 'The Stars and Stripes tasted their first defeat since May when they fell to Panama on Sunday, but can quickly get back to winning ways',
                  'url': {
                    'url': 'https://www.goal.com/en/news/usmnt-vs-costa-rica-tv-channel-live-stream-team-news-preview/1ou1gnhu835jx174sq8yqn34os'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/ba5b2874d167e19150f3be8bf99a09717d912699a853321939dfbdef5bb9ff87.jpg.pad'
                    }
                  },
                  'publisherId': 'eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Goal.com',
                  popScore: 0,
                  'score': 14.381372591543293,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Culture',
                  'channels': ['Culture'],
                  'publishTime': {
                    internalValue: BigInt('13278621600000000')
                  },
                  'title': 'Tyga Turns Himself in to Police Following Domestic Violence Allegations From Ex-Girlfriend Camaryn Swanson',
                  'description': 'Rapper Tyga turned himself in early Tuesday to the Los Angeles Police Department following a domestic violence allegation brought forth from his ex-girlfriend Camaryn Swanson.Read more...',
                  'url': {
                    'url': 'https://www.theroot.com/tyga-turns-himself-in-to-police-following-domestic-viol-1847854486'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/3816eb674df32251c4b9fbda44a33acfe85764931d36c26d1f5af0cd86a0a960.jpg.pad'
                    }
                  },
                  'publisherId': '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'The Root',
                  popScore: 0,
                  'score': 14.381375498821068,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Business',
                  'channels': ['Business'],
                  'publishTime': {
                    internalValue: BigInt('13278621593000000')
                  },
                  'title': 'Fastenal Company 2021 Q3 - Results - Earnings Call Presentation',
                  'description': '',
                  'url': {
                    'url': 'https://seekingalpha.com/article/4459715-fastenal-company-2021-q3-results-earnings-call-presentation?source=feed_all_articles'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/07fd83154e9157f70207bd1aa6dd7998ec3aa6a1b730aef93868a9ac950e912e.jpg.pad'
                    }
                  },
                  'publisherId': 'fc5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Seeking Alpha',
                  popScore: 0,
                  'score': 14.391950109691049,
                  'relativeTimeDescription': '23 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Sports',
                  'channels': ['Sports'],
                  'publishTime': {
                    internalValue: BigInt('13278621564000000')
                  },
                  'title': 'One-handed CB Marshon Lattimore still playing lights-out: Saints takeaways vs. Washington',
                  'description': 'Marshon Lattimore and WR Deonte Harris were standouts in the Saints\' 33-22 win over Washington, and they weren\'t the only ones.',
                  'url': {
                    'url': 'https://theathletic.com/2885011/2021/10/13/one-handed-cb-marshon-lattimore-still-playing-lights-out-saints-takeaways-vs-washington/?source=rss'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/dbb85cea85bf1cb02a2e31827af8f4621a7fe1e3cd465739db8cda2e5c45e5db.jpg.pad'
                    }
                  },
                  'publisherId': 'd4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'The Athletic',
                  popScore: 0,
                  'score': 14.434964041654474,
                  'relativeTimeDescription': '24 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 6,
          'items': [
            {
              article: undefined,
              deal: undefined,
              promotedArticle: {
                'data': {
                  'categoryName': 'Brave Partners',
                  'channels': ['Brave Partners'],
                  'publishTime': {
                    internalValue: BigInt('13278528021000000')
                  },
                  'title': 'The Beginner’s Guide to Account-Based Marketing (ABM)',
                  'description': 'This leads to a common paradox—marketing can hit its goals by bringing in a high volume of leads, but sales can’t hit its goals because those same leads are poorly qualified. Account-based marketing (ABM) aims to fix that by tightly…Read more ›',
                  'url': {
                    'url': 'https://ahrefs.com/blog/account-based-marketing/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/9d2f7ab67a81520a1e281d9def23ce4f4fbcf46cac8be01707f0cd0fb24e597a.jpg.pad'
                    }
                  },
                  'publisherId': 'b4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Ahrefs',
                  popScore: 0,
                  'score': 22.921395422138495,
                  'relativeTimeDescription': '1 day ago'
                },
                'creativeInstanceId': '2626e169-a372-42ca-af14-b0df795d2819'
              }
            }
          ]
        },
        {
          'cardType': 2,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Brave',
                  'channels': ['Brave'],
                  'publishTime': {
                    internalValue: BigInt('13276800071000000')
                  },
                  'title': 'Brave Launches Brave Talk for Privacy-Preserving Video Conferencing',
                  'description': 'Today, Brave launched Brave Talk, a new privacy-focused video conferencing feature built directly into the Brave browser.\nThe post Brave Launches Brave Talk for Privacy-Preserving Video Conferencing appeared first on Brave Browser.',
                  'url': {
                    'url': 'https://brave.com/brave-talk-launch/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/8bc59275469ab8db7c8245e5269b5bfa84a8e77586de349dd1bce0435c27a6a5.jpg.pad'
                    }
                  },
                  'publisherId': 'b4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Brave Blog',
                  popScore: 0,
                  'score': 28.831838413657156,
                  'relativeTimeDescription': '21 days ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Brave',
                  'channels': ['Brave'],
                  'publishTime': {
                    internalValue: BigInt('13276723068000000')
                  },
                  'title': 'Research Paper: Privacy and Security Issues in Web 3.0',
                  'description': 'We at Brave Research just published a technical report called “Privacy and Security Issues in Web 3.0” on arXiv. This blog post summarizes our findings and puts them in perspective for Brave users.\nThe post Research Paper: Privacy and Security Issues in Web 3.0 appeared first on Brave Browser.',
                  'url': {
                    'url': 'https://brave.com/research-paper-privacy-and-security-issues-in-web-3-0/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/260702227df254d2ada663cbdb14442933161f55da5b891d59f82335015a025d.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Brave Blog',
                  popScore: 0,
                  'score': 57.82917689204489,
                  'relativeTimeDescription': '22 days ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Brave',
                  'channels': ['Brave'],
                  'publishTime': {
                    internalValue: BigInt('13276712102000000')
                  },
                  'title': 'What’s Brave Done For My Privacy Lately? Episode #10: Custom Filter List Subscriptions',
                  'description': 'This is the tenth in a series of blog posts on new Brave privacy features. This post describes work done by Anton Lazarev, Research Engineer. Authors: Peter Snyder and Anton Lazarev.\nThe post What’s Brave Done For My Privacy Lately? Episode #10: Custom Filter List Subscriptions appeared first on Brave Browser.',
                  'url': {
                    'url': 'https://brave.com/privacy-updates-10/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/2d9c785ca6d8dcdcf2d2046796d97a81cb99a002fb91895cca43a6846aeb3a2f.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Brave Blog',
                  popScore: 0,
                  'score': 115.70439692971131,
                  'relativeTimeDescription': '22 days ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Entertainment',
                  'channels': ['Entertainment'],
                  'publishTime': {
                    internalValue: BigInt('13278621540000000')
                  },
                  'title': 'We see, acknowledge, and connect with Larry David in the trailer for Curb Your Enthusiasm’s new season',
                  'description': 'Larry David and his seemingly endless array of irritations are returning for another glorious season of Curb Your Enthusiasm on October 24. But you don’t have to wait until then to see how stupid things like prayers and toasts are. They’re all right here in the brand new trailer. Read more...',
                  'url': {
                    'url': 'https://www.avclub.com/we-see-acknowledge-and-connect-with-larry-david-in-th-1847856248'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/d299252e34751a7d534376404149e30c574809d4f2a297dfc8ef4ac97e55fa3d.jpg.pad'
                    }
                  },
                  'publisherId': 'd4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'The A.V. Club',
                  popScore: 0,
                  'score': 14.469881193573206,
                  'relativeTimeDescription': '24 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Business',
                  'channels': ['Business'],
                  'publishTime': {
                    internalValue: BigInt('13278621540000000')
                  },
                  'title': 'Stocks Up Slightly After Inflation Data, Major Earnings',
                  'description': 'U.S. share benchmarks ticked up as fresh consumer-price data boosted the view that the bout of elevated inflation might last longer.',
                  'url': {
                    'url': 'https://www.wsj.com/articles/global-stock-markets-dow-update-10-13-2021-11634110620?mod=rss_markets_main'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/9a51fc8831ae2c54f2a02293bc472e88abe2f9ded6310dc18506badcf581837a.jpg.pad'
                    }
                  },
                  'publisherId': 'a5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'WSJ - Markets',
                  popScore: 0,
                  'score': 14.469883937507502,
                  'relativeTimeDescription': '24 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Top News',
                  'channels': ['Top News'],
                  'publishTime': {
                    internalValue: BigInt('13278621508000000')
                  },
                  'title': 'Fox News Hosts Shout Down Liberal Panelist for Saying NYC Is No ‘Hellhole’',
                  'description': 'Fox NewsFox News hosts Lisa “Kennedy” Montgomery and Julie Banderas on Wednesday took turns berating a liberal panelist for having the temerity to claim that dispute their claim that New York City is a “hellhole.”Kennedy and Banderas, in an effort to bolster their case that the city is a terrifying, crime-ridden wasteland, boasted that they “talk to cops” and read the New York Post as evidence for their claims. Besides whipping their viewers into a frenzy over vaccine mandates and critical race ',
                  'url': {
                    'url': 'https://www.thedailybeast.com/fox-news-hosts-kennedy-and-julie-banderas-shout-down-liberal-panelist-for-saying-nyc-is-no-hellhole?source=articles&via=rss&utm_source=feedburner&utm_medium=feed&utm_campaign=Feed%3A+thedailybeast%2Farticles+%28The+Daily+Beast+-+Latest+Articles%29'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/3cdc6866dee932665c37ac1ae515c360ff77b47c6a555c3f02a534a79bd49308.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'The Daily Beast',
                  popScore: 0,
                  'score': 14.515511304097387,
                  'relativeTimeDescription': '25 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Health',
                  'channels': ['Health'],
                  'publishTime': {
                    internalValue: BigInt('13278621506000000')
                  },
                  'title': 'Hilarie Burton Morgan on the Problem With True Crime, Life on the Farm, and Her ‘One Tree Hill’ Podcast',
                  'description': 'Plus how she\'s practicing self-care.',
                  'url': {
                    'url': 'https://www.self.com/story/hilarie-burton-morgan-interview'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/3d1c2c118390f7d72a6502148c8733a2cbcb3a4eb3795307ab5a6c8b43671f19.jpg.pad'
                    }
                  },
                  'publisherId': 'a5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'SELF',
                  popScore: 0,
                  'score': 14.518330759046432,
                  'relativeTimeDescription': '25 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Business',
                  'channels': ['Business'],
                  'publishTime': {
                    internalValue: BigInt('13278621467000000')
                  },
                  'title': 'Plug Power jumps 13% after it partners with Airbus to study and develop hydrogen-powered air travel',
                  'description': 'Airbus is working towards a goal of bringing zero-emission aircraft to market by 2035, and it thinks hydrogen could help achieve that objective.',
                  'url': {
                    'url': 'https://markets.businessinsider.com/news/stocks/plug-power-stock-price-airbus-partnership-hydrogen-airport-study-phillips-2021-10'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/932bcb6a3ce07dc885c0ce7b881c3dfeb9fb643e6d5ce968decf14d1983dbbdd.jpg.pad'
                    }
                  },
                  'publisherId': 'b4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Business Insider',
                  popScore: 0,
                  'score': 14.572489911501611,
                  'relativeTimeDescription': '25 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Top News',
                  'channels': ['Top News'],
                  'publishTime': {
                    internalValue: BigInt('13278621435000000')
                  },
                  'title': '60K Film and TV Workers May Strike Within Days as Industry Still Recovers From Pandemic',
                  'description': '\"Without an end date, we could keep talking forever,\" said President of the International Alliance of Theatrical Stage Employees Matthew Loeb.',
                  'url': {
                    'url': 'https://www.newsweek.com/60k-film-tv-workers-may-strike-within-days-industry-still-recovers-pandemic-1638658'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/1e4526119064ccbd65d5fc372fe878ff60b64c1ba9047f3cb664b2afdeddf73f.jpg.pad'
                    }
                  },
                  'publisherId': 'fc5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Newsweek',
                  popScore: 0,
                  'score': 14.615857594582533,
                  'relativeTimeDescription': '26 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 5,
          'items': []
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Technology',
                  'channels': ['Technology'],
                  'publishTime': {
                    internalValue: BigInt('13278621421000000')
                  },
                  'title': 'Apple AirPods 3 rumored to debut alongside new MacBook Pros at October 18 Unleashed event',
                  'description': ' Yesterday Apple confirmed a new “Unleashed” launch event scheduled on October 18. The stars of the show will almost certainly be  Apple’s next-generation ARM-based M-series chipset dubbed M1X and all-new 14” and 16” MacBook Pro laptops. Now Weibo leaker @PandaIsBald is also throwing in the long-rumored AirPods 3 to the mix.\n\n\n\n\nAirPods 3 leaked design (images: 52audio.com)\n\nApple’s regular non-Pro AirPods 2 have been out since March 2019 and are clearly due for an update. The third generation w',
                  'url': {
                    'url': 'https://www.gsmarena.com/apple_airpods_3_rumored_to_debut_alongside_new_macbook_pros_at_october_18_unleashed_event-news-51408.php'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/3352539771a4ac7482f4486de7900c458f052e11b605609fafcf206bb4634075.jpg.pad'
                    }
                  },
                  'publisherId': 'a5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'GSMArena',
                  popScore: 0,
                  'score': 14.634541104175572,
                  'relativeTimeDescription': '26 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Business',
                  'channels': ['Business'],
                  'publishTime': {
                    internalValue: BigInt('13278621420000000')
                  },
                  'title': ': Starbucks and Netflix partner for series tied to Netflix’s book club',
                  'description': 'Starbucks and Netflix have partnered for a series that will discuss how the streaming service\'s book club choices are transformed into movies and shows\n   \n',
                  'url': {
                    'url': 'https://www.marketwatch.com/story/starbucks-and-netflix-partner-for-series-tied-to-netflixs-book-club-11634130338?rss=1&siteid=rss'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/98683b85d91545ed868557351adee4d1b6fa45337082c356c36cedcc9a1c0085.jpg.pad'
                    }
                  },
                  'publisherId': '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'MarketWatch',
                  popScore: 0,
                  'score': 14.635871140907735,
                  'relativeTimeDescription': '26 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 3,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Culture',
                  'channels': ['Culture'],
                  'publishTime': {
                    internalValue: BigInt('13278621400000000')
                  },
                  'title': 'Cheat Maker Is Not Afraid of Call of Duty’s New Kernel-Level Anti-Cheat',
                  'description': 'Activision announced the launch of a kernel-level anti-cheat system called RICOCHET to fight cheaters.',
                  'url': {
                    'url': 'https://www.vice.com/en/article/z3xjqa/cheat-maker-is-so-far-not-afraid-of-call-of-dutys-new-kernel-level-anti-cheat'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/f2dbdd22cf97479e7c2bccae2d3becc8346edd6b603ca2350ee02c35546de347.jpg.pad'
                    }
                  },
                  'publisherId': 'eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'VICE',
                  popScore: 0,
                  'score': 14.662245395018518,
                  'relativeTimeDescription': '27 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Culture',
                  'channels': ['Culture'],
                  'publishTime': {
                    internalValue: BigInt('13278620323000000')
                  },
                  'title': 'Airlines Are Already Defying the Texas Ban on Vaccine Mandates',
                  'description': 'Gov. Greg Abbott made companies choose between state law and federal regulations, and Southwest and American Airlines made that choice pretty fast.',
                  'url': {
                    'url': 'https://www.vice.com/en/article/g5gzw7/airlines-are-defying-texas-ban-on-vaccine-mandates'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/39c1559de30f9abf91d6035ad655be5e8ba09f64ab4a73201010cb8921248068.jpg.pad'
                    }
                  },
                  'publisherId': 'c5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'VICE',
                  popScore: 0,
                  'score': 31.45969721478307,
                  'relativeTimeDescription': '45 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Culture',
                  'channels': ['Culture'],
                  'publishTime': {
                    internalValue: BigInt('13278620305000000')
                  },
                  'title': 'The Best Dog DNA Tests That Actually Work, According to the Dog-Obsessed',
                  'description': 'Help your pup assume their rightful throne as king of the dog park with the best dog DNA tests, according to convinced and happy pet owners.',
                  'url': {
                    'url': 'https://www.vice.com/en/article/dyvad7/best-dog-dna-tests'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/c6147aad0103a239ba49c79fa7696593bbf82558ba08a5362462fbbeea0a195c.jpg.pad'
                    }
                  },
                  'publisherId': '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'VICE',
                  popScore: 0,
                  'score': 62.9745088197745,
                  'relativeTimeDescription': '45 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Sports',
                  'channels': ['Sports'],
                  'publishTime': {
                    internalValue: BigInt('13278621373000000')
                  },
                  'title': 'Poor IPL run not a concern, Nicholas Pooran wants to just \'refocus and go again\'',
                  'description': '\"We won two World Cups without getting singles\" - West Indies vice-captain says power-hitting will still be big on the team\'s agenda',
                  'url': {
                    'url': 'https://www.espncricinfo.com/story/t20-world-cup-west-indies-vice-captain-nicholas-pooran-wants-to-just-refocus-and-go-again-after-poor-ipl-2021-1282815?ex_cid=OTC-RSS'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/01dde228cf27287b3ef5d3c9b5293c7b8cdaa59e269aa856f080c535016b7e01.jpg.pad'
                    }
                  },
                  'publisherId': 'eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'ESPN - Cricket',
                  popScore: 0,
                  'score': 14.697300142885872,
                  'relativeTimeDescription': '27 minutes ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Top News',
                  'channels': ['Top News'],
                  'publishTime': {
                    internalValue: BigInt('13278621361000000')
                  },
                  'title': 'Navy engineer smuggled secrets \'a few at a time\' and wanted to meet \'foreign spies\' for drinks',
                  'description': 'Written communications between Jonathan Toebbe and an undercover FBI agent posing as a foreign spy show that he collected secret data over the years. Toebbe also planned to be extracted from the US.',
                  'url': {
                    'url': 'https://www.dailymail.co.uk/news/article-10087987/Navy-engineer-smuggled-secrets-time-wanted-meet-foreign-spies-drinks.html?ns_mchannel=rss&ns_campaign=1490&ito=1490'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/876f15ba1c5582a0bef82b3cd6b2d567d04d9394ef208dc3e3749a3d3da46df2.jpg.pad'
                    }
                  },
                  'publisherId': 'c5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Daily Mail',
                  popScore: 0,
                  'score': 14.712686451514855,
                  'relativeTimeDescription': '27 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Technology',
                  'channels': ['Technology'],
                  'publishTime': {
                    internalValue: BigInt('13278621350000000')
                  },
                  'title': 'How Quickbase is using low-code to streamline Daifuku’s supply chain',
                  'description': 'At VentureBeat\'s Low-Code/No-Code Summit, Quickbase explained how its platform helps connect and automate systems, processes, and workloads.',
                  'url': {
                    'url': 'https://venturebeat.com/2021/10/13/how-quickbase-is-using-low-code-to-streamline-daifukus-supply-chain/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/e1bab8826e0dc937e0c1ee4f79a3e63d3fbc8caf11773259dbea1cb3202b9a1a.jpg.pad'
                    }
                  },
                  'publisherId': 'a5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'VentureBeat',
                  popScore: 0,
                  'score': 14.726687401901625,
                  'relativeTimeDescription': '27 minutes ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 4,
          'items': [
            {
              article: undefined,
              promotedArticle: undefined,
              deal: {
                'data': {
                  'categoryName': 'Brave',
                  'channels': ['Brave'],
                  'publishTime': {
                    internalValue: BigInt('13256920996000000')
                  },
                  'title': 'Sony 55 Inch TV',
                  'description': 'BRAVIA OLED 4K Ultra HD Smart TV with HDR and Alexa Compatibility',
                  'url': {
                    'url': 'https://www.amazon.com/dp/B084KQFNBX?tag=bravesoftware-20&linkCode=osi&th=1&psc=1&language=en_US&currency=USD'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/6b0de702b8c1596cb713c89f3b79b568959cfebf4af6611e63c54d39a43e0233.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Brave Offers',
                  popScore: 0,
                  'score': 270.2865946654164,
                  'relativeTimeDescription': '251 days ago'
                },
                'offersCategory': 'Electronics'
              }
            },
            {
              article: undefined,
              promotedArticle: undefined,
              deal: {
                'data': {
                  'categoryName': 'Brave',
                  'channels': ['Brave'],
                  'publishTime': {
                    internalValue: BigInt('13256920996000000')
                  },
                  'title': 'Samsung Galaxy Tab S7',
                  'description': 'Go for hours on a single charge, and back to 100% with the fast-charging USB-C port.',
                  'url': {
                    'url': 'https://www.amazon.com/dp/B08FBN5STQ?tag=bravesoftware-20&linkCode=osi&th=1&psc=1&language=en_US&currency=USD'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/b8b0e9f54885248f635c620d638c716164972f0b7f72c5ec79357517e2d2a171.jpg.pad'
                    }
                  },
                  'publisherId': 'fc5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Brave Offers',
                  popScore: 0,
                  'score': 540.5731893340486,
                  'relativeTimeDescription': '251 days ago'
                },
                'offersCategory': 'Electronics'
              }
            },
            {
              article: undefined,
              promotedArticle: undefined,
              deal: {
                'data': {
                  'categoryName': 'Brave',
                  'channels': ['Brave'],
                  'publishTime': {
                    internalValue: BigInt('13256920996000000')
                  },
                  'title': 'Samsung Curved LED-Lit Monitor',
                  'description': 'A stylish design featuring a Black body metallic finish and sleek curves',
                  'url': {
                    'url': 'https://www.amazon.com/dp/B079K3MXWF?tag=bravesoftware-20&linkCode=osi&th=1&psc=1&language=en_US&currency=USD'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/e2bc4f0ca84cc9bc7183a92072bf41cbcdd4bd4e31da0a5d6e084a1da2c2c7fd.jpg.pad'
                    }
                  },
                  'publisherId': 'b4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Brave Offers',
                  popScore: 0,
                  'score': 1081.146378673966,
                  'relativeTimeDescription': '251 days ago'
                },
                'offersCategory': 'Electronics'
              }
            }
          ]
        },
        {
          'cardType': 0,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Cars',
                  'channels': ['Cars'],
                  'publishTime': {
                    internalValue: BigInt('13278542934000000')
                  },
                  'title': 'Rivian’s first retail hub to open in Venice, CA, as ‘a space to gather’',
                  'description': '\nFresh off the heels of delivering its flagship R1T electric pickup to first customers, Rivian has shared details of its first hub, centered in Venice, California. Rather than existing as solely a retail space, Rivian hopes this first of several hub locations will offer a space for public gatherings and encourages its community to visit and connect. If you happen to order a $70,000 EV while you’re there… well that’s a welcomed option as well.\n more…\nThe post Rivian’s first retail hub to open in ',
                  'url': {
                    'url': 'https://electrek.co/2021/10/12/rivians-first-retail-hub-to-open-in-venice-ca-as-a-space-to-gather/'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/c85762ffe3bd48886ed1ce58f868958989ad89b0723f5844a7ffe0fef57fa1c5.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Electrek',
                  popScore: 0,
                  'score': 23121.416404237974,
                  'relativeTimeDescription': '22 hours ago'
                }
              }
            }
          ]
        },
        {
          'cardType': 1,
          'items': [
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Business',
                  'channels': ['Business'],
                  'publishTime': {
                    internalValue: BigInt('13278518400000000')
                  },
                  'title': 'Tech’s Exponential Growth – and How to Solve the Problems It’s Created',
                  'description': 'Understanding and improving the impact that Big Tech has on society\n\n \n',
                  'url': {
                    'url': 'https://hbr.org/podcast/2021/10/techs-exponential-growth-and-how-to-solve-the-problems-its-created?utm_source=feedburner&utm_medium=feed&utm_campaign=Feed%3A+harvardbusiness+%28HBR.org%29'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/703dace4d9ab2590c184826c2a7b39ed2249cace04101f891c011c31021a3b9d.jpg.pad'
                    }
                  },
                  'publisherId': '4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'Harvard Business Review',
                  popScore: 0,
                  'score': 5917.313882859709,
                  'relativeTimeDescription': '1 day ago'
                }
              }
            },
            {
              promotedArticle: undefined,
              deal: undefined,
              article: {
                isDiscover: true,
                'data': {
                  'categoryName': 'Top News',
                  'channels': ['Top News'],
                  'publishTime': {
                    internalValue: BigInt('13278596700000000')
                  },
                  'title': 'Global Climate Pledges Off Track to Meet Paris Targets, IEA Says',
                  'description': 'Whether lawmakers continue existing policies or make good on recent promises, rising temperatures will exceed the limit global leaders committed to in the Paris Agreement, the International Energy Agency said.',
                  'url': {
                    'url': 'https://www.wsj.com/articles/governments-climate-pledges-not-enough-to-meet-paris-agreement-targets-iea-says-11634097601'
                  },
                  'urlHash': '',
                  'image': {
                    imageUrl: undefined,
                    paddedImageUrl: {
                      'url': 'https://pcdn.brave.com/brave-today/cache/1261719cac8a7c5d7d5ab0d2cc3b5b6a43cf177b3fd0a103f366f59b397efd9d.jpg.pad'
                    }
                  },
                  'publisherId': 'eb4eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
                  'publisherName': 'WSJ',
                  popScore: 0,
                  'score': 325.5895923286794,
                  'relativeTimeDescription': '7 hours ago'
                }
              }
            }
          ]
        }
      ]
    }
  ]
}

export const mockBraveNewsController: Partial<BraveNewsControllerRemote> = {
  async getLocale() {
    return { locale: 'en-US' }
  },

  async getFeed() {
    return { feed }
  },

  async getPublishers() {
    return { publishers }
  },

  async getChannels() {
    const channelNames = Object.values(publishers).reduce((prev, next) => {
      for (const locale of next.locales) {
        for (const channel of locale.channels) {
          prev.add(channel)
        }
      }
      prev.add(next.categoryName)
      return prev
    }, new Set<string>())

    return {
      channels: Array.from(channelNames).map(c => ({ channel: c, isSubscribed: false }))
    }
  },

  async setChannelSubscribed(channelId, subscribed) {
    return { updated: { ...(await mockBraveNewsController.getChannels!())[channelId], subscribed } }
  },

  async findFeeds() {
    return { results: [] }
  },

  async subscribeToNewDirectFeed() {
    return { isValidFeed: false, isDuplicate: false, publishers: await mockBraveNewsController.getChannels!() }
  },

  async setPublisherPref(publisherId, newStatus) {
    publishers[publisherId].userEnabledStatus = newStatus
    return {
      newStatus
    }
  }
}

// @ts-expect-error
window.storybookBraveNewsController = mockBraveNewsController
