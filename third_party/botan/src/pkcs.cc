#include <fstream>
#include <vector>
#include <iostream>
#include <cstddef>
#include <string>
#include <fstream>

#include "pkcs.h"

#include <botan/auto_rng.h>
#include <botan/p11.h>
#include <botan/p11_rsa.h>
#include <botan/p11_object.h>
#include <botan/p11_types.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>
#include <botan/secmem.h>

std::vector<uint8_t> hexStringToBytes(char* charArray)
{
    std::string hex(charArray);

    std::vector<uint8_t> bytes;

    for (size_t i = 0; i < hex.length(); i += 2) {
        uint8_t byte = std::stoi(hex.substr(i, 2), nullptr, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

std::string vectorToHex(const std::vector<uint8_t>& buffer) {
    std::ostringstream oss;
    for (const auto& byte : buffer) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

std::string botan_high_level::pkcs11::sign_data(char* module_path, char* pin, char* md_hash) {
    try {
        Botan::PKCS11::Module module(module_path);

        std::vector<Botan::PKCS11::SlotId> slots;
        try {
            slots = Botan::PKCS11::Slot::get_available_slots(module, true);
            std::cout << "Number of slots: " << slots.size() << std::endl;

            if (slots.empty()) {
                return "ERROR_SLOT_NOT_FOUND";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error getting available slots: " << e.what() << std::endl;
            return "ERROR_SLOT_NOT_FOUND";
        }

        try {
            Botan::PKCS11::Slot slot(module, slots.at(0));
            Botan::PKCS11::Session session(slot, false);

            try {
                Botan::PKCS11::secure_string secure_pin(pin, pin + std::strlen(pin));
                session.login(Botan::PKCS11::UserType::User, secure_pin);
                std::cout << "Successfully logged in" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error logging in: " << e.what() << std::endl;
                return "ERROR_LOGIN_FAILED";
            }

            Botan::PKCS11::AttributeContainer search_template;
            search_template.add_class(Botan::PKCS11::ObjectClass::PrivateKey);
            auto found_objs = Botan::PKCS11::Object::search<Botan::PKCS11::Object>(session, search_template.attributes());

            std::cout << "Found " << found_objs.size() << " objects" << std::endl;

            if (found_objs.empty()) return "ERROR_NO_OBJS_FOUND";

            // Load the private key from the HSM
            Botan::PKCS11::ObjectHandle priv_key_handle = found_objs.at(0).handle();
            Botan::PKCS11::PKCS11_RSA_PrivateKey priv_key(session, priv_key_handle);

            std::string mechanism = "EMSA3(Raw)";

            // Sign the hash
            Botan::AutoSeeded_RNG rng;
            Botan::PK_Signer signer(priv_key, rng, mechanism, Botan::Signature_Format::Standard);
            auto signature = signer.sign_message(hexStringToBytes(md_hash), rng);
            std::cout << "Document signed" << std::endl;
            session.logoff();
            return vectorToHex(signature);
        } catch (const std::exception& e) {
            return "ERROR_SIGNING_FAILURE";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error initializing module: " << e.what() << std::endl;
        return "ERROR_MODULE_NOT_FOUND";
    }
}