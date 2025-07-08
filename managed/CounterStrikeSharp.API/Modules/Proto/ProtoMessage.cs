using System;
using System.Runtime.InteropServices;
using CounterStrikeSharp.API.Modules.Utils;

namespace CounterStrikeSharp.API.Modules.Proto;

public class ProtoMessage : NativeObject, IDisposable
{
    public ProtoMessage(IntPtr pointer) : base(pointer) { }

    public static ProtoMessage FromAddress(IntPtr address) => new(NativeAPI.ProtoFromAddress(address));

    public bool HasField(string fieldName) => NativeAPI.ProtoHasfield(Handle, fieldName);

    public int ReadInt(string fieldName, int? index = null) => NativeAPI.ProtoReadint(Handle, fieldName, index ?? -1);
    public uint ReadUInt(string fieldName, int? index = null) => (uint)NativeAPI.ProtoReadint(Handle, fieldName, index ?? -1);
    public long ReadInt64(string fieldName, int? index = null) => Convert.ToInt64(NativeAPI.ProtoReadint64(Handle, fieldName, index ?? -1));
    public ulong ReadUInt64(string fieldName, int? index = null) => Convert.ToUInt64(NativeAPI.ProtoReadint64(Handle, fieldName, index ?? -1));
    public float ReadFloat(string fieldName, int? index = null) => NativeAPI.ProtoReadfloat(Handle, fieldName, index ?? -1);
    public double ReadDouble(string fieldName, int? index = null) => NativeAPI.ProtoReadfloat(Handle, fieldName, index ?? -1);
    
    public string ReadString(string fieldName, int? index = null)
    {
        int bufferSize = 1024;
        IntPtr bufferPtr = Marshal.AllocHGlobal(bufferSize);
        
        try
        {
            NativeAPI.ProtoReadstring(Handle, fieldName, bufferSize, bufferPtr, index ?? -1);
            string result = Marshal.PtrToStringUTF8(bufferPtr) ?? string.Empty;
            return result;
        }
        finally
        {
            Marshal.FreeHGlobal(bufferPtr);
        }
    }
    
    public bool ReadBool(string fieldName, int? index = null) => NativeAPI.ProtoReadbool(Handle, fieldName, index ?? -1);

    public int GetRepeatedFieldCount(string fieldName) => NativeAPI.ProtoGetrepeatedfieldcount(Handle, fieldName);

    public ProtoMessage GetMessage(string fieldName, int? index = null) => new(NativeAPI.ProtoGetmessage(Handle, fieldName, index ?? -1));

    public string TypeName => NativeAPI.ProtoGettypename(Handle);
    
    public string DebugString => NativeAPI.ProtoGetdebugstring(Handle);

    private void ReleaseUnmanagedResources()
    {
        Server.NextFrame(() => { /* NativeAPI.ProtoDelete(Handle); */ });
    }

    public void Dispose()
    {
        ReleaseUnmanagedResources();
        GC.SuppressFinalize(this);
    }

    ~ProtoMessage()
    {
        ReleaseUnmanagedResources();
    }
}
