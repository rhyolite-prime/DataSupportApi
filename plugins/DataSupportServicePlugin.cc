/**
 *
 *  DataSupportServicePlugin.cc
 *
 */

#include "DataSupportServicePlugin.h"

using namespace drogon;
using namespace data_support::plugins;

void DataSupportServicePlugin::initAndStart(const Json::Value &config)
{
    LOG_INFO << "DataSupportServicePlugin started (MagmaNet control plane enabled)";
}

void DataSupportServicePlugin::shutdown() 
{
    /// Shutdown the plugin
}
