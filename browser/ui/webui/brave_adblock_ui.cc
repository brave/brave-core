/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_adblock_ui.h"

#include "brave/common/webui_url_constants.h"
#include "components/grit/brave_components_resources.h"

BraveAdblockUI::BraveAdblockUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kAdblockJS,
        IDR_BRAVE_ADBLOCK_JS, IDR_BRAVE_ADBLOCK_HTML) {
}

BraveAdblockUI::~BraveAdblockUI() {
}
