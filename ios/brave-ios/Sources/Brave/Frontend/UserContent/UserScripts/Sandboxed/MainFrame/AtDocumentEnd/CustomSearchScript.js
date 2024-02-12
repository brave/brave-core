/* vim: set ts=2 sts=2 sw=2 et tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

Object.defineProperty(window.__firefox__, "searchQueryForField", {
  enumerable: false,
  configurable: false,
  writable: false,
  value: function() {
    var input = document.activeElement;
    if (input.tagName.toLowerCase() !== "input") return null;
    var form = input.form;
    if (!form || form.method.toLowerCase() != "get") return null;

    var inputs = form.getElementsByTagName("input");
    inputs = Array.prototype.slice.call(inputs, 0);
    var params = inputs.map(function(element) {
      if (element.name == input.name) return [element.name, "{searchTerms}"].join("=");
      return [element.name, element.value].map(encodeURIComponent).join("=");
    });

    var selectFields = form.getElementsByTagName("select");
    selectFields = Array.prototype.slice.call(selectFields, 0);
    var selectParams = selectFields.map(function(e){
      return [e.name, e.options[e.selectedIndex].value].map(encodeURIComponent).join("=");
    });
    params = params.concat(selectParams);
    if (!form.action) return null; //an invalid form.
    var url = [form.action, params.join("&")].join("?");
    return url;
  }
});
