# Favicon Documentation

This file documents the list of changes made to the Chromium iOS Favicon implementation as it isn't compatible with Brave-iOS.


# Changes

[BraveIOSWebFaviconDriver.h](https://github.com/brave/brave-core/blob/master/ios/browser/favicon/brave_ios_web_favicon_driver.h): Modification of `WebFaviconDriver` to get rid of `web::WebState` and instead use `ProfileIOS` as iOS cannot use `web::WebState` since we do not use `CRWWebView` in our Swift code. This means we got rid of all the `WebStateObserver` and `UserData` as well, and instead store the class with the `ProfileIOS`. This is done via the `CreateForBrowserState` and `FromBrowserState` functions.

[BraveIOSWebFaviconDriver.mm](https://github.com/brave/brave-core/blob/master/ios/browser/favicon/brave_ios_web_favicon_driver.mm)

The below code was added in order to setup a navigation stack for the Swift iOS WebView when navigation has just begun.
```c++
void BraveIOSWebFaviconDriver::DidStartNavigation(
                                         ProfileIOS* profile,
                                         const GURL& page_url) {
    items.clear();
    auto item = std::make_unique<web::NavigationItemImpl>();
    item->SetOriginalRequestURL(page_url);
    item->SetURL(page_url);
    items.push_back(std::move(item));
}
```

When navigation is complete, we call the below function to begin fetching the `Favicon` for the `URL` that was navigated to:
```c++
void BraveIOSWebFaviconDriver::DidFinishNavigation(
                                         ProfileIOS* profile,
                                         const GURL& page_url) {
    web::NavigationItemImpl* item = !items.empty() ? items.back().get() : nullptr;
    DCHECK(item);
    
    // Fetch the fav-icon
    FetchFavicon(item->GetURL(), /*IsSameDocument*/ **false**);
}
```

This all emulates the `web::WebState::DidStartNavigation` and `web::WebState::DidFinishNavigation` functions in a much simpler way, that would be compatible with Brave-iOS until the day we switch over to using Chromium's `CRWWebView`.