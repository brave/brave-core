/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <fstream>
#include "filter_list.cc"
#include "lists/regions.h"

// Gets regions from regions.h file and parses it to a format supported by iOS app.
int main(int argc, const char * argv[]) {
    std::string regionListTxt;

    for (int i = 0; i < region_lists.size(); i++) {
        std::string regionLine;

        FilterList region = region_lists[i];

        if(region.langs.size() == 0) {
            continue;
        }

        for(int j = 0; j < region.langs.size(); j++) {
            std::string language = region.langs[j];
            regionLine.append(language);
            regionLine.append(",");
        }

        regionLine.append(region.uuid);
        regionListTxt.append(regionLine);
        regionListTxt.append("\n");
    }

    std::ofstream file("adblock-regions.txt");
    file << regionListTxt;

    return 0;
}
