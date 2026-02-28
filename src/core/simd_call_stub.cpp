#include "core/simd_call_stub.h"

#include <cassert>
#include <mutex>
#include <string>
#include <unordered_map>

#include "pch.h"
#include <asmjit/core.h>
#include <asmjit/x86.h>

namespace counterstrikesharp {

static bool DataTypeIsSimd(DataType_t dataType) noexcept
{
    return dataType == DATA_TYPE_M128 || dataType == DATA_TYPE_M256 || dataType == DATA_TYPE_M512;
}

bool SignatureHasSimd(const std::vector<DataType_t>& args, DataType_t returnType) noexcept
{
    if (DataTypeIsSimd(returnType))
    {
        return true;
    }

    for (auto dt : args)
    {
        if (DataTypeIsSimd(dt))
        {
            return true;
        }
    }

    return false;
}

static bool SignatureUsesM256OrM512(const std::vector<DataType_t>& args, DataType_t returnType) noexcept
{
    auto isM256OrM512 = [](DataType_t dt) noexcept {
        return dt == DATA_TYPE_M256 || dt == DATA_TYPE_M512;
    };

    if (isM256OrM512(returnType))
    {
        return true;
    }

    for (auto dt : args)
    {
        if (isM256OrM512(dt))
        {
            return true;
        }
    }

    return false;
}

static bool SignatureUsesM512(const std::vector<DataType_t>& args, DataType_t returnType) noexcept
{
    if (returnType == DATA_TYPE_M512)
    {
        return true;
    }

    for (auto dt : args)
    {
        if (dt == DATA_TYPE_M512)
        {
            return true;
        }
    }

    return false;
}

const char* GetSimdSupportError(const std::vector<DataType_t>& args, DataType_t returnType) noexcept
{
    const auto& cpuInfo = asmjit::CpuInfo::host();

    if (SignatureUsesM512(args, returnType) && !cpuInfo.hasFeature(asmjit::CpuFeatures::X86::kAVX512_F))
    {
        return "M512 requires AVX-512 (AVX512F) support";
    }

    if (SignatureUsesM256OrM512(args, returnType) && !cpuInfo.hasFeature(asmjit::CpuFeatures::X86::kAVX))
    {
        return "M256 requires AVX support";
    }

    return nullptr;
}

static asmjit::TypeId ConvertDataTypeToAsmjitTypeId(DataType_t dataType)
{
    using asmjit::TypeId;

    switch (dataType)
    {
        case DATA_TYPE_VOID:
            return TypeId::kVoid;
        case DATA_TYPE_BOOL:
            return TypeId::kUInt8;
        case DATA_TYPE_CHAR:
            return TypeId::kInt8;
        case DATA_TYPE_UCHAR:
            return TypeId::kUInt8;
        case DATA_TYPE_SHORT:
            return TypeId::kInt16;
        case DATA_TYPE_USHORT:
            return TypeId::kUInt16;
        case DATA_TYPE_INT:
            return TypeId::kInt32;
        case DATA_TYPE_UINT:
            return TypeId::kUInt32;
        case DATA_TYPE_LONG:
#ifdef _WIN32
            return TypeId::kInt32;
#else
            return TypeId::kInt64;
#endif
        case DATA_TYPE_ULONG:
#ifdef _WIN32
            return TypeId::kUInt32;
#else
            return TypeId::kUInt64;
#endif
        case DATA_TYPE_LONG_LONG:
            return TypeId::kInt64;
        case DATA_TYPE_ULONG_LONG:
            return TypeId::kUInt64;
        case DATA_TYPE_FLOAT:
            return TypeId::kFloat32;
        case DATA_TYPE_DOUBLE:
            return TypeId::kFloat64;
        case DATA_TYPE_POINTER:
        case DATA_TYPE_STRING:
            return TypeId::kUIntPtr;
        case DATA_TYPE_M128:
            return TypeId::kInt8x16;
        case DATA_TYPE_M256:
            return TypeId::kInt8x32;
        case DATA_TYPE_M512:
            return TypeId::kInt8x64;
        default:
            assert(!"Unknown function parameter type!");
            return TypeId::kUIntPtr;
    }
}

static std::string BuildSimdStubKey(asmjit::CallConvId callConvId, const std::vector<DataType_t>& args, DataType_t returnType)
{
    std::string key;
    key.reserve(2 + args.size());

    key.push_back(static_cast<char>(static_cast<unsigned char>(callConvId)));
    key.push_back(static_cast<char>(static_cast<unsigned char>(returnType)));

    for (auto dt : args)
    {
        key.push_back(static_cast<char>(static_cast<unsigned char>(dt)));
    }

    return key;
}

static void SetAsmjitInvokeArgFromRawSlot(asmjit::x86::Compiler& cc,
                                          asmjit::InvokeNode* invokeNode,
                                          uint32_t argIndex,
                                          asmjit::x86::Gp rawArgs,
                                          int32_t slotOffset,
                                          DataType_t argType)
{
    switch (argType)
    {
        case DATA_TYPE_BOOL:
        case DATA_TYPE_UCHAR:
        {
            asmjit::x86::Gp arg = cc.newUInt8();
            cc.mov(arg, asmjit::x86::byte_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_CHAR:
        {
            asmjit::x86::Gp arg = cc.newInt8();
            cc.mov(arg, asmjit::x86::byte_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_SHORT:
        {
            asmjit::x86::Gp arg = cc.newInt16();
            cc.mov(arg, asmjit::x86::word_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_USHORT:
        {
            asmjit::x86::Gp arg = cc.newUInt16();
            cc.mov(arg, asmjit::x86::word_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_INT:
        {
            asmjit::x86::Gp arg = cc.newInt32();
            cc.mov(arg, asmjit::x86::dword_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_UINT:
        {
            asmjit::x86::Gp arg = cc.newUInt32();
            cc.mov(arg, asmjit::x86::dword_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_LONG:
        case DATA_TYPE_ULONG:
        {
#ifdef _WIN32
            asmjit::x86::Gp arg = (argType == DATA_TYPE_ULONG) ? cc.newUInt32() : cc.newInt32();
            cc.mov(arg, asmjit::x86::dword_ptr(rawArgs, slotOffset));
#else
            asmjit::x86::Gp arg = (argType == DATA_TYPE_ULONG) ? cc.newUInt64() : cc.newInt64();
            cc.mov(arg, asmjit::x86::qword_ptr(rawArgs, slotOffset));
#endif
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_LONG_LONG:
        case DATA_TYPE_ULONG_LONG:
        {
            asmjit::x86::Gp arg = (argType == DATA_TYPE_ULONG_LONG) ? cc.newUInt64() : cc.newInt64();
            cc.mov(arg, asmjit::x86::qword_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_FLOAT:
        {
            asmjit::x86::Xmm arg = cc.newXmmSs();
            cc.movss(arg, asmjit::x86::dword_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_DOUBLE:
        {
            asmjit::x86::Xmm arg = cc.newXmmSd();
            cc.movsd(arg, asmjit::x86::qword_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_POINTER:
        case DATA_TYPE_STRING:
        {
            asmjit::x86::Gp arg = cc.newUIntPtr();
            cc.mov(arg, asmjit::x86::qword_ptr(rawArgs, slotOffset));
            invokeNode->setArg(argIndex, arg);
            break;
        }
        case DATA_TYPE_M128:
        {
            asmjit::x86::Gp valuePtr = cc.newUIntPtr();
            cc.mov(valuePtr, asmjit::x86::qword_ptr(rawArgs, slotOffset));

            asmjit::x86::Xmm vec = cc.newXmm();
            cc.movups(vec, asmjit::x86::ptr(valuePtr));
            invokeNode->setArg(argIndex, vec);
            break;
        }
        case DATA_TYPE_M256:
        {
            asmjit::x86::Gp valuePtr = cc.newUIntPtr();
            cc.mov(valuePtr, asmjit::x86::qword_ptr(rawArgs, slotOffset));

            asmjit::x86::Ymm vec = cc.newYmm();
            cc.vmovups(vec, asmjit::x86::ptr(valuePtr));
            invokeNode->setArg(argIndex, vec);
            break;
        }
        case DATA_TYPE_M512:
        {
            asmjit::x86::Gp valuePtr = cc.newUIntPtr();
            cc.mov(valuePtr, asmjit::x86::qword_ptr(rawArgs, slotOffset));

            asmjit::x86::Zmm vec = cc.newZmm();
            cc.vmovups(vec, asmjit::x86::ptr(valuePtr));
            invokeNode->setArg(argIndex, vec);
            break;
        }
        default:
            assert(!"Unknown function parameter type!");
            break;
    }
}

static void
SetAsmjitInvokeRetToBuffer(asmjit::x86::Compiler& cc, asmjit::InvokeNode* invokeNode, asmjit::x86::Gp retBuffer, DataType_t returnType)
{
    switch (returnType)
    {
        case DATA_TYPE_VOID:
            break;
        case DATA_TYPE_BOOL:
        case DATA_TYPE_UCHAR:
        {
            asmjit::x86::Gp ret = cc.newUInt8();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::byte_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_CHAR:
        {
            asmjit::x86::Gp ret = cc.newInt8();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::byte_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_SHORT:
        {
            asmjit::x86::Gp ret = cc.newInt16();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::word_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_USHORT:
        {
            asmjit::x86::Gp ret = cc.newUInt16();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::word_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_INT:
        {
            asmjit::x86::Gp ret = cc.newInt32();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::dword_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_UINT:
        {
            asmjit::x86::Gp ret = cc.newUInt32();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::dword_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_LONG:
        case DATA_TYPE_ULONG:
        {
#ifdef _WIN32
            asmjit::x86::Gp ret = (returnType == DATA_TYPE_ULONG) ? cc.newUInt32() : cc.newInt32();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::dword_ptr(retBuffer), ret);
#else
            asmjit::x86::Gp ret = (returnType == DATA_TYPE_ULONG) ? cc.newUInt64() : cc.newInt64();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::qword_ptr(retBuffer), ret);
#endif
            break;
        }
        case DATA_TYPE_LONG_LONG:
        case DATA_TYPE_ULONG_LONG:
        {
            asmjit::x86::Gp ret = (returnType == DATA_TYPE_ULONG_LONG) ? cc.newUInt64() : cc.newInt64();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::qword_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_FLOAT:
        {
            asmjit::x86::Xmm ret = cc.newXmmSs();
            invokeNode->setRet(0, ret);
            cc.movss(asmjit::x86::dword_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_DOUBLE:
        {
            asmjit::x86::Xmm ret = cc.newXmmSd();
            invokeNode->setRet(0, ret);
            cc.movsd(asmjit::x86::qword_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_POINTER:
        case DATA_TYPE_STRING:
        {
            asmjit::x86::Gp ret = cc.newUIntPtr();
            invokeNode->setRet(0, ret);
            cc.mov(asmjit::x86::qword_ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_M128:
        {
            asmjit::x86::Xmm ret = cc.newXmm();
            invokeNode->setRet(0, ret);
            cc.movups(asmjit::x86::ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_M256:
        {
            asmjit::x86::Ymm ret = cc.newYmm();
            invokeNode->setRet(0, ret);
            cc.vmovups(asmjit::x86::ptr(retBuffer), ret);
            break;
        }
        case DATA_TYPE_M512:
        {
            asmjit::x86::Zmm ret = cc.newZmm();
            invokeNode->setRet(0, ret);
            cc.vmovups(asmjit::x86::ptr(retBuffer), ret);
            break;
        }
        default:
            assert(!"Unknown function return type!");
            break;
    }
}

static SimdCallStub GetOrCreateSimdCallStub(asmjit::CallConvId targetCallConv,
                                            const std::vector<DataType_t>& argTypes,
                                            DataType_t returnType,
                                            const asmjit::CpuFeatures& cpuFeatures)
{
    static asmjit::JitRuntime runtime;
    static std::mutex mutex;
    static std::unordered_map<std::string, SimdCallStub> cache;

    std::lock_guard<std::mutex> lock(mutex);

    auto key = BuildSimdStubKey(targetCallConv, argTypes, returnType);
    auto it = cache.find(key);
    if (it != cache.end())
    {
        return it->second;
    }

    asmjit::CodeHolder code;
    code.init(asmjit::Environment::host(), cpuFeatures);

    asmjit::x86::Compiler cc(&code);

    asmjit::FuncSignatureBuilder stubSignature(asmjit::CallConvId::kHost);
    stubSignature.setRet(asmjit::TypeId::kVoid);
    stubSignature.addArg(asmjit::TypeId::kUIntPtr); // targetFunc
    stubSignature.addArg(asmjit::TypeId::kUIntPtr); // rawArgs
    stubSignature.addArg(asmjit::TypeId::kUIntPtr); // retBuffer

    asmjit::FuncNode* funcNode = cc.addFunc(stubSignature);

    asmjit::x86::Gp targetFunc = cc.newUIntPtr("targetFunc");
    asmjit::x86::Gp rawArgs = cc.newUIntPtr("rawArgs");
    asmjit::x86::Gp retBuffer = cc.newUIntPtr("retBuffer");

    funcNode->setArg(0, targetFunc);
    funcNode->setArg(1, rawArgs);
    funcNode->setArg(2, retBuffer);

    asmjit::FuncSignatureBuilder callSignature(targetCallConv);
    callSignature.setRet(ConvertDataTypeToAsmjitTypeId(returnType));
    for (auto dt : argTypes)
    {
        callSignature.addArg(ConvertDataTypeToAsmjitTypeId(dt));
    }

    asmjit::InvokeNode* invokeNode = nullptr;
    cc.invoke(&invokeNode, targetFunc, callSignature);

    for (size_t i = 0; i < argTypes.size(); i++)
    {
        const int32_t slotOffset = static_cast<int32_t>(i * sizeof(uint64_t));
        SetAsmjitInvokeArgFromRawSlot(cc, invokeNode, static_cast<uint32_t>(i), rawArgs, slotOffset, argTypes[i]);
    }

    SetAsmjitInvokeRetToBuffer(cc, invokeNode, retBuffer, returnType);

    cc.endFunc();

    asmjit::Error err = cc.finalize();
    if (err)
    {
        return nullptr;
    }

    SimdCallStub fn = nullptr;
    err = runtime.add(&fn, &code);
    if (err)
    {
        return nullptr;
    }

    cache.emplace(std::move(key), fn);
    return fn;
}

SimdCallStub GetOrCreateSimdCallStubForHost(const std::vector<DataType_t>& argTypes, DataType_t returnType)
{
    const auto& cpuInfo = asmjit::CpuInfo::host();

#ifdef _WIN32
    constexpr asmjit::CallConvId targetCallConv = asmjit::CallConvId::kVectorCall;
#else
    constexpr asmjit::CallConvId targetCallConv = asmjit::CallConvId::kX64SystemV;
#endif

    return GetOrCreateSimdCallStub(targetCallConv, argTypes, returnType, cpuInfo.features());
}

} // namespace counterstrikesharp
