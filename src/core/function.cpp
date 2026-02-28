/**
 * =============================================================================
 * Source Python
 * Copyright (C) 2012-2015 Source Python Development Team.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, the Source Python Team gives you permission
 * to link the code of this program (as well as its derivative works) to
 * "Half-Life 2," the "Source Engine," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, the Source.Python
 * Development Team grants this exception to all derivative works.
 *
 * This file has been modified from its original form, under the terms of GNU
 * General Public License, version 3.0.
 */

#include "core/function.h"
#include "core/simd_call_stub.h"

#include <algorithm>
#include <cstdint>

#include "core/log.h"
#include "dyncall/dyncall/dyncall.h"

#include "pch.h"
#include "dynohook/core.h"
#include "dynohook/manager.h"

#ifdef _WIN32
#include "dynohook/conventions/x64/x64MsFastcall.h"
#else
#include "dynohook/conventions/x64/x64SystemVcall.h"
#endif

namespace counterstrikesharp {

DCCallVM* g_pCallVM = dcNewCallVM(4096);
std::map<dyno::Hook*, ValveFunction*> g_HookMap;

// ============================================================================
// >> GetDynCallConvention
// ============================================================================
int GetDynCallConvention(Convention_t eConv)
{
    switch (eConv)
    {
        case CONV_CUSTOM:
            return -1;
        case CONV_CDECL:
            return DC_CALL_C_DEFAULT;
        case CONV_THISCALL:
#ifdef _WIN32
            return DC_CALL_C_X86_WIN32_THIS_MS;
#else
            return DC_CALL_C_X86_WIN32_THIS_GNU;
#endif
#ifdef _WIN32
        case CONV_STDCALL:
            return DC_CALL_C_X86_WIN32_STD;
        case CONV_FASTCALL:
            return DC_CALL_C_X86_WIN32_FAST_MS;
#endif
    }

    return -1;
}

ValveFunction::ValveFunction(void* ulAddr, Convention_t callingConvention, std::vector<DataType_t> args, DataType_t returnType)
    : m_ulAddr(ulAddr)
{
    m_Args = args;

    m_eReturnType = returnType;

    m_eCallingConvention = callingConvention;

    m_iCallingConvention = GetDynCallConvention(m_eCallingConvention);
}

ValveFunction::ValveFunction(void* ulAddr, Convention_t callingConvention, DataType_t* args, int argCount, DataType_t returnType)
    : m_ulAddr(ulAddr)

{
    m_Args = std::vector<DataType_t>(args, args + argCount);
    m_eReturnType = returnType;

    m_eCallingConvention = callingConvention;
    m_iCallingConvention = GetDynCallConvention(m_eCallingConvention);
}

ValveFunction::~ValveFunction()
{
    if (m_precallback != nullptr)
    {
        globals::callbackManager.ReleaseCallback(m_precallback);
        m_precallback = nullptr;
    }

    if (m_postcallback != nullptr)
    {
        globals::callbackManager.ReleaseCallback(m_postcallback);
        m_postcallback = nullptr;
    }
}

bool ValveFunction::IsCallable() { return (m_eCallingConvention != CONV_CUSTOM) && (m_iCallingConvention != -1); }

template <class ReturnType, class Function> ReturnType CallHelper(Function func, DCCallVM* vm, void* addr)
{
    return static_cast<ReturnType>(func(vm, addr));
}

static void CallHelperVoid(DCCallVM* vm, void* addr) { dcCallVoid(vm, addr); }

static void PushDynCallArg(DCCallVM* vm, ScriptContext& script_context, int contextIndex, DataType_t argType)
{
    switch (argType)
    {
        case DATA_TYPE_BOOL:
            dcArgBool(vm, script_context.GetArgument<bool>(contextIndex));
            break;
        case DATA_TYPE_CHAR:
            dcArgChar(vm, script_context.GetArgument<char>(contextIndex));
            break;
        case DATA_TYPE_UCHAR:
            dcArgChar(vm, script_context.GetArgument<unsigned char>(contextIndex));
            break;
        case DATA_TYPE_SHORT:
            dcArgShort(vm, script_context.GetArgument<short>(contextIndex));
            break;
        case DATA_TYPE_USHORT:
            dcArgShort(vm, script_context.GetArgument<unsigned short>(contextIndex));
            break;
        case DATA_TYPE_INT:
            dcArgInt(vm, script_context.GetArgument<int>(contextIndex));
            break;
        case DATA_TYPE_UINT:
            dcArgInt(vm, script_context.GetArgument<unsigned int>(contextIndex));
            break;
        case DATA_TYPE_LONG:
            dcArgLong(vm, script_context.GetArgument<long>(contextIndex));
            break;
        case DATA_TYPE_ULONG:
            dcArgLong(vm, script_context.GetArgument<unsigned long>(contextIndex));
            break;
        case DATA_TYPE_LONG_LONG:
            dcArgLongLong(vm, script_context.GetArgument<long long>(contextIndex));
            break;
        case DATA_TYPE_ULONG_LONG:
            dcArgLongLong(vm, script_context.GetArgument<unsigned long long>(contextIndex));
            break;
        case DATA_TYPE_FLOAT:
            dcArgFloat(vm, script_context.GetArgument<float>(contextIndex));
            break;
        case DATA_TYPE_DOUBLE:
            dcArgDouble(vm, script_context.GetArgument<double>(contextIndex));
            break;
        case DATA_TYPE_POINTER:
            dcArgPointer(vm, script_context.GetArgument<void*>(contextIndex));
            break;
        case DATA_TYPE_STRING:
            dcArgPointer(vm, (void*)script_context.GetArgument<const char*>(contextIndex));
            break;
        default:
            assert(!"Unknown function parameter type!");
            break;
    }
}

static void CallDynCallAndSetResult(DCCallVM* vm, ScriptContext& script_context, void* target, DataType_t returnType)
{
    switch (returnType)
    {
        case DATA_TYPE_VOID:
            CallHelperVoid(vm, target);
            break;
        case DATA_TYPE_BOOL:
            script_context.SetResult(CallHelper<bool>(dcCallBool, vm, target));
            break;
        case DATA_TYPE_CHAR:
            script_context.SetResult(CallHelper<char>(dcCallChar, vm, target));
            break;
        case DATA_TYPE_UCHAR:
            script_context.SetResult(CallHelper<unsigned char>(dcCallChar, vm, target));
            break;
        case DATA_TYPE_SHORT:
            script_context.SetResult(CallHelper<short>(dcCallShort, vm, target));
            break;
        case DATA_TYPE_USHORT:
            script_context.SetResult(CallHelper<unsigned short>(dcCallShort, vm, target));
            break;
        case DATA_TYPE_INT:
            script_context.SetResult(CallHelper<int>(dcCallInt, vm, target));
            break;
        case DATA_TYPE_UINT:
            script_context.SetResult(CallHelper<unsigned int>(dcCallInt, vm, target));
            break;
        case DATA_TYPE_LONG:
            script_context.SetResult(CallHelper<long>(dcCallLong, vm, target));
            break;
        case DATA_TYPE_ULONG:
            script_context.SetResult(CallHelper<unsigned long>(dcCallLong, vm, target));
            break;
        case DATA_TYPE_LONG_LONG:
            script_context.SetResult(CallHelper<long long>(dcCallLongLong, vm, target));
            break;
        case DATA_TYPE_ULONG_LONG:
            script_context.SetResult(CallHelper<unsigned long long>(dcCallLongLong, vm, target));
            break;
        case DATA_TYPE_FLOAT:
            script_context.SetResult(CallHelper<float>(dcCallFloat, vm, target));
            break;
        case DATA_TYPE_DOUBLE:
            script_context.SetResult(CallHelper<double>(dcCallDouble, vm, target));
            break;
        case DATA_TYPE_POINTER:
            script_context.SetResult(CallHelper<void*>(dcCallPointer, vm, target));
            break;
        case DATA_TYPE_STRING:
            script_context.SetResult(CallHelper<const char*>(dcCallPointer, vm, target));
            break;
        default:
            assert(!"Unknown function return type!");
            break;
    }
}

static void SetResultFromSimdRetBuffer(ScriptContext& script_context, DataType_t returnType, uint8_t* retBuffer)
{
    switch (returnType)
    {
        case DATA_TYPE_VOID:
            break;
        case DATA_TYPE_BOOL:
            script_context.SetResult(*reinterpret_cast<bool*>(retBuffer));
            break;
        case DATA_TYPE_CHAR:
            script_context.SetResult(*reinterpret_cast<char*>(retBuffer));
            break;
        case DATA_TYPE_UCHAR:
            script_context.SetResult(*reinterpret_cast<unsigned char*>(retBuffer));
            break;
        case DATA_TYPE_SHORT:
            script_context.SetResult(*reinterpret_cast<short*>(retBuffer));
            break;
        case DATA_TYPE_USHORT:
            script_context.SetResult(*reinterpret_cast<unsigned short*>(retBuffer));
            break;
        case DATA_TYPE_INT:
            script_context.SetResult(*reinterpret_cast<int*>(retBuffer));
            break;
        case DATA_TYPE_UINT:
            script_context.SetResult(*reinterpret_cast<unsigned int*>(retBuffer));
            break;
        case DATA_TYPE_LONG:
            script_context.SetResult(*reinterpret_cast<long*>(retBuffer));
            break;
        case DATA_TYPE_ULONG:
            script_context.SetResult(*reinterpret_cast<unsigned long*>(retBuffer));
            break;
        case DATA_TYPE_LONG_LONG:
            script_context.SetResult(*reinterpret_cast<long long*>(retBuffer));
            break;
        case DATA_TYPE_ULONG_LONG:
            script_context.SetResult(*reinterpret_cast<unsigned long long*>(retBuffer));
            break;
        case DATA_TYPE_FLOAT:
            script_context.SetResult(*reinterpret_cast<float*>(retBuffer));
            break;
        case DATA_TYPE_DOUBLE:
            script_context.SetResult(*reinterpret_cast<double*>(retBuffer));
            break;
        case DATA_TYPE_POINTER:
            script_context.SetResult(*reinterpret_cast<void**>(retBuffer));
            break;
        case DATA_TYPE_STRING:
            script_context.SetResult(*reinterpret_cast<const char**>(retBuffer));
            break;
        case DATA_TYPE_M128:
        case DATA_TYPE_M256:
        case DATA_TYPE_M512:
            script_context.SetResult(static_cast<void*>(retBuffer));
            break;
        default:
            assert(!"Unknown function return type!");
            break;
    }
}

void ValveFunction::Call(ScriptContext& script_context, int offset, bool bypass)
{
    if (!IsCallable()) return;

    void* target = m_ulAddr;
    if (bypass && m_trampoline)
    {
        target = m_trampoline;
    }

    if (SignatureHasSimd(m_Args, m_eReturnType))
    {
        if (const char* simdSupportError = GetSimdSupportError(m_Args, m_eReturnType))
        {
            script_context.ThrowNativeError(simdSupportError);
            return;
        }

        auto* rawArgs = reinterpret_cast<const uint64_t*>(script_context.GetArgumentBuffer()) + offset;

        thread_local alignas(64) uint8_t retBuffer[64];

        auto stub = GetOrCreateSimdCallStubForHost(m_Args, m_eReturnType);
        if (!stub)
        {
            script_context.ThrowNativeError("Failed to build SIMD call stub");
            return;
        }

        stub(target, rawArgs, retBuffer);
        SetResultFromSimdRetBuffer(script_context, m_eReturnType, retBuffer);

        return;
    }

    dcReset(g_pCallVM);
    dcMode(g_pCallVM, m_iCallingConvention);

    for (size_t i = 0; i < m_Args.size(); i++)
    {
        const int contextIndex = static_cast<int>(i) + offset;
        PushDynCallArg(g_pCallVM, script_context, contextIndex, m_Args[i]);
    }

    CallDynCallAndSetResult(g_pCallVM, script_context, target, m_eReturnType);
}

static HookResult ExecuteScriptHookCallbacks(ScriptCallback* callback, dyno::Hook& hook, HookResult maxResult)
{
    if (callback == nullptr)
    {
        return maxResult;
    }

    callback->Reset();
    callback->ScriptContext().Push(&hook);

    for (auto fnMethodToCall : callback->GetFunctions())
    {
        if (!fnMethodToCall) continue;
        fnMethodToCall(&callback->ScriptContextStruct());

        auto result = callback->ScriptContext().GetResult<HookResult>();
        maxResult = (std::max)(result, maxResult);

        if (maxResult >= HookResult::Stop)
        {
            break;
        }
    }

    return maxResult;
}

dyno::ReturnAction HookHandler(dyno::HookType hookType, dyno::Hook& hook)
{
    auto* vf = g_HookMap[&hook];

    if (hookType == dyno::HookType::Pre)
    {
        auto global_callback = vf->m_callback;
        HookResult maxResult = HookResult::Continue;

        if (global_callback.has_value())
        {
            HookResult result = global_callback.value()(HookMode::Pre, hook);
            maxResult = (std::max)(result, maxResult);
        }

        maxResult = ExecuteScriptHookCallbacks(vf->m_precallback, hook, maxResult);

        // Store the pre-hook result for the post-hook to check
        vf->m_lastPreHookResult.push_back(maxResult);

        if (maxResult >= HookResult::Handled)
        {
            return dyno::ReturnAction::Supercede;
        }

        return dyno::ReturnAction::Ignored;
    }

    // Post hook
    HookResult preResult = HookResult::Continue;
    if (!vf->m_lastPreHookResult.empty())
    {
        preResult = vf->m_lastPreHookResult.back();
        vf->m_lastPreHookResult.pop_back();
    }

    if (preResult >= HookResult::Handled)
    {
        return dyno::ReturnAction::Ignored;
    }

    auto* callback = vf->m_postcallback;
    auto global_callback = vf->m_callback;

    if (callback == nullptr && !global_callback.has_value())
    {
        return dyno::ReturnAction::Ignored;
    }

    if (global_callback.has_value())
    {
        HookResult result = global_callback.value()(HookMode::Post, hook);
        if (result >= HookResult::Handled)
        {
            return dyno::ReturnAction::Supercede;
        }
    }

    if (callback == nullptr)
    {
        return dyno::ReturnAction::Ignored;
    }

    HookResult maxResult = ExecuteScriptHookCallbacks(callback, hook, HookResult::Continue);

    if (maxResult >= HookResult::Handled)
    {
        return dyno::ReturnAction::Supercede;
    }

    return dyno::ReturnAction::Ignored;
}

static dyno::DataObject ConvertDataTypeToDynoHook(DataType_t dataType)
{
    switch (dataType)
    {
        case DATA_TYPE_VOID:
            return dyno::DataObject(dyno::DataType::Void);
        case DATA_TYPE_BOOL:
            return dyno::DataObject(dyno::DataType::Bool);
        case DATA_TYPE_CHAR:
            return dyno::DataObject(dyno::DataType::Char);
        case DATA_TYPE_UCHAR:
            return dyno::DataObject(dyno::DataType::UChar);
        case DATA_TYPE_SHORT:
            return dyno::DataObject(dyno::DataType::Short);
        case DATA_TYPE_USHORT:
            return dyno::DataObject(dyno::DataType::UShort);
        case DATA_TYPE_INT:
            return dyno::DataObject(dyno::DataType::Int);
        case DATA_TYPE_UINT:
            return dyno::DataObject(dyno::DataType::UInt);
        case DATA_TYPE_LONG:
            return dyno::DataObject(dyno::DataType::Long);
        case DATA_TYPE_ULONG:
            return dyno::DataObject(dyno::DataType::ULong);
        case DATA_TYPE_LONG_LONG:
            return dyno::DataObject(dyno::DataType::LongLong);
        case DATA_TYPE_ULONG_LONG:
            return dyno::DataObject(dyno::DataType::ULongLong);
        case DATA_TYPE_FLOAT:
            return dyno::DataObject(dyno::DataType::Float);
        case DATA_TYPE_DOUBLE:
            return dyno::DataObject(dyno::DataType::Double);
        case DATA_TYPE_POINTER:
            return dyno::DataObject(dyno::DataType::Pointer);
        case DATA_TYPE_STRING:
            return dyno::DataObject(dyno::DataType::String);
        case DATA_TYPE_VARIANT:
            return dyno::DataObject(dyno::DataType::Object);
        case DATA_TYPE_M128:
            return dyno::DataObject(dyno::DataType::M128);
        case DATA_TYPE_M256:
            return dyno::DataObject(dyno::DataType::M256);
        case DATA_TYPE_M512:
            return dyno::DataObject(dyno::DataType::M512);
        default:
            assert(!"Unknown function parameter type!");
            return dyno::DataObject(dyno::DataType::Pointer);
    }
}

std::vector<dyno::DataObject> ConvertArgsToDynoHook(const std::vector<DataType_t>& dataTypes)
{
    std::vector<dyno::DataObject> converted;
    converted.reserve(dataTypes.size());

    for (DataType_t dt : dataTypes)
    {
        converted.push_back(ConvertDataTypeToDynoHook(dt));
    }

    return converted;
}

static dyno::Hook* GetOrCreateDynoHook(ValveFunction& valveFunction)
{
    dyno::HookManager& manager = dyno::HookManager::Get();
    dyno::Hook* hook = manager.hook((void*)valveFunction.m_ulAddr, [&valveFunction] {
#ifdef _WIN32
        return new dyno::x64MsFastcall(ConvertArgsToDynoHook(valveFunction.m_Args), ConvertDataTypeToDynoHook(valveFunction.m_eReturnType));
#else
        return new dyno::x64SystemVcall(ConvertArgsToDynoHook(valveFunction.m_Args),
                                        ConvertDataTypeToDynoHook(valveFunction.m_eReturnType));
#endif
    });
    g_HookMap[hook] = &valveFunction;
    return hook;
}

void ValveFunction::AddHook(const std::function<HookResult(HookMode, dyno::Hook&)>& callback)
{
    dyno::Hook* hook = GetOrCreateDynoHook(*this);
    hook->addCallback(dyno::HookType::Post, (dyno::HookHandler*)&HookHandler);
    hook->addCallback(dyno::HookType::Pre, (dyno::HookHandler*)&HookHandler);
    m_trampoline = hook->getOriginal();
    m_callback = callback;
}

void ValveFunction::AddHook(CallbackT callable, bool post)
{
    dyno::Hook* hook = GetOrCreateDynoHook(*this);
    hook->addCallback(dyno::HookType::Post, (dyno::HookHandler*)&HookHandler);
    hook->addCallback(dyno::HookType::Pre, (dyno::HookHandler*)&HookHandler);
    m_trampoline = hook->getOriginal();

    ScriptCallback*& callback = post ? m_postcallback : m_precallback;
    if (callback == nullptr)
    {
        callback = globals::callbackManager.CreateCallback("");
    }

    callback->AddListener(callable);
}
void ValveFunction::RemoveHook(CallbackT callable, bool post)
{
    GetOrCreateDynoHook(*this);
    m_trampoline = nullptr;

    ScriptCallback* callback = post ? m_postcallback : m_precallback;
    if (callback != nullptr)
    {
        callback->RemoveListener(callable);
    }
}

} // namespace counterstrikesharp
