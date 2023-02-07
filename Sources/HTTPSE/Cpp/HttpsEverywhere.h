/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "leveldb/db.h"
#include "RecentlyUsedCache.h"
#include <string>

class HTTPSEverywhere
{
private:
    leveldb::DB* db = nullptr;
    RecentlyUsedCache<std::string> recentlyUsedCache;
    std::string applyRedirectRule(std::string originalUrl, const std::string &rule);
    // re2 library doesn't use '$' sign for arguments but '\' sign, we need to correct https rules for that.
    std::string correcttoRuleToRE2Engine(const std::string& to);
    
public:
    ~HTTPSEverywhere() { close(); }
    bool initHTTPSE(const std::string &pathToDb);
    bool isLoaded();
    void close();
    // Returns https url on success, empty string if no redirect
    std::string getHTTPSURL(const std::string &urlHost, const std::string &urlPath);
};

