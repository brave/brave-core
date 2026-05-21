# Glossary of Terms

A place to define all specific terms and vocabulary for the Brave Ads component, please use this glossary to ensure consistency throughout the codebase. If other stakeholders use different terminology, please consider updating so we all speak the same language.

| term  | description  |
|---|---|
| Ad unit  | An ad unit is displayed to the user. Also known as an ad placement.  |
| Ad type  | The classification of an ad unit, such as new tab page ad, notification ad, or search result ad.  |
| Advertiser  | An advertiser is someone or a company that promotes products, services, or ideas through various mediums to reach and persuade potential customers or target audiences.  |
| Advertisement  | An advertisement is a promotional message or content created to promote a product, service, or idea to a target audience, also known as an Ad.  |
| Analytics  | Examine data to uncover meaningful insights, trends, and patterns that can inform decision-making and improve understanding of a given subject or domain.  |
| Anti-targeting  | Exclude users from receiving targeted advertisements based on specific attributes such as demographics, interests, or behaviors.  |
| Allocation  | The distribution of available impression budget across eligible campaigns and creatives to ensure fair and optimal delivery.  |
| Basic Attention Token  | See the [whitepaper](https://basicattentiontoken.org/static-assets/documents/BasicAttentionTokenWhitePaper-4.pdf). Also known as BAT.  |
| Behavioral targeting  | Selects ads based on a privacy-preserving model of a user's browsing behavior and interests, built and evaluated entirely on the user's device.  |
| Behavioral  | Behavioral advertising targets ads based on the user's previous online behavior, such as browsing history and interactions, to deliver relevant and personalized advertisements.  |
| Blinded token  | A token that has been cryptographically blinded by the client before submission to the issuer, preventing the issuer from linking the signed token to the requesting user.  |
| Brave Rewards user  | Refers to an individual who has joined Brave Rewards and will be rewarded BAT for viewing ads.  |
| Campaign  | A campaign is a coordinated series of marketing activities and messages designed to achieve specific goals within a defined timeframe.  |
| Catalog  | A collection of available campaigns, creative sets and creative instances.  |
| Click  | Refers to a user interacting with an advertisement by clicking on the ad.  |
| Click-through rate  | The percentage of ad impressions that result in clicks.  |
| Condition matcher  | One or more pref path and condition pairs, evaluated with AND logic on the user's device, that must all match before a new tab page ad can be served. Paths may reference stored prefs or virtual prefs computed at runtime, and conditions may use epoch, numerical, regex, or pattern operators.  |
| Confirmations  | Confirm events, i.e., views, without revealing to Brave the particular user involved. See [security and privacy model for ad confirmations](https://github.com/brave/brave-browser/wiki/Security-and-privacy-model-for-ad-confirmations).  |
| Contextual  | Contextual advertising targets ads based on the web page's content or the user's online activity context to deliver relevant and personalized advertisements.  |
| Conversion  | When a user triggers an action, it is counted as a conversion. Conversions include making a purchase or signing up for a newsletter.  |
| Creative instance  | A creative instance refers to a specific version or variation of an advertisement allowing advertisers to test different approaches, optimize performance, and effectively engage their target audience.  |
| Creative set  | A creative set refers to a collection of related creative instances or variations of an advertisement.  |
| Creatives  | Creatives contain the visual and textual elements developed to convey a message or promote a product or service in advertising aimed at capturing attention and resonating with the target audience to achieve marketing goals.  |
| Confirmation token  | A blinded token issued by Brave and held by the client, redeemed anonymously to confirm an ad event such as a view or click without revealing the user's identity.  |
| Day parting  | Day parting is a strategy that involves scheduling and targeting ads based on specific times of the day to maximize their effectiveness.  |
| Diagnostic  | A report of the current state and health of the Brave Ads system, surfaced in `brave://ads-internals` for debugging purposes.  |
| Deposit  | Refers to redeeming a confirmation token in exchange for BAT.  |
| Eligible ads  | Ads that pass all eligibility criteria, including exclusion rules, frequency caps, and targeting requirements, and qualify to be served at a given moment.  |
| Exclusion rules  | A set of rules that disqualify individual ads from being served, such as recency, frequency, conversion, and anti-targeting checks. Also known as frequency capping.  |
| Frequency capping  | Limits how many times a given ad will be shown within a specified period, also known as exclusion rules.  |
| Geographic  | Geographic advertising targets ads based on specific geographic locations to reach the desired audience.  |
| Impression  | An impression is when an ad is served or viewed by a user.  |
| Machine learning  | Statistical models and algorithms used within Brave Ads for on-device text classification and behavioral targeting. Also known as ML.  |
| Issuers  | Refers to public keys used to sign privacy-preserving Blinded Tokens. See [challenge-bypass-ristretto](https://github.com/brave-intl/challenge-bypass-ristretto) and [privacy pass cryptographic protocol](https://www.petsymposium.org/2018/files/papers/issue3/popets-2018-0026.pdf).  |
| New tab page ad  | An ad unit displayed as a branded background image on a user's new tab page. Also known as a sponsored image.  |
| Non-Brave Rewards user  | Refers to a user who has not joined Brave Rewards and will not be rewarded BAT for viewing ads.  |
| Notification ad  | An ad unit delivered as a native browser notification, shown at opportune moments based on user attention.  |
| Pacing  | Rate at which an ad campaign uses up its pre-set number of impressions.  |
| Page land  | A user's arrival on the advertiser's website, i.e., the pages after the user clicks on an ad, also known as a site visit.  |
| Placement  | The location at which an ad is displayed to the user. Also known as an ad unit.  |
| Priority  | Priority at which an ad campaign uses up its impressions.  |
| Privacy-preserving tokens  | See [challenge-bypass-ristretto](https://github.com/brave-intl/challenge-bypass-ristretto) and [privacy pass cryptographic protocol](https://www.petsymposium.org/2018/files/papers/issue3/popets-2018-0026.pdf).  |
| Purchase intent  | Likelihood or inclination of a consumer to make a purchase based on their expressed interest or behavior. It indicates the consumer's readiness to buy a product or service.  |
| Purchase intent targeting  | Serves relevant ads when a user's browsing and search patterns suggest readiness to make a purchase of a specific product or service.  |
| Reactions  | Allow a user to give feedback on a category, advertiser, or ad influencing which ads the user can see.  |
| Resources  | External resources provided by [components](brave://components).  |
| Round robin  | An impression delivery strategy that ensures every ad is served at least once per rotation before any repeats, with the actual selection within the remaining eligible ads remaining random.  |
| Search result ad  | An ad unit displayed within search results and delivered by the search provider, where ad selection is handled server-side.  |
| Segment  | An advertising taxonomy to target ads to reach the desired audience.  |
| Serving  | Deliver advertisements to users, ensuring that suitable ads are shown to the right audience at the right time, also known as delivery.  |
| Statement of accounts  | A statement of accounts outlines the transactions during a specific period, giving the user an overview of earned rewards.  |
| Targeting  | Delivering an ad to the appropriate audience through behavioral, contextual, or geographic targeting.  |
| Text classification  | Target audiences based on the analysis and understanding of textual data to deliver more relevant and personalized advertisements.  |
| Token  | A cryptographic credential, either blinded or unblinded, used to confirm ad events in a privacy-preserving manner. See also blinded token and confirmation token.  |
| Transactions  | Transactions occur when redeeming a confirmation token and are recorded in a ledger on local storage, providing a chronological and organized history of events.  |
| User attention  | Measures user attention to choose the most opportune moment to serve an ad and drive performance.  |
| Unblinded token  | A token after cryptographic unblinding by the client, used to create anonymous confirmations without revealing the user's identity to Brave servers.  |
| User engagement  | Measures user engagement with an advertisement, such as a user viewing, clicking or converting an ad.  |
| User model  | A privacy-preserving model built locally from a user's browsing behavior, used to match relevant ads on-device without exposing personal data to Brave servers.  |
| Wallet  | Holds a Brave Rewards payment identifier and recovery seed to enable the refill of confirmation tokens and disbursement of earned rewards to the user.  |

Please add to it!
