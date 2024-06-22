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
#include <botan/base64.h>

void printStr(const Botan::PKCS11::secure_string& str) {
   
    for (uint8_t value : str) {
        std::cout << value;
    }
    std::cout << std::endl;
}

void botanmylib::myclass::init() {
    //
    std::cout<<"init called but nothing to be done";
}

std::string botanmylib::myclass::calculate12(char* documentPath) 
{
   Botan::PKCS11::Module module("/Users/Shubham.Kumar/projects/chromium/src/brave/third_party/botan/libs/libcastle_v2.1.0.0.dylib");
   // open write session to first slot with connected token

   std::vector<Botan::PKCS11::SlotId> slots = Botan::PKCS11::Slot::get_available_slots(module, true);
   std::cout<<"running try catch now\n";
    slots = Botan::PKCS11::Slot::get_available_slots(module, false);
    try {
        std::cout<<"number of slots " << slots.size()<<std::endl;
        if(slots.empty()) {
            return "No slots found\n";
        }
        Botan::PKCS11::Slot slot(module, slots.at(0));
        std::cout<<" ----- "<<"slot desc "<<slot.get_slot_info().slotDescription;
        
        Botan::PKCS11::Session session(slot, false);

        Botan::PKCS11::secure_string pin = {'1', '2', '3', '4', '5', '6', '7', '8'};
        session.login(Botan::PKCS11::UserType::User, pin);

        std::cout<<" ----- "<<"successfully logged in"<<std::endl;

        Botan::PKCS11::AttributeContainer search_template;
        search_template.add_class(Botan::PKCS11::ObjectClass::PrivateKey);
        auto found_objs = Botan::PKCS11::Object::search<Botan::PKCS11::Object>(session, search_template.attributes());
        
        std::cout <<" ----- "<< "Found " << found_objs.size() << " objects" << std::endl;

        if(found_objs.empty()) return "None objs found\n";

        for(auto& obj : found_objs) {
            std::cout << "Objects: ";
            printStr(obj.get_attribute_value(Botan::PKCS11::AttributeType::Label));
        }

        Botan::PKCS11::ObjectHandle priv_key_handle = found_objs.at(0).handle();

        // Load the private key from the HSM
        Botan::PKCS11::PKCS11_RSA_PrivateKey priv_key(session, priv_key_handle);
        // Read the document to sign
        std::ifstream file(documentPath, std::ios::binary);
        Botan::PKCS11::secure_string document_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        std::string mechanism = "EMSA3(SHA-256)";

        // Sign the document
        Botan::AutoSeeded_RNG rng;
        Botan::PK_Signer signer(priv_key, rng, mechanism, Botan::Signature_Format::Standard);   
        auto signature = signer.sign_message(document_data, rng);

        std::cout <<" ----- "<< "Document signed" << std::endl;

        Botan::PK_Verifier verifier(priv_key, mechanism, Botan::Signature_Format::Standard);
        bool verification_result = verifier.verify_message(document_data, signature);

        std::string base64_signature = "";

        if (verification_result) {
            base64_signature = Botan::base64_encode(signature.data(), signature.size());
            session.logoff();
            std::cout<<base64_signature<<std::endl;
            return  base64_signature; 
        } else {
            session.logoff();
            return "Verification Failed";
        }
    } catch(const std::exception& e) {
        std::cout<<" ----- "<<"PKCS error :::: "<<e.what()<<std::endl;
        return "Signature verification failed!! some exception is thrown";    
    }
}