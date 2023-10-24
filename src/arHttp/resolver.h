#pragma once

#include <pxr/usd/ar/defaultResolver.h>

PXR_NAMESPACE_OPEN_SCOPE

class ArHttp : public ArDefaultResolver {
public:
    ArHttp();
    ~ArHttp() override;

protected:
    ArResolvedPath _Resolve(const std::string&) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE