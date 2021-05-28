// Before adding to this list, get approval from the security team
module.exports = [
  'http://update.googleapis.com/service/update2', // allowed because it 307's to go-updater.brave.com. should never actually connect to googleapis.com.
  'https://update.googleapis.com/service/update2', // allowed because it 307's to go-updater.brave.com. should never actually connect to googleapis.com.
  'https://safebrowsing.googleapis.com/v4/threatListUpdates', // allowed because it 307's to safebrowsing.brave.com
  'https://clients2.googleusercontent.com/crx/blobs/',
  'http://dl.google.com/', // allowed because it 307's to redirector.brave.com
  'https://dl.google.com/', // allowed because it 307's to redirector.brave.com
  'https://no-thanks.invalid/', // fake gaia URL
  'https://go-updater.brave.com/',
  'https://safebrowsing.brave.com/',
  'https://brave-core-ext.s3.brave.com/',
  'https://laptop-updates.brave.com/', // stats/referrals
  'https://static.brave.com/',
  'https://static1.brave.com/',
  'http://componentupdater.brave.com/service/update2', // allowed because it 307's to https://componentupdater.brave.com
  'https://componentupdater.brave.com/service/update2',
  'https://crlsets.brave.com/',
  'https://crxdownload.brave.com/crx/blobs/',
  'https://updates.bravesoftware.com/', // Omaha/Sparkle
  'https://p3a.brave.com/',
  'https://dns.google/dns-query', // needed for DoH on Mac build machines
  'https://chrome.cloudflare-dns.com/dns-query', // needed for DoH on Mac build machines
  'https://tor.bravesoftware.com/', // for fetching tor client updater component
  'https://redirector.brave.com/',
  'https://sync-v2.brave.com/v2', // brave sync v2 production
  'https://sync-v2.bravesoftware.com/v2', // brave sync v2 staging
  'https://sync-v2.brave.software/v2', // brave sync v2 dev
  'https://variations.brave.com/seed', // brave A/B testing
  'https://brave-today-cdn.brave.com/', // Brave Today (production)
  'https://pcdn.brave.com/', // Brave's Privacy-focused CDN
]
