/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_CHROME_COMPONENT_UPDATER_CONFIGURATOR_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_CHROME_COMPONENT_UPDATER_CONFIGURATOR_H_

#include "base/memory/ref_counted.h"
#include "components/update_client/configurator.h"

class PrefRegistrySimple;
class PrefService;

namespace base {
class CommandLine;
}

namespace net {
class URLRequestContextGetter;
}

namespace component_updater {

// Registers preferences associated with the component updater configurator
// for Chrome. The preferences must be registered with the local pref store
// before they can be queried by the configurator instance.
// This function is called before MakeChromeComponentUpdaterConfigurator.
void RegisterPrefsForBraveComponentUpdaterConfigurator(
    PrefRegistrySimple* registry);

scoped_refptr<update_client::Configurator>
MakeBraveComponentUpdaterConfigurator(
    const base::CommandLine* cmdline,
    net::URLRequestContextGetter* context_getter,
    PrefService* pref_service);

}  // namespace component_updater

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_CHROME_COMPONENT_UPDATER_CONFIGURATOR_H_
