# Search Query Metrics

This feature measures search activity in the Brave browser in a privacy-preserving way by sending a lightweight analytics ping when a user views search results, without ever collecting the search query. The ping contains a [payload](#payload) with key-value pairs and is sent over OHTTP, ensuring network-level unlinkability. To reduce the risk of fingerprinting, all values reported are picked from a limited set of options.

### Payload

> [!IMPORTANT]
> Approval from the privacy team is required for changes to the existing payload or for new payload metrics.

The following fields are included in the search query metric payload sent by the browser.

| key  | example  | description  |
|---|---|---|
| buildChannel  | {"buildChannel":"beta"}  | Browser build channel:<br><br>- release<br>- beta<br>- nightly<br>- developer  |
| country  | {"country":"US"}  | Two-letter ISO 3166-1 alpha-2 country code representing the user's current country, determined by IP geolocation.  |
| defaultSearchEngne  | {"defaultSearchEngne":"Brave"}  | User’s configured default search engine.  |
| entryPoint  | {"entryPoint":"direct"}  | How the search was initiated (Bookmark, Direct, NTP, Omnibox History, Omnibox Suggestion, Omnibox Search, Quick Search (Mobile only), Shortcut (Desktop only), Top Site, or Other).  |
| isDefaultBrowser  | {"isDefaultBrowser":false}  | Indicates whether Brave is set as the system's default browser.  |
| isFirstQuery  | {"isFirstQuery":true}  | Whether this is the first search query for the given UTC day.  |
| language  | {"language":"en"}  | Language derived from the device's system language settings.  |
| platform  | {"platform":"windows"}  | Operating system:<br><br>- windows<br>- macos<br>- linux<br>- android<br>- ios.  |
 | searchEngine  | {"searchEngine":"Brave"}  | The search engine used for the query. If it is one of the following search engines, its name appears in the payload. Otherwise, it is reported as “Other” to maintain user privacy.<br><br>- Brave<br>- Google<br>- DuckDuckGo<br>- Qwant<br>- Bing<br>- Startpage<br>- Ecosia<br>- Yahoo! JAPAN<br>- ChatGPT<br>- Perplexity  |
| studies  | {"studies": {"BraveSearch.FooStudy":"GroupA", "BraveSearch.BarStudy":"GroupB"}}  | User studies used for A/B testing. Configurable through Griffin with the “BraveSearch.” prefix.  |
| transactionId  | {"transactionId":"a889d841-5df2-464a-a196-6dbbcad9b42f"}  | Randomly generated UUIDv4 for each transaction, used for deduplication.  |
| type  | {"type":"query"}  | Event type.  |
| versionNumber  | {"versionNumber":"1.87.0"}  | Brave browser version number with fixed value for patch.  |
