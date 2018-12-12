/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "HttpsEverywhere.h"

#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <iostream>
#include "JsonCpp.h"
#include "re2/re2.h"

using json = nlohmann::json;

static bool startsWith(const std::string& s1, const std::string& prefix) {
    return prefix.size() <= s1.size() && s1.compare(0, prefix.size(), prefix) == 0;
}

bool HTTPSEverywhere::initHTTPSE(const std::string &pathToDb) {
    leveldb::Options options;
    leveldb::Status status = leveldb::DB::Open(options, pathToDb, &db);
    return status.ok();
}

bool HTTPSEverywhere::isLoaded() {
    return db != nullptr;
}

void HTTPSEverywhere::close() {
    delete db;
    db = nullptr;
    recentlyUsedCache.clear();
}

// Returns https url on success, empty string if no redirect
std::string HTTPSEverywhere::applyRedirectRule(std::string originalUrl, const std::string &rule) {
    auto json_object = json::parse(rule);
    if (json_object.is_null()) {
        return "";
    }

    if (!startsWith(originalUrl, "http://")) {
        originalUrl = "http://" + originalUrl;
    }

    for (auto ruleItem : json_object) {
        auto exclusions = ruleItem["e"];
        if (!exclusions.is_null() && exclusions.is_array()) {
            std::vector<json> v = exclusions;
            for (auto item : v) {
                if (item.empty()) { continue; }

                std::string pattern = item["p"];
                if (std::regex_match(originalUrl, std::regex(pattern))) {
                    //printf("exclusion %s\n", originalUrl.c_str());
                    return "";
                }
            }
        }

        auto upgradeRules = ruleItem["r"];
        if (upgradeRules.is_null() || !upgradeRules.is_array()) {
            return "";
        }

        std::vector<json> v = upgradeRules;
        for (auto item : v) {
            auto defaultUpgrade = item["d"];
            if (!defaultUpgrade.is_null()) {
                return originalUrl.insert(4, "s");
            }
            
            auto from = item["f"];
            auto to = item["t"];
            if (!from.is_null() && !to.is_null()) {
                std::string f = from;
                std::string t = to;
                
                t = correcttoRuleToRE2Engine(t);
                
                RE2 regExp(f);
                std::string newUrl(originalUrl);
                if (RE2::Replace(&newUrl, regExp, t) && newUrl != originalUrl) {
                    return newUrl;
                }
            }
        }
    }

    return "";
}

std::string HTTPSEverywhere::correcttoRuleToRE2Engine(const std::string& to) {
    std::string correctedto(to);
    size_t pos = to.find("$");
    while (std::string::npos != pos) {
        correctedto[pos] = '\\';
        pos = correctedto.find("$");
    }
    
    return correctedto;
}

static std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> result;
    while (getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}


// returns parts in reverse order, makes list of lookup domains like com.foo.*
static std::vector<std::string> expandDomainForLookup(const std::string &domain)
{
    std::vector<std::string> resultDomains;
    std::vector<std::string> domainParts = split(domain, '.');
    if (domainParts.empty()) {
        return resultDomains;
    }

    for (int i = 0; i < domainParts.size() - 1; i++) {  // i < size()-1 is correct: don't want 'com.*' added to resultDomains
        std::string slice = "";
        std::string dot = "";
        for (int j = (unsigned int)domainParts.size() - 1; j >= i; j--) {
            slice += dot + domainParts[j];
            dot = ".";
        }
        if (0 != i) {
            // We don't want * on the top URL
            resultDomains.push_back(slice + ".*");
        } else {
            resultDomains.push_back(slice);
        }
    }
    return resultDomains;
}

static std::string dbGet(leveldb::DB* db, const std::string &key)
{
    if (!db) {
        return "";
    }

    std::string value;
    leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);
    return s.ok() ? value : "";
}


// Returns https url on success, empty string if no redirect
std::string HTTPSEverywhere::getHTTPSURL(const std::string &urlHost, const std::string &urlPath) {
    if (nullptr == db) {
        return "";
    }

    std::string fullURL = urlHost + urlPath;
    if (recentlyUsedCache.data.count(fullURL) > 0) {
        return recentlyUsedCache.data[fullURL];
    }

    const std::vector<std::string> domains = expandDomainForLookup(urlHost);
    for (auto domain : domains) {
        std::string value = dbGet(db, domain);
        if (!value.empty()) {
            std::string newURL = applyRedirectRule(fullURL, value);
            recentlyUsedCache.data[fullURL] = newURL;
            return newURL;
        }
    }

    recentlyUsedCache.data[fullURL] = "";
    return "";
}
