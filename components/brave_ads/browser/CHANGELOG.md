### Ads service public facing API changelog

---
### New API (4th November 2019)
##### Related PR

[https://github.com/brave/brave-core/pull/3805](https://github.com/brave/brave-core/pull/3805)

##### Description

Added a new function to the ads service to check if the users locale is newly
supported. Androd on-boarding should use this API call instead of the Android
implementation.

##### Added

bool AdsService::IsNewlySupportedLocale()
---

---
### Refactored API (21st February 2020)
##### Related PR

[https://github.com/brave/brave-core/pull/4685](https://github.com/brave/brave-core/pull/4685)

##### Description

Refactored public facing API for ad notifications

##### Renamed

GetAdNotificationForId to GetAdNotification
GetAds to GetCreativeAdNotifications
ConfirmAd to ConfirmAdNotification
NotificationInfo to AdNotificationInfo
OnNotificationEvent to OnAdNotificationEvent
NotificationEventType to AdNotificationEventType
BatAdsNotificationEventType to BatAdNotificationEventType
BatAdsNotification to BatAdNotification

---
### New API (3rd March 2020)
##### Related PR

[https://github.com/brave/brave-core/pull/4705](https://github.com/brave/brave-core/pull/4705)

##### Description

Added new public API calls for Publisher Ads

##### Added

void SiteSupportsPublisherAds(
    const std::string& url,
    SiteSupportsPublisherAdsCallback callback)

void GetCreativePublisherAds(
    const std::string& url,
    const std::vector<std::string>& categories,
    const std::vector<std::string>& sizes,
    const GetCreativePublisherAdsCallback callback)

void GetCreativePublisherAdsToPreCache(
    const GetCreativePublisherAdsToPreCacheCallback callback)

void FlagPublisherAdWasPreCached(
    const std::string& creative_instance_id,
    const FlagPublisherAdWasPreCachedCallback callback)

bool ShouldShowPublisherAdsOnParticipatingSites() const

void CanShowPublisherAds(
    const std::string& url,
    const OnCanShowPublisherAdsCallback callback)

void GetPublisherAds(
    const std::string& url,
    const std::vector<std::string>& sizes,
    GetPublisherAdsCallback callback)

void GetPublisherAdsToPreCache(
    GetPublisherAdsToPreCacheCallback callback)

void OnPublisherAdEvent(
    const PublisherAdInfo& info,
    const PublisherAdEventType event_type)

##### Renamed

ConfirmAdNotification to ConfirmAd
---
