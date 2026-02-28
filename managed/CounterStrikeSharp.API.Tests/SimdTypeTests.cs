using System.Runtime.InteropServices;
using CounterStrikeSharp.API.Modules.Memory;

namespace CounterStrikeSharp.API.Tests;

public class SimdTypeTests
{
    [Fact]
    public void SimdStructSizesAreCorrect()
    {
        Assert.Equal(16, Marshal.SizeOf<M128>());
        Assert.Equal(32, Marshal.SizeOf<M256>());
        Assert.Equal(64, Marshal.SizeOf<M512>());
    }

    [Fact]
    public void SimdTypesMapToValidDataTypes()
    {
        Assert.Equal(DataType.DATA_TYPE_M128, typeof(M128).ToValidDataType());
        Assert.Equal(DataType.DATA_TYPE_M256, typeof(M256).ToValidDataType());
        Assert.Equal(DataType.DATA_TYPE_M512, typeof(M512).ToValidDataType());
    }
}
