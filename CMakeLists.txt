cmake_minimum_required(VERSION 3.11)
project(DataSupportApi CXX)

include(CheckIncludeFileCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options("-include" "${CMAKE_CURRENT_SOURCE_DIR}/utils/codecvt_helper.h" "-Wno-deprecated-declarations")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fcoroutines)
endif()

add_executable(${PROJECT_NAME} main.cc
        services/magma_pay/MagmaPayService.h
        services/magma_sms/MagmaSmsService.h
        services/waec_evoucher/WaecVoucherService.h
        services/smart_suite_banking/SmartSuiteBankingService.h
        services/auxillary/AuxiliaryService.h
        services/flashx/FlashxService.h
        services/waec_evoucher/WaecVoucherService.cc
        services/business_account/BusinessAccountService.h
        services/business_account/BusinessAccountService.cc
        dto/BaseApiResponse.h
        dto/BusinessAccountDto.h
        constants/ErrorCode.h
        constants/StatusType.h
        services/business_api_keys/BusinessApiKeyService.h
        services/business_api_keys/BusinessApiKeyService.cc
        services/redis/RedisCacheManager.h
        services/redis/RedisCacheManager.cc
        utils/codecvt_helper.h
        dto/BusinessApiKeyDto.h
        dto/waec_voucher/WaecMerchantAuthDto.h
        dto/smart_suite/FlexyWithdrawalToBankDto.h
        services/quartz/QuartzApi.h
        dto/QuartzJobDto.h
        services/quartz/QuartzApi.cc
        dto/smart_suite/FlexyWithdrawalToMomoDto.h
        utils/IdGeneratorUtils.h
        utils/IdGeneratorUtils.cc
        utils/CryptoUtils.h
        utils/CryptoUtils.cc
        utils/PhoneNumberFormatter.h
        utils/PhoneNumberFormatter.cc
        utils/UrlUtils.h
        utils/UrlUtils.cc
        dto/smart_suite/ChangePinDto.h
        dto/smart_suite/MiniStatementDto.h
        dto/smart_suite/AccountBalanceDto.h
        dto/smart_suite/UssdDepositDto.h
        dto/smart_suite/WebCheckoutDepositDto.h
        constants/TransactionStatusCode.h
        dto/smart_suite/KiddyWithdrawalRequestDto.h
        dto/smart_suite/SavingsWithdrawalRequestDto.h
        dto/smart_suite/InterAccountTransferDto.h
        services/smart_suite_banking/SmartSuiteBankingService.cc
        services/hubtel_api/HubtelApi.h
        services/hubtel_api/HubtelApi.cc
        dto/smart_suite/SmartSuiteApiResponse.h
        dto/smart_suite/SmartSuiteOperationResponse.h
        services/turbo_ledger/TurboLedgerService.h
        services/turbo_ledger/TurboLedgerService.cc
        dto/turbo_ledger/CreateIndividualAccountDto.h
        services/erp_api/ErpApi.h
        services/erp_api/ErpApi.cc
        services/email/EmailApi.h
        dto/SendEmailDto.h
        services/email/EmailApi.cc
        dto/erp_api/SyncApiCredentialDto.h
        dto/erp_api/SyncPosBranchDto.h
        services/pay_stack_api/PayStackApi.h
        services/pay_stack_api/PaystackApi.cc
)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon 
# add_subdirectory(drogon) 
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
#
# and comment out the following lines
find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon bcrypt OpenSSL::Crypto)

# ##############################################################################

if (CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "c++17 or higher is required")
elseif (CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "use c++17")
else ()
    message(STATUS "use c++20")
endif ()

aux_source_directory(controllers CTL_SRC)
aux_source_directory(filters FILTER_SRC)
aux_source_directory(plugins PLUGIN_SRC)
aux_source_directory(models MODEL_SRC)

drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
                    ${CMAKE_CURRENT_BINARY_DIR})
# use the following line to create views with namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE)
# use the following line to create views with namespace CHANGE_ME prefixed
# and path namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE CHANGE_ME)

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_CURRENT_SOURCE_DIR}/models)
target_sources(${PROJECT_NAME}
               PRIVATE
               ${SRC_DIR}
               ${CTL_SRC}
               ${FILTER_SRC}
               ${PLUGIN_SRC}
               ${MODEL_SRC})
# ##############################################################################
# uncomment the following line for dynamically loading views 
# set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS ON)

# ##############################################################################

add_subdirectory(test)
add_subdirectory(external_deps/Bcrypt)
