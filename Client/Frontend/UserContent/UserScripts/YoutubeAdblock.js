// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

(function() {
    const $<prunePaths> = ['playerResponse.adPlacements', 'playerResponse.playerAds', 'adPlacements', 'playerAds'];
    const $<findOwner> = function(root, path) {
        let owner = root;
        let chain = path;
        while(true) {
            if (owner instanceof Object === false) { return; }
            const pos = chain.indexOf('.');
            if (pos === -1) {
                return owner.hasOwnProperty(chain)? [owner, chain]: undefined;
            }
            const prop = chain.slice(0, pos);
            if (owner.hasOwnProperty(prop) === false) { return; }
            owner = owner[prop];
            chain = chain.slice(pos + 1);
        }
    };
    
    JSON.parse = new Proxy(JSON.parse, {
        apply: function() {
            const r = Reflect.apply(...arguments);
            for (const path of $<prunePaths>) {
                const details = $<findOwner>(r, path);
                if (details !== undefined) {
                    delete details[0][details[1]];
                }
            }
            return r;
        },
    });
})();
