/*
 * Copyright (c) 2014 Bas Timmer/NTAuthority et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file has been modified from its original form for use in this program
 * under GNU Lesser General Public License, version 2.
 */

using System.Text;
using System.Drawing;
using System.Security;
using System.Collections.Concurrent;
using System.Runtime.InteropServices;
using CounterStrikeSharp.API.Modules.Utils;
using CounterStrikeSharp.API.Modules.Memory;

namespace CounterStrikeSharp.API.Core
{
    public class NativeException(string message) : Exception(message)
    {
    }

    [StructLayout(LayoutKind.Sequential)]
    [Serializable]
    public unsafe struct FxScriptContext
    {
        public int numArguments;
        public int numResults;
        public int hasError;

        public ulong nativeIdentifier;
        public fixed byte functionData[8 * 32];
        public fixed byte result[8];
    }

    public class ScriptContext
    {
        [ThreadStatic]
        private static ScriptContext? _globalScriptContext;

        public static ScriptContext GlobalScriptContext
        {
            get
            {
                _globalScriptContext ??= new ScriptContext();
                return _globalScriptContext;
            }
        }

        public unsafe ScriptContext()
        {
        }

        public unsafe ScriptContext(FxScriptContext* context)
        {
            m_extContext = *context;
        }

        private readonly ConcurrentQueue<Action> ms_finalizers = new();

        private readonly object ms_lock = new();

        internal object Lock => ms_lock;

        internal FxScriptContext m_extContext = new();

        internal bool isCleanupLocked = false;

        [SecuritySafeCritical]
        public void Reset()
        {
            InternalReset();
        }

        [SecurityCritical]
        private void InternalReset()
        {
            m_extContext.numArguments = 0;
            m_extContext.numResults = 0;
            m_extContext.hasError = 0;
            //CleanUp();
        }

        [SecuritySafeCritical]
        public void Invoke()
        {
            if (!isCleanupLocked)
            {
                isCleanupLocked = true;
                InvokeNativeInternal();
                GlobalCleanUp();
                isCleanupLocked = false;
                return;
            }

            InvokeNativeInternal();
        }

        [SecurityCritical]
        private unsafe void InvokeNativeInternal()
        {
            fixed (FxScriptContext* cxt = &m_extContext)
            {
                Helpers.InvokeNative(new IntPtr(cxt));
            }
        }

        public unsafe byte[] GetBytes()
        {
            fixed (FxScriptContext* context = &m_extContext)
            {
                byte[] arr = new byte[8 * 32];
                Marshal.Copy((IntPtr)context->functionData, arr, 0, 8 * 32);
                return arr;
            }
        }

        public unsafe IntPtr GetContextUnderlyingAddress()
        {
            fixed (FxScriptContext* context = &m_extContext)
            {
                return (IntPtr)context;
            }
        }

        [SecuritySafeCritical]
        public void Push(object arg)
        {
            PushInternal(arg);
        }

        [SecuritySafeCritical]
        public unsafe void SetResult(object arg, FxScriptContext* cxt)
        {
            SetResultInternal(cxt, arg);
        }

        [SecurityCritical]
        private unsafe void PushInternal(object arg)
        {
            fixed (FxScriptContext* context = &m_extContext)
            {
                Push(context, arg);
            }
        }

        [SecurityCritical]
        public unsafe void SetIdentifier(ulong arg)
        {
            fixed (FxScriptContext* context = &m_extContext)
            {
                context->nativeIdentifier = arg;
            }
        }

        public unsafe void CheckErrors()
        {
            fixed (FxScriptContext* context = &m_extContext)
            {
                if (Convert.ToBoolean(context->hasError))
                {
                    string error = GetResult<string>();
                    Reset();
                    throw new NativeException(error);
                }
            }
        }

        [SecurityCritical]
        internal unsafe void Push(FxScriptContext* context, object arg)
        {
            arg ??= 0;

            if (arg.GetType().IsEnum)
            {
                arg = Convert.ChangeType(arg, arg.GetType().GetEnumUnderlyingType());
            }

            if (arg is string)
            {
                var str = (string)Convert.ChangeType(arg, typeof(string));
                PushString(context, str);
                return;
            }
            else if (arg is InputArgument ia)
            {
                Push(context, ia.Value);
                return;
            }
            else if (arg is IMarshalToNative marshalToNative)
            {
                foreach (var value in marshalToNative.GetNativeObject())
                {
                    Push(context, value);
                }
                return;
            }
            else if (arg is NativeObject nativeObject)
            {
                Push(context, (InputArgument)nativeObject);
                return;
            }
            else if (arg is NativeEntity nativeValue)
            {
                Push(context, (InputArgument)nativeValue);
                return;
            }

            if (arg is M128 m128)
            {
                PushSimd(context, m128);
                return;
            }

            if (arg is M256 m256)
            {
                PushSimd(context, m256);
                return;
            }

            if (arg is M512 m512)
            {
                PushSimd(context, m512);
                return;
            }

            if (Marshal.SizeOf(arg.GetType()) <= 8)
            {
                PushUnsafe(context, arg);
            }

            context->numArguments++;
        }

        [SecurityCritical]
        private unsafe void PushSimd<T>(FxScriptContext* context, T value) where T : struct
        {
            var size = Marshal.SizeOf<T>();
            var ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(value, ptr, false);
            ms_finalizers.Enqueue(() => Free(ptr));

            *(IntPtr*)&context->functionData[8 * context->numArguments] = ptr;
            context->numArguments++;
        }

        [SecurityCritical]
        internal unsafe void SetResultInternal(FxScriptContext* context, object arg)
        {
            arg ??= 0;

            if (arg.GetType().IsEnum)
            {
                arg = Convert.ChangeType(arg, arg.GetType().GetEnumUnderlyingType());
            }

            if (arg is string)
            {
                var str = (string)Convert.ChangeType(arg, typeof(string));
                SetResultString(context, str);
                return;
            }
            else if (arg is InputArgument ia)
            {
                SetResultInternal(context, ia.Value);
                return;
            }

            if (Marshal.SizeOf(arg.GetType()) <= 8)
            {
                SetResultUnsafe(context, arg);
            }
        }

        [SecurityCritical]
        internal unsafe void PushUnsafe(FxScriptContext* cxt, object arg)
        {
            *(long*)&cxt->functionData[8 * cxt->numArguments] = 0;
            Marshal.StructureToPtr(arg, new IntPtr(cxt->functionData + (8 * cxt->numArguments)), true);
        }

        [SecurityCritical]
        internal unsafe void SetResultUnsafe(FxScriptContext* cxt, object arg)
        {
            *(long*)&cxt->result[0] = 0;
            Marshal.StructureToPtr(arg, new IntPtr(cxt->result), true);
        }

        [SecurityCritical]
        internal unsafe void PushString(string str)
        {
            fixed (FxScriptContext* cxt = &m_extContext)
            {
                PushString(cxt, str);
            }
        }

        [SecurityCritical]
        internal unsafe void PushString(FxScriptContext* cxt, string str)
        {
            var ptr = IntPtr.Zero;

            if (str != null)
            {
                var b = Encoding.UTF8.GetBytes(str);
                ptr = Marshal.AllocHGlobal(b.Length + 1);
                Marshal.Copy(b, 0, ptr, b.Length);
                Marshal.WriteByte(ptr, b.Length, 0);

                ms_finalizers.Enqueue(() => Free(ptr));
            }

            *(IntPtr*)&cxt->functionData[8 * cxt->numArguments] = ptr;
            cxt->numArguments++;
        }

        [SecurityCritical]
        internal unsafe void SetResultString(FxScriptContext* cxt, string str)
        {
            var ptr = IntPtr.Zero;

            if (str != null)
            {
                var b = Encoding.UTF8.GetBytes(str);
                ptr = Marshal.AllocHGlobal(b.Length + 1);
                Marshal.Copy(b, 0, ptr, b.Length);
                Marshal.WriteByte(ptr, b.Length, 0);

                ms_finalizers.Enqueue(() => Free(ptr));
            }

            *(IntPtr*)&cxt->result[8] = ptr;
        }

        [SecuritySafeCritical]
        private static void Free(IntPtr ptr)
        {
            Marshal.FreeHGlobal(ptr);
        }

        [SecuritySafeCritical]
        public T GetArgument<T>(int index)
        {
            return (T)GetArgument(typeof(T), index);
        }

        [SecuritySafeCritical]
        public object GetArgument(Type type, int index)
        {
            return GetArgumentHelper(type, index);
        }

        [SecurityCritical]
        internal unsafe object GetArgument(FxScriptContext* cxt, Type type, int index)
        {
            return GetArgumentHelper(cxt, type, index);
        }

        [SecurityCritical]
        private unsafe object GetArgumentHelper(Type type, int index)
        {
            fixed (FxScriptContext* cxt = &m_extContext)
            {
                return GetArgumentHelper(cxt, type, index);
            }
        }

        [SecurityCritical]
        private unsafe object GetArgumentHelper(FxScriptContext* context, Type type, int index)
        {
            return GetResult(type, &context->functionData[index * 8]);
        }

        [SecuritySafeCritical]
        public T GetResult<T>()
        {
            return (T)GetResult(typeof(T));
        }

        [SecuritySafeCritical]
        public object GetResult(Type type)
        {
            return GetResultHelper(type);
        }

        [SecurityCritical]
        internal unsafe object GetResult(FxScriptContext* cxt, Type type)
        {
            return GetResultHelper(cxt, type);
        }

        [SecurityCritical]
        private unsafe object GetResultHelper(Type type)
        {
            fixed (FxScriptContext* cxt = &m_extContext)
            {
                return GetResultHelper(cxt, type);
            }
        }

        [SecurityCritical]
        private unsafe object GetResultHelper(FxScriptContext* context, Type type)
        {
            return GetResult(type, &context->result[0]);
        }

        [SecurityCritical]
        internal unsafe object GetResult(Type type, byte* ptr)
        {
            if (type == typeof(M128) || type == typeof(M256) || type == typeof(M512))
            {
                var dataPtr = *(IntPtr*)&ptr[0];
                if (dataPtr == IntPtr.Zero)
                {
                    return Activator.CreateInstance(type)!;
                }

                return Marshal.PtrToStructure(dataPtr, type)!;
            }

            if (type == typeof(string))
            {
                var nativeUtf8 = *(IntPtr*)&ptr[0];
                if (nativeUtf8 == IntPtr.Zero)
                {
                    return null!;
                }

                var len = 0;
                while (Marshal.ReadByte(nativeUtf8, len) != 0)
                {
                    ++len;
                }

                var buffer = new byte[len];
                Marshal.Copy(nativeUtf8, buffer, 0, buffer.Length);
                return Encoding.UTF8.GetString(buffer);
            }

            if (typeof(NativeObject).IsAssignableFrom(type))
            {
                var pointer = (IntPtr)GetResult(typeof(IntPtr), ptr);
                return Activator.CreateInstance(type, pointer)!;
            }

            if (type == typeof(Color))
            {
                var pointer = (IntPtr)GetResult(typeof(IntPtr), ptr);
                return Marshaling.ColorMarshaler.NativeToManaged(pointer);
            }

            // this one only works if the 'Raw'/uint is passed
            // maybe do this with a marshaler?!
            if (type == typeof(CEntityHandle))
            {
                return new CEntityHandle((uint)GetResult(typeof(uint), ptr));
            }

            if (type == typeof(object))
            {
                // var dataPtr = *(IntPtr*)&ptr[0];
                // var dataLength = *(long*)&ptr[8];
                //
                // byte[] data = new byte[dataLength];
                // Marshal.Copy(dataPtr, data, 0, (int)dataLength);

                return null!;
                //return MsgPackDeserializer.Deserialize(data);
            }

            if (type.IsEnum)
            {
                return Enum.ToObject(type, GetResult(type.GetEnumUnderlyingType(), ptr));
            }

            if (Marshal.SizeOf(type) <= 8)
            {
                return GetResultInternal(type, ptr);
            }

            return null!;
        }

        [SecurityCritical]
        private static unsafe object GetResultInternal(Type type, byte* ptr)
        {
            return Marshal.PtrToStructure(new IntPtr(ptr), type)!;
        }


        [SecurityCritical]
        internal unsafe string ErrorHandler(byte* error)
        {
            if (error != null)
            {
                var errorStart = error;
                int length = 0;

                for (var p = errorStart; *p != 0; p++)
                {
                    length++;
                }

                return Encoding.UTF8.GetString(errorStart, length);
            }

            return "Native invocation failed.";
        }

        internal void GlobalCleanUp()
        {
            lock (ms_lock)
            {
                while (ms_finalizers.TryDequeue(out var cb))
                {
                    cb();
                }
            }
        }

        public override string ToString()
        {
            return $"ScriptContext{{numArgs={m_extContext.numArguments}}}";
        }
    }
}
