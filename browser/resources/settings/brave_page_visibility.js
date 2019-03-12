/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

cr.define('settings', function() {
  // use value defined in page_visibility.js in guest mode
  if (loadTimeData.getBoolean('isGuest')) return;

  // We need to specify values for every attribute in pageVisibility instead of
  // only overriding specific attributes here because chromium does not
  // explicitly define pageVisibility in page_visibility.js since polymer only
  // notifies after a property is set.
  // Use proxy objects here so we only need to write out the attributes we
  // would like to hide.

  const appearanceHandler = {
    get: function(obj, prop) {
      return true;
    }
  };

  const socialBlockingHandler = {
    get: function(obj, prop) {
      return true;
    }
  };

  const privacyHandler = {
    get: function(obj, prop) {
      return true;
    }
  }

  const defaultHandler = {
    get: function(obj, prop) {
      return true
    }
  }

  const defaultSections = [
    'extensions',
    'getStarted',
    'shields',
  ]

  const hiddenSections = [
    'a11y',
    'people',
    'defaultBrowser'
  ]

  const handler = {
    get: function(obj, prop) {
      if (prop === 'appearance') return new Proxy({}, appearanceHandler);
      if (prop === 'socialBlocking') return new Proxy({}, socialBlockingHandler);
      if (prop === 'braveSync') {
        if (loadTimeData.getBoolean('isSyncDisabled'))
          return false;
        return true;
      }
      if (prop === 'privacy') return new Proxy({}, privacyHandler)
      if (defaultSections.includes(prop)) return new Proxy({}, defaultHandler)
      return hiddenSections.includes(prop) ? false : true;
    }
  };

  let proxy = new Proxy({}, handler);

  return { pageVisibility: proxy };
});
