// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// PruneJS
(function() {
  const prunePaths = ['playerResponse.adPlacements', 'playerResponse.playerAds', 'adPlacements', 'playerAds'];
  const findOwner = function(root, path) {
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
      for (const path of prunePaths) {
        const details = findOwner(r, path);
        if (details !== undefined) {
          delete details[0][details[1]];
        }
      }
      return r;
    },
  });
})();

/// SetJS
(function() {
  const setJS = function(chain, cValue) {
    const thisScript = document.currentScript;
    if (cValue === 'undefined') {
      cValue = undefined;
    } else if (cValue === 'false') {
      cValue = false;
    } else if (cValue === 'true') {
      cValue = true;
    } else if (cValue === 'null') {
      cValue = null;
    } else if (cValue === 'noopFunc') {
      cValue = function(){};
    } else if (cValue === 'trueFunc') {
      cValue = function(){ return true; };
    } else if (cValue === 'falseFunc') {
      cValue = function(){ return false; };
    } else if (/^\d+$/.test(cValue)) {
      cValue = parseFloat(cValue);
      if (isNaN(cValue)) { return; }
      if (Math.abs(cValue) > 0x7FFF) { return; }
    } else if (cValue === "''") {
      cValue = '';
    } else {
      return;
    }
      
    let aborted = false;
    const mustAbort = function(v) {
      if (aborted) { return true; }
      aborted =
        (v !== undefined && v !== null) &&
        (cValue !== undefined && cValue !== null) &&
        (typeof v !== typeof cValue);
      return aborted;
    };
      
    // https://github.com/uBlockOrigin/uBlock-issues/issues/156
    //   Support multiple trappers for the same property.
    const trapProp = function(owner, prop, handler) {
      if (handler.init(owner[prop]) === false) { return; }
      const odesc = Object.getOwnPropertyDescriptor(owner, prop);
      let prevGetter, prevSetter;
      if (odesc instanceof Object) {
        if (odesc.get instanceof Function) {
          prevGetter = odesc.get;
        }
        if (odesc.set instanceof Function) {
          prevSetter = odesc.set;
        }
      }
      
      Object.defineProperty(owner, prop, {
        configurable: true,
        get() {
          if (prevGetter !== undefined) {
            prevGetter();
          }
          return handler.getter();
        },
        set(a) {
          if (prevSetter !== undefined) {
            prevSetter(a);
          }
          handler.setter(a);
        }
      });
    };
      
      const trapChain = function(owner, chain) {
        const pos = chain.indexOf('.');
        if (pos === -1) {
          trapProp(owner, chain, {
            v: undefined,
            init: function(v) {
              if (mustAbort(v)) { return false; }
              this.v = v;
              return true;
            },
            getter: function() {
              return document.currentScript === thisScript
                  ? this.v
                  : cValue;
            },
            setter: function(a) {
              if (mustAbort(a) === false) { return; }
              cValue = a;
            }
          });
          return;
        }
        
        const prop = chain.slice(0, pos);
        const v = owner[prop];
        chain = chain.slice(pos + 1);
        if (v instanceof Object || typeof v === 'object' && v !== null) {
          trapChain(v, chain);
          return;
        }
        
        trapProp(owner, prop, {
          v: undefined,
          init: function(v) {
            this.v = v;
            return true;
          },
          getter: function() {
            return this.v;
          },
          setter: function(a) {
            this.v = a;
            if (a instanceof Object) {
              trapChain(a, chain);
            }
          }
        });
      };
      
      trapChain(window, chain);
  };
  
  setJS('ytInitialPlayerResponse.adPlacements', 'undefined');
  setJS('playerResponse.adPlacements', 'undefined');
})();
