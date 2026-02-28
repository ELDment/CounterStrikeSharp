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

using Microsoft.Extensions.Logging;

namespace CounterStrikeSharp.API.Modules.Memory
{
    public enum DataType
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
        DATA_TYPE_M512 = 19,
    }

    public static class DataTypeExtensions
    {
        private static readonly Dictionary<Type, DataType> types = new()
        {
            { typeof(float), DataType.DATA_TYPE_FLOAT },
            { typeof(double), DataType.DATA_TYPE_DOUBLE },
            { typeof(IntPtr), DataType.DATA_TYPE_POINTER },
            { typeof(int), DataType.DATA_TYPE_INT },
            { typeof(uint), DataType.DATA_TYPE_UINT },
            { typeof(bool), DataType.DATA_TYPE_BOOL },
            { typeof(string), DataType.DATA_TYPE_STRING },
            { typeof(long), DataType.DATA_TYPE_LONG },
            { typeof(ulong), DataType.DATA_TYPE_ULONG },
            { typeof(short), DataType.DATA_TYPE_SHORT },
            { typeof(sbyte), DataType.DATA_TYPE_UCHAR },
            { typeof(byte), DataType.DATA_TYPE_CHAR },
            { typeof(M128), DataType.DATA_TYPE_M128 },
            { typeof(M256), DataType.DATA_TYPE_M256 },
            { typeof(M512), DataType.DATA_TYPE_M512 },
        };

        public static DataType? ToDataType(this Type type)
        {
            if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                return Nullable.GetUnderlyingType(type)!.ToDataType();
            }

            if (types.TryGetValue(type, out DataType value))
            {
                return value;
            }

            if (typeof(NativeObject).IsAssignableFrom(type))
            {
                return DataType.DATA_TYPE_POINTER;
            }

            if (type.IsEnum && types.ContainsKey(Enum.GetUnderlyingType(type)))
            {
                return types[Enum.GetUnderlyingType(type)];
            }

            Application.Instance.Logger.LogWarning("Error retrieving data type for type {Type}", type.FullName);

            return null;
        }

        public static DataType ToValidDataType(this Type type)
        {
            if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                return Nullable.GetUnderlyingType(type)!.ToValidDataType();
            }

            if (types.TryGetValue(type, out DataType value))
            {
                return value;
            }

            if (typeof(NativeObject).IsAssignableFrom(type))
            {
                return DataType.DATA_TYPE_POINTER;
            }

            if (type.IsEnum && types.ContainsKey(Enum.GetUnderlyingType(type)))
            {
                return types[Enum.GetUnderlyingType(type)];
            }

            throw new NotSupportedException("Data type not supported: " + type.FullName);
        }
    }
}
