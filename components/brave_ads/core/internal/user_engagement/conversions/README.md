# Conversions

When launching campaigns, advertisers select specific actions. When a user triggers an action, it is counted as a conversion. Conversions include making a purchase or signing up for a newsletter.

| type  | explanation  |
|---|---|
| view-through  | View-through conversions occur when a user is exposed to an ad viewed impression and later completes a desired action, such as making a purchase or filling out a form  |
| click-through  | Click-through conversions occur when a user clicks on an ad and later completes a desired action, such as making a purchase or filling out a form  |

## Architecture

This diagram shows how a page visit becomes an attributed conversion, including reading the order ID and order value from the checkout or thank you page when a campaign's own conversion settings ask for it.

`CampaignConversionInfo`, `AdEventInfo`, the confirmation queue, and the
last successfully validated conversion patterns are persisted and survive
a browser restart. The `TabInfo` order ID/value cache below is in-memory
only and does not survive a browser restart or tab close, but that is not data
loss, since revisiting a matching page before converting re-extracts it,
including when a restored tab reopens the same page on browser launch.

A conversion whose campaign requests order extraction is held back in an
in-memory, unpersisted cache until that tab's order ID/value resolves. If
the tab closes first, the held-back conversion is dropped rather than
reported with incomplete order data it can no longer refresh. Navigating
away instead reports it with whatever order data resolved by then, since
extraction for the page it was waiting on can never resolve afterward.

A solid arrow is a direct dependency or control flow step. A dashed arrow
is a secondary, asynchronous, or optional relationship, such as config
delivery or a cache write that is not itself the main flow. A double-headed
arrow is a synchronous round trip, such as a mojo call and its reply.

```mermaid
%%{init: {"flowchart": {"nodeSpacing": 60, "rankSpacing": 90, "curve": "basis"}}}%%
flowchart TB
  User(["User visits a page, or focuses a credit card field"])
  Component(["Sponsored content component updates"])

  subgraph Renderer["content/renderer/"]
    Extractor["OrderExtractor"]
  end

  subgraph ContentBrowser["content/browser/"]
    Fetcher["OrderFetcher"]
  end

  subgraph Browser["browser/brave_ads/"]
    TabHelper["AdsTabHelper"]
    CreditCardObserver["CreditCardFieldFocusObserver"]
  end

  subgraph ConversionsCore["core/internal/user_engagement/conversions/"]
    Conversions["Conversions"]
    UrlPatternMatch{"Does the page's URL match a campaign's thank you URL pattern?"}
    Matched{"Matched ad event within the observation window?"}
    RecordAdEvent["Record the conversion ad event"]
    BuildConversion["Build the ConversionInfo, attaching the cached order ID/value"]
    NoMatch(["Not a conversion"])
    Pending{"Order extraction still pending for this tab?"}
    DeferredCache["Deferred conversions (in-memory, not persisted)"]
    DeferredOutcome{"Order extraction resolves, or tab closes first?"}
    Dropped(["Tab closed before order extraction resolved, conversion dropped"])
  end

  subgraph OrderExtractionGroup["core/internal/creatives/conversions/order/"]
    Schema["Conversion patterns, defaulting until a pref or sponsored content value loads"]
    PatternsPref[("Last successfully validated conversion patterns, persisted")]
    CampaignMatch{"Matches a campaign wanting order extraction, with a pending ad click or view?"}
    NoExtract(["No matching campaign, nothing extracted"])
    ExtractIdGate{"Is order ID extraction enabled for this campaign?"}
    NoId(["This campaign does not need the order ID"])
    ExtractValueGate{"Is order value extraction enabled for this campaign?"}
    NoValue(["This campaign does not need the order value"])
  end

  subgraph CampaignConversionsGroup["core/internal/creatives/conversions/"]
    CampaignConversions[("Campaign conversion settings (persisted)")]
  end

  subgraph AdEventsGroup["core/internal/user_engagement/ad_events/"]
    AdEvents[("Ad events (persisted)")]
  end

  subgraph Tabs["core/internal/tabs/"]
    TabManagerNode["TabManager"]
    TabInfoCache["TabInfo order ID/value cache (in-memory, not persisted)"]
  end

  subgraph AdUnits["core/internal/ad_units/"]
    AdHandlerNode["AdHandler"]
  end

  subgraph AccountGroup["core/internal/account/"]
    Account["Account"]
    Queued(["Queued for confirmation, handled the same as any other ad event"])
  end

  Component -.-> Schema
  Schema -.-> PatternsPref
  PatternsPref -.-> Schema
  User --> TabHelper
  User -.-> CreditCardObserver
  CreditCardObserver --> TabHelper

  %% Order extraction only runs on a page matching a campaign's own
  %% checkout or thank you URL pattern; every other page stops at the gate.
  TabHelper --> Conversions
  Conversions --> CampaignConversions
  CampaignConversions --> AdEvents
  AdEvents --> CampaignMatch
  CampaignMatch -->|No| NoExtract
  CampaignMatch -->|Yes| ExtractIdGate
  CampaignMatch -->|Yes| ExtractValueGate
  ExtractIdGate -->|No| NoId
  ExtractIdGate -->|Yes| Fetcher
  ExtractValueGate -->|No| NoValue
  ExtractValueGate -->|Yes| Fetcher
  Schema -.-> Fetcher
  Fetcher <--> Extractor
  Fetcher --> TabHelper
  TabHelper --> TabManagerNode
  TabManagerNode -.-> TabInfoCache

  %% Conversion recording, on the tab's own navigation notification,
  %% independent of the extraction gate above.
  TabManagerNode --> Conversions
  CampaignConversions --> UrlPatternMatch
  UrlPatternMatch -->|No| NoMatch
  UrlPatternMatch -->|Yes| Matched
  AdEvents --> Matched
  Matched -->|No| NoMatch
  Matched -->|Yes| Pending
  Pending -->|No| RecordAdEvent
  Pending -->|Yes| DeferredCache
  RecordAdEvent --> BuildConversion
  TabInfoCache --> BuildConversion
  BuildConversion --> AdHandlerNode
  DeferredCache --> DeferredOutcome
  TabManagerNode -.-> DeferredOutcome
  DeferredOutcome -->|Resolves| RecordAdEvent
  DeferredOutcome -->|Tab closes| Dropped
  AdHandlerNode --> Account
  Account --> Queued
```
