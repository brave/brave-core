/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const locale = {
  // General
  learnMore: 'Learn more',
  done: 'Done',
  searchSettings: 'Search settings',
  headerLabel: 'you are inside a',

  // Header Privaste Window
  headerTitle: 'Private Window',
  headerText: 'Brave doesn’t record browsing information while in a Private Window. Sites you visit aren’t recorded in your history and cookies are deleted on close. Your activity is not completely anonymous though and may still be visible to your ISP or websites you visit.',
  headerButton: 'Learn more about Private Windows',

  // Header Private Window with Tor
  headerTorTitle: 'Private Window with Tor',
  headerTorText: 'Brave doesn’t record browsing information while in a Private Window. Sites you visit aren’t recorded in your history and cookies are deleted on close. Your activity is not completely anonymous though and may still be visible to your ISP or websites you visit.',
  headerTorButton: 'Learn more about Private Windows with Tor',

  // Box for DDG
  boxDdgLabel: 'Search Privately with',
  boxDdgTitle: 'DuckDuckGo',
  boxDdgText: 'Brave recommends and defaults to using DuckDuckGo to search the web while inside private windows. DuckDuckGo is the search engine that doesn’t track you.  No personal information or data is collected by DuckDuckGo enabling you to search with more privacy.',
  boxDdgText2: 'DuckDuckGo is the default search engine while in Private Windows with Tor. DuckDuckGo doesn’t track your search history and with Tor protection, your location is anonymized. You can change the search engine in settings but most popular alternatives behave poorly with Tor enabled.',
  boxDdgButton: 'Search with DuckDuckGo',

  // Box for Tor
  boxTorLabel: 'Browse anonymously with',
  boxTorTitle: 'Tor Protection',
  boxTorText: 'Tor anonymizes your traffic from the sites you visit by sending your connection through several Tor servers before it reaches your destination. These connections are encrypted, so your ISP or employer can’t see which sites you’re visiting either. Tor can slow down browsing and some might not work at all.',
  boxTorText2: 'Browsing in a Private Window only changes what Brave records on your device. Your activity is not completely anonymous to others and may still be visible to your service provider or websites you visit. Tor hides your IP address from the sites you visit, and hides the sites you visit from your ISP or your employer.',
  boxTorButton: 'Learn more about Tor in Brave',

  // Modal Private Window with Tor
  modalPrivateWindowTorTitle: 'Private Window with Tor',
  modalPrivateWindowTorDisclaimer1: 'When you use Tor, Brave doesn’t connect directly to a website like normal. Instead, you connect to a chain of three different computers in the volunteer-run Tor network, one after another, and only then to the website you’re visiting. Between those three Tor computers, only one knows where your connection is really coming from and only one knows where it’s really going. And those two don’t even talk to each other because there’s another computer in the middle!',
  modalPrivateWindowTorDisclaimer2: 'From the perspective of the websites you visit, it looks like your connection is coming from that last Tor computer — sites don’t learn your real IP address. But they can tell that the connection is being shuffled around by Tor because the list of Tor computers isn’t a secret. Some sites will treat you very differently because of this. Most often they’ll just keep asking you to prove you’re human. If a site relies on a feature which would reveal your real IP address or make you much easier to recognize when you’re not using Tor, that site might not work at all because we’d rather be safe than sorry.',
  modalPrivateWindowTorDisclaimer3: 'Your ISP or employer or the owner of the WiFi network you’re connected to also won’t see which sites you visit because Brave doesn’t connect directly to the site. Instead, someone watching your network connection only sees that you’re making a connection to the Tor network. Some network owners try to block connections to the Tor network because they want to decide which sites you get to visit, and Tor lets you gets around that blocking.',
  modalPrivateWindowTorDisclaimer4: 'If your employer administers your device they might also keep track of what you do with it. Using Private windows, even with Tor, probably won\'t stop them from knowing which sites you\'ve visited. Someone else with access to your device could also have installed software which monitors your activity, and Brave can’t protect you from this either.',
  modalPrivateWindowTorDisclaimer5: 'With Tor, Brave works hard to ensure that you’re extremely difficult to track online while providing a delightful browsing experience. But if your personal safety depends on remaining anonymous you may wish to use the Tor Browser from https://torproject.org/ instead.',
  modalPrivateWindowTorDisclaimer6: 'You can learn more about Tor at https://www.torproject.org/about/overview.html.en',

  // Modal Private Window
  modalPrivateWindowTitle: 'Private Window',
  modalPrivateWindowDisclaimer1: 'Private Windows aren’t recorded in your browser history, and they don’t contribute attention towards Brave Rewards. When you’re done and you close your Private Window, Brave throws away all your cookies and so on, which logs you out of sites you visited in the Private Window.',
  modalPrivateWindowDisclaimer2: 'Unless you use Tor, sites learn your IP address when you visit them — even in a Private Window. From your IP address, they can often guess roughly where you are — typically your city. Sometimes that location guess can be much more specific. Sites also know everything you specifically tell them, such as search terms. If you log into a site, they\'ll know you\'re the owner of that account. You\'ll still be logged out when you close the Private Window because Brave will throw away the cookie which keeps you logged in. Although Brave works hard to make you hard to track online, sites could remember you by your IP address (or some other ways to recognise your device), and connect your browsing in a Private Window with your other browsing in Brave.',
  modalPrivateWindowDisclaimer3: 'Whoever connects you to the Internet (your ISP) can see all of your network activity. Often, this is your mobile carrier. If you\'re connected to a WiFi network, this is the owner of that network, and if you\'re using a VPN, then it\'s whoever runs that VPN. Your ISP can see which sites you visit as you visit them. If those sites use HTTPS, they can\'t make much more than an educated guess about what you do on those sites. But if a site only uses HTTP then your ISP can see everything: your search terms, which pages you read, and which links you follow.',
  modalPrivateWindowDisclaimer4: 'If an employer manages your device, they might also keep track of what you do with it. Using Private Windows probably won\'t stop them from knowing which sites you\'ve visited. Someone else with access to your device could also have installed software which monitors your activity, and Private Windows can\'t protect you from this either.',

  // Modal Tor in Brave
  modalTorInBraveTitle: 'Tor in Brave',
  modalTorInBraveDisclaimer1: 'Ross edit me',
  modalTorInBraveDisclaimer2: 'Ross edit me too',

  // Modal DuckDuckGo
  modalDuckduckGoTitle: 'DuckDuckGo',
  modalDuckduckGoDisclaimer1: 'Ross edit me',
  modalDuckduckGoDisclaimer2: 'Ross edit me too'
}

export default locale
