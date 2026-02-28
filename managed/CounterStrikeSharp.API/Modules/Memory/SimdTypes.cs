/*
 *  This file is part of CounterStrikeSharp.
 *  CounterStrikeSharp is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CounterStrikeSharp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CounterStrikeSharp.  If not, see <https://www.gnu.org/licenses/>. *
 */

using System.Runtime.InteropServices;

namespace CounterStrikeSharp.API.Modules.Memory
{
    /// <summary>
    /// 128-bit SIMD value (16 bytes).
    /// </summary>
    /// <remarks>
    /// Used by the dynamic function &amp; hook system as a raw 16-byte payload.
    /// On Windows, SIMD invocation is emitted as <c>__vectorcall</c> by default when any SIMD types are present.
    /// </remarks>
    [StructLayout(LayoutKind.Sequential)]
    public struct M128
    {
        public float X0;
        public float X1;
        public float X2;
        public float X3;
    }

    /// <summary>
    /// 256-bit SIMD value (32 bytes).
    /// </summary>
    /// <remarks>
    /// Used by the dynamic function &amp; hook system as a raw 32-byte payload.
    /// Requires AVX to be meaningful on the native side.
    /// </remarks>
    [StructLayout(LayoutKind.Sequential)]
    public struct M256
    {
        public float X0;
        public float X1;
        public float X2;
        public float X3;
        public float X4;
        public float X5;
        public float X6;
        public float X7;
    }

    /// <summary>
    /// 512-bit SIMD value (64 bytes).
    /// </summary>
    /// <remarks>
    /// Used by the dynamic function &amp; hook system as a raw 64-byte payload.
    /// Requires AVX-512 support (at least AVX512F) to be meaningful on the native side.
    /// </remarks>
    [StructLayout(LayoutKind.Sequential)]
    public struct M512
    {
        public float X0;
        public float X1;
        public float X2;
        public float X3;
        public float X4;
        public float X5;
        public float X6;
        public float X7;
        public float X8;
        public float X9;
        public float X10;
        public float X11;
        public float X12;
        public float X13;
        public float X14;
        public float X15;
    }
}
