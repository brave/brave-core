// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

'use strict';

Object.defineProperty(window.__firefox__, '$<brave-search-helper>', {
    enumerable: false,
    configurable: true,
    writable: false,
    value: {
        id: 1,
        resolution_handlers: {},
        resolve(id, data, error) {
            if (error && window.__firefox__.$<brave-search-helper>.resolution_handlers[id].reject) {
                window.__firefox__.$<brave-search-helper>.resolution_handlers[id].reject(error);
            } else if (window.__firefox__.$<brave-search-helper>.resolution_handlers[id].resolve) {
                window.__firefox__.$<brave-search-helper>.resolution_handlers[id].resolve(data);
            } else if (window.__firefox__.$<brave-search-helper>.resolution_handlers[id].reject) {
                window.__firefox__.$<brave-search-helper>.resolution_handlers[id].reject(new Error("Invalid Data!"));
            } else {
                console.log("Invalid Promise ID: ", id);
            }
            
            delete window.__firefox__.$<brave-search-helper>.resolution_handlers[id];
        },
        sendMessage(method_id) {
            return new Promise((resolve, reject) => {
               window.__firefox__.$<brave-search-helper>.resolution_handlers[method_id] = { resolve, reject };
               webkit.messageHandlers.BraveSearchHelper.postMessage({ 'securitytoken': '$<security_token>' ,'method_id': method_id});
           });
        }
    }
});

Object.defineProperty(window, 'brave', {
enumerable: false,
configurable: true,
writable: false,
    value: {
    getCanSetDefaultSearchProvider() {
        return window.__firefox__.$<brave-search-helper>.sendMessage(1);
    },
    setIsDefaultSearchProvider() {
        return window.__firefox__.$<brave-search-helper>.sendMessage(2);
    }
}
});
