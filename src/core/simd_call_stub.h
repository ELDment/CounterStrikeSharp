#pragma once

#include "core/function.h"

#include <cstdint>
#include <vector>

namespace counterstrikesharp {

using SimdCallStub = void (*)(void* targetFunc, const uint64_t* rawArgs, void* retBuffer);

bool SignatureHasSimd(const std::vector<DataType_t>& args, DataType_t returnType) noexcept;

const char* GetSimdSupportError(const std::vector<DataType_t>& args, DataType_t returnType) noexcept;

SimdCallStub GetOrCreateSimdCallStubForHost(const std::vector<DataType_t>& argTypes, DataType_t returnType);

} // namespace counterstrikesharp
