/**
 * =============================================================================
 * Source Python
 * Copyright (C) 2012-2015 Source Python Development Team.  All rights reserved.
 * =============================================================================
 *
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
 * This file has been modified from its original form, under the GNU General
 * Public License, version 3.0.
 */

#pragma once

#include "scripting/callback_manager.h"
#include "scripting/script_engine.h"
#include <map>
#include <optional>

namespace dyno {
class Hook;
} // namespace dyno

namespace counterstrikesharp {

enum DataType_t
{
    DATA_TYPE_VOID = 0,
    DATA_TYPE_BOOL = 1,
    DATA_TYPE_CHAR = 2,
    DATA_TYPE_UCHAR = 3,
    DATA_TYPE_SHORT = 4,
    DATA_TYPE_USHORT = 5,
    DATA_TYPE_INT = 6,
    DATA_TYPE_UINT = 7,
    DATA_TYPE_LONG = 8,
    DATA_TYPE_ULONG = 9,
    DATA_TYPE_LONG_LONG = 10,
    DATA_TYPE_ULONG_LONG = 11,
    DATA_TYPE_FLOAT = 12,
    DATA_TYPE_DOUBLE = 13,
    DATA_TYPE_POINTER = 14,
    DATA_TYPE_STRING = 15,
    DATA_TYPE_VARIANT = 16,

    // DynoHook SIMD types (passed through the script context by pointer indirection).
    DATA_TYPE_M128 = 17,
    DATA_TYPE_M256 = 18,
    DATA_TYPE_M512 = 19
};

enum Protection_t
{
    PROTECTION_NONE,
    PROTECTION_READ,
    PROTECTION_READ_WRITE,
    PROTECTION_EXECUTE,
    PROTECTION_EXECUTE_READ,
    PROTECTION_EXECUTE_READ_WRITE
};

enum Convention_t
{
    CONV_CUSTOM,
    CONV_CDECL,
    CONV_THISCALL,
    CONV_STDCALL,
    CONV_FASTCALL
};

class ValveFunction
{
  public:
    ValveFunction(void* ulAddr, Convention_t callingConvention, std::vector<DataType_t> args, DataType_t returnType);
    ValveFunction(void* ulAddr, Convention_t callingConvention, DataType_t* args, int argCount, DataType_t returnType);

    ~ValveFunction();

    bool IsCallable();

    void SetOffset(int offset) { m_offset = offset; }
    void SetSignature(const char* signature) { m_signature = signature; }

    void Call(ScriptContext& args, int offset = 0, bool bypass = false);
    void AddHook(const std::function<HookResult(HookMode, dyno::Hook&)>& callback);
    void AddHook(CallbackT callable, bool post);
    void RemoveHook(CallbackT callable, bool post);

    void* m_ulAddr;
    void* m_trampoline;
    std::vector<DataType_t> m_Args;
    DataType_t m_eReturnType;

    // Shared built-in calling convention identifier
    Convention_t m_eCallingConvention;

    // DynCall calling convention
    int m_iCallingConvention;

    int m_offset;
    const char* m_signature;
    ScriptCallback* m_precallback = nullptr;
    ScriptCallback* m_postcallback = nullptr;
    std::optional<std::function<HookResult(HookMode, dyno::Hook&)>> m_callback;

    std::vector<HookResult> m_lastPreHookResult;
};

} // namespace counterstrikesharp
