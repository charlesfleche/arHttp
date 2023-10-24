#include "resolver.h"

#include "httplib.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/ar/defineResolver.h>

#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(ArHttp, ArResolver);

TF_DEFINE_ENV_SETTING(
    AR_HTTP_SERVER_URL,
    "http://localhost:8000",
    "arHttp server url"
);
TF_DEFINE_ENV_SETTING(
    AR_HTTP_PATH_FORMAT,
    "/%s",
    "arHttp url path format. %s will be replaced by the asset path"
);

TF_DEBUG_CODES(
    AR_HTTP
);

ArHttp::ArHttp() : ArDefaultResolver() {
}
ArHttp::~ArHttp() {
}

ArResolvedPath ArHttp::_Resolve(const std::string& assetPath) const
{
    const auto path = ArDefaultResolver::_Resolve(assetPath);
    if (path) {
        return path;
    }

    const std::string url = TfGetEnvSetting(AR_HTTP_SERVER_URL);
    const std::string format = TfGetEnvSetting(AR_HTTP_PATH_FORMAT);

    httplib::Client cli(url);

    const auto http_path = TfStringPrintf(format.c_str(), assetPath.c_str());
    const auto log_prefix = TfStringPrintf("GET %s%s", url.c_str(), http_path.c_str());

    TF_DEBUG_MSG(AR_HTTP, "%s Requesting...\n", log_prefix.c_str());

    const auto res = cli.Get(http_path);

    const auto err = res.error();
    if (err != httplib::Error::Success)
    {
        TF_DEBUG_MSG(
            AR_HTTP,
            "%s %s\n",
            log_prefix.c_str(),
            httplib::to_string(err).c_str()
        );
        return ArResolvedPath();
    }

    const auto status_message = httplib::status_message(res->status);

    if (res->status != 200)
    {
        TF_DEBUG_MSG(
            AR_HTTP,
            "%s %d %s\n",
            log_prefix.c_str(),
            res->status,
            status_message
        );
        return ArResolvedPath();
    }

    const auto fs_path = res->body;
    TF_DEBUG_MSG(
        AR_HTTP,
        "%s %d %s %s\n",
        log_prefix.c_str(),
        res->status,
        status_message,
        fs_path.c_str()
    );

    return ArResolvedPath(fs_path);
}

PXR_NAMESPACE_CLOSE_SCOPE
