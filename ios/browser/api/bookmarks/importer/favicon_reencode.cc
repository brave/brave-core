#include "brave/ios/browser/api/bookmarks/importer/favicon_reencode.h"

namespace importer {

bool ReencodeFavicon(const unsigned char* src_data,
                     size_t src_len,
                     std::vector<unsigned char>* png_data) {
    
    if (!png_data) {
        return false;
    }
    
    // Brandon T. - Needs a better way to decode `src_data` into a PNG into `png_data` for iOS.
    //UIImage *image = [UIImage imageWithData: [NSData dataWithBytes:src_data length:src_length]];
    //NSData *pngData = UIImagePNGRepresentation(image);
    //const unsigned char *bytes = static_cast<const unsigned char*>([pngData bytes]);
    //png_data->insert(png_data->begin(), bytes, bytes + [pngData count]);
    
    //Return false for now.. as not sure if we need favIcons encoding anyway..
    return false;
}

}  // namespace importer
