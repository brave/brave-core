#ifndef MYCLASS_H
#define MYCLASS_H

namespace botan_high_level {

    class pkcs11 {
        public:
            static std::string sign_data(char* module_path, char* pin, char* md_hash);
    };


} // namespace extensions

#endif // MYCLASS_H
