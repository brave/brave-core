#ifndef browser_state_otr_helper_h
#define browser_state_otr_helper_h

namespace web {
class BrowserState;
}

namespace brave {
// Returns the original BrowserState even for incognito states.
web::BrowserState* GetBrowserStateRedirectedInIncognito(
    web::BrowserState* browser_state);

// Returns non-null BrowserState even for Incognito contexts so that a
// separate instance of a service is created for the Incognito context.
web::BrowserState* GetBrowserStateOwnInstanceInIncognito(
    web::BrowserState* browser_state);
}

#endif //browser_state_otr_helper_h
