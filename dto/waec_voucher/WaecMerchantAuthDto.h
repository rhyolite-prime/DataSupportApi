//
// Created by Antigravity on 21/07/2026.
//

#ifndef DATASUPPORTAPI_WAECMERCHANTAUTHDTO_H
#define DATASUPPORTAPI_WAECMERCHANTAUTHDTO_H

#include <string>

namespace data_support::dto {

    class WaecMerchantAuthDto {
    public:
        std::string merchantIdentifier; // Email or Phone
        std::string pin;

        WaecMerchantAuthDto() = default;
        virtual ~WaecMerchantAuthDto() = default;
    };

}
#endif //DATASUPPORTAPI_WAECMERCHANTAUTHDTO_H
