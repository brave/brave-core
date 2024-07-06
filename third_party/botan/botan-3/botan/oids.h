/*
* OID Registry
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_OIDS_H_
#define BOTAN_OIDS_H_

#include <botan/asn1_obj.h>

BOTAN_DEPRECATED_HEADER("oids.h")

namespace Botan::OIDS {

/**
* Register an OID to string mapping.
* @param oid the oid to register
* @param name the name to be associated with the oid
*/
BOTAN_DEPRECATED("Use OID::register_oid") inline void add_oid(const OID& oid, std::string_view name) {
   OID::register_oid(oid, name);
}

BOTAN_DEPRECATED("Use OID::register_oid") BOTAN_UNSTABLE_API void add_oid2str(const OID& oid, std::string_view name);

BOTAN_DEPRECATED("Use OID::register_oid") BOTAN_UNSTABLE_API void add_str2oid(const OID& oid, std::string_view name);

BOTAN_DEPRECATED("Use OID::register_oid") inline void add_oidstr(const char* oidstr, const char* name) {
   OID::register_oid(OID(oidstr), name);
}

/**
* Resolve an OID
* @param oid the OID to look up
* @return name associated with this OID, or an empty string
*/
BOTAN_DEPRECATED("Use OID::human_name_or_empty") inline std::string oid2str_or_empty(const OID& oid) {
   return oid.human_name_or_empty();
}

/**
* Find the OID to a name. The lookup will be performed in the
* general OID section of the configuration.
* @param name the name to resolve
* @return OID associated with the specified name
*/
BOTAN_DEPRECATED("Use OID::from_name") inline OID str2oid_or_empty(std::string_view name) {
   return OID::from_name(name).value_or(OID());
}

BOTAN_DEPRECATED("Use OID::human_name_or_empty") inline std::string oid2str_or_throw(const OID& oid) {
   std::string s = oid.human_name_or_empty();
   if(s.empty()) {
      throw Lookup_Error("No name associated with OID " + oid.to_string());
   }
   return s;
}

BOTAN_DEPRECATED("Use OID::human_name_or_empty") inline std::string lookup(const OID& oid) {
   return oid.human_name_or_empty();
}

BOTAN_DEPRECATED("Use OID::from_name") inline OID lookup(std::string_view name) {
   return OID::from_name(name).value_or(OID());
}

}  // namespace Botan::OIDS

#endif
