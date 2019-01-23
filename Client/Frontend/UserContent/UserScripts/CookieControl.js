// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// The $<> is used to replace the var name at runtime with a random string.

//Cookie should neither be saved nor accessed (local or session) when user has blocked all cookies.
var $<cookie> = Object.getOwnPropertyDescriptor(Document.prototype, 'cookie') ||
    Object.getOwnPropertyDescriptor(HTMLDocument.prototype, 'cookie');
if ($<cookie> && $<cookie>.configurable) {
    Object.defineProperty(document, 'cookie', {
        get: function() {
            // Not returning null here as some websites don't have a check for cookie returning null, and may not behave properly
            return "";
        },
        set: function(val) {
            console.error("Access denied for 'cookie'")
            return;
        }
    });
}


//Access to localStorage should be denied when user has blocked all Cookies.
var $<local> = Object.getOwnPropertyDescriptor(window, 'localStorage');
if ($<local>) {
    Object.defineProperty(window, 'localStorage', {
        get: function() {
            console.error("Access denied for 'localStorage'")
            return null;
        },
    });
}

//Access to sessionStorage should be denied when user has blocked all Cookies.
var $<session> = Object.getOwnPropertyDescriptor(window, 'sessionStorage');
if ($<session>) {
    Object.defineProperty(window, 'sessionStorage', {
        get: function() {
            console.error("Access denied for 'sessionStorage'")
            return null;
        },
    });
}
