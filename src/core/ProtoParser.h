#pragma once

#include <networksystem/inetworkmessages.h>
#include "core/globals.h"
#include <stdio.h>
#include "networkbasetypes.pb.h"

using namespace google;

namespace counterstrikesharp {

class ProtoParser
{
  private:
    protobuf::Message* msg;
    bool ownsMessage;

  public:
    ProtoParser(void* protoAddress);
    ~ProtoParser();

    inline bool HasField(const char* pszFieldName)
    {
        const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
        if (!field)
        {
            return false;
        }

        if (field->label() == protobuf::FieldDescriptor::LABEL_REPEATED)
        {
            return msg->GetReflection()->FieldSize(*msg, field) > 0;
        }

        return msg->GetReflection()->HasField(*msg, field);
    }

    inline bool GetInt32(const char* pszFieldName, int32* out)
    {
        // 检查输入参数
        if (out == nullptr)
        {
            printf("[DEBUG] GetInt32: out指针为空\n");
            return false;
        }

        // 初始化默认返回值
        *out = 0;

        // 增强型msg指针检查
        if (msg == nullptr)
        {
            printf("[DEBUG] GetInt32: msg指针为空\n");
            return false;
        }

        // 以下代码在安全模式下不会执行
        // 验证msg指针是否有效
        try
        {
            volatile char testByte = *(reinterpret_cast<char*>(msg));
            (void)testByte; // 防止编译器优化掉
        }
        catch (...)
        {
            printf("[DEBUG] GetInt32: msg指针无效或无法访问\n");
            return false;
        }

        if (pszFieldName == nullptr)
        {
            printf("[DEBUG] GetInt32: 字段名为空\n");
            return false;
        }

        printf("[DEBUG] GetInt32: 开始获取字段 %s，msg指针=%p\n", pszFieldName, static_cast<const void*>(msg));

        try
        {
            // 尝试获取消息类型名，如果失败则可能是无效指针
            std::string typeName;
            try
            {
                typeName = msg->GetTypeName();
                printf("[DEBUG] GetInt32: 消息类型名=%s\n", typeName.c_str());
            }
            catch (...)
            {
                printf("[DEBUG] GetInt32: 无法获取消息类型名，可能是无效指针\n");
                return false;
            }

            // 先检查描述符
            const protobuf::Descriptor* descriptor = nullptr;
            try
            {
                descriptor = msg->GetDescriptor();
                if (descriptor == nullptr)
                {
                    printf("[DEBUG] GetInt32: 消息描述符为空\n");
                    return false;
                }
            }
            catch (const std::exception& e)
            {
                printf("[DEBUG] GetInt32: 获取描述符异常 - %s\n", e.what());
                return false;
            }
            catch (...)
            {
                printf("[DEBUG] GetInt32: 获取描述符未知异常\n");
                return false;
            }

            // 验证描述符有效性
            try
            {
                std::string descName = descriptor->name();
                printf("[DEBUG] GetInt32: 描述符名称=%s\n", descName.c_str());
            }
            catch (...)
            {
                printf("[DEBUG] GetInt32: 描述符无效或无法访问\n");
                return false;
            }

            // 查找字段
            const protobuf::FieldDescriptor* field = nullptr;
            try
            {
                field = descriptor->FindFieldByName(pszFieldName);
            }
            catch (const std::exception& e)
            {
                printf("[DEBUG] GetInt32: 查找字段异常 - %s\n", e.what());
                return false;
            }
            catch (...)
            {
                printf("[DEBUG] GetInt32: 查找字段未知异常\n");
                return false;
            }

            if (!field)
            {
                printf("[DEBUG] GetInt32: 找不到字段 %s\n", pszFieldName);
                return false;
            }
            printf("[DEBUG] GetInt32: 找到字段 %s, cpp类型=%d\n", pszFieldName, (int)field->cpp_type());

            // 检查字段类型
            if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_INT32 &&
                field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_ENUM)
            {
                printf("[DEBUG] GetInt32: 字段类型不是INT32或ENUM，实际类型=%d\n", (int)field->cpp_type());
                return false;
            }

            // 检查是否为重复字段
            if (field->label() == protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                printf("[DEBUG] GetInt32: 字段是重复类型，不支持此方法访问\n");
                return false;
            }

            // 获取反射器
            const protobuf::Reflection* reflection = nullptr;
            try
            {
                reflection = msg->GetReflection();
                if (reflection == nullptr)
                {
                    printf("[DEBUG] GetInt32: 消息反射器为空\n");
                    return false;
                }
            }
            catch (const std::exception& e)
            {
                printf("[DEBUG] GetInt32: 获取反射器异常 - %s\n", e.what());
                return false;
            }
            catch (...)
            {
                printf("[DEBUG] GetInt32: 获取反射器未知异常\n");
                return false;
            }

            // 验证反射器有效性
            try
            {
                bool hasField = reflection->HasField(*msg, field);
                printf("[DEBUG] GetInt32: 字段是否存在=%s\n", hasField ? "是" : "否");
                if (!hasField)
                {
                    printf("[DEBUG] GetInt32: 字段未设置，使用默认值0\n");
                    return true; // 返回默认值0
                }
            }
            catch (...)
            {
                printf("[DEBUG] GetInt32: 检查字段存在性失败，反射器可能无效\n");
                return false;
            }

            // 获取字段值
            try
            {
                printf("[DEBUG] GetInt32: 尝试获取字段值\n");
                if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_INT32)
                {
                    *out = reflection->GetInt32(*msg, field);
                    printf("[DEBUG] GetInt32: 成功获取INT32字段值: %d\n", *out);
                }
                else // CPPTYPE_ENUM
                {
                    *out = reflection->GetEnumValue(*msg, field);
                    printf("[DEBUG] GetInt32: 成功获取ENUM字段值: %d\n", *out);
                }
                return true;
            }
            catch (const std::exception& e)
            {
                printf("[DEBUG] GetInt32: 获取字段值异常 - %s\n", e.what());
                return false;
            }
            catch (...)
            {
                printf("[DEBUG] GetInt32: 获取字段值未知异常\n");
                return false;
            }
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] GetInt32: 整体异常 - %s\n", e.what());
            return false;
        }
        catch (...)
        {
            printf("[DEBUG] GetInt32: 整体未知异常\n");
            return false;
        }
    }

    inline bool GetInt64(const char* pszFieldName, int64* out)
    {
        if (msg == nullptr)
        {
            printf("[DEBUG] GetInt64: msg是空指针\n");
            return false;
        }
        printf("[DEBUG] GetInt64: 开始获取字段 %s\n", pszFieldName);

        try
        {
            const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
            if (!field)
            {
                printf("[DEBUG] GetInt64: 找不到字段 %s\n", pszFieldName);
                return false;
            }
            printf("[DEBUG] GetInt64: 找到字段 %s, cpp类型=%d\n", pszFieldName, (int)field->cpp_type());

            if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_INT64)
            {
                printf("[DEBUG] GetInt64: 字段类型不是INT64，实际类型=%d\n", (int)field->cpp_type());
                return false;
            }

            if (field->label() == protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                printf("[DEBUG] GetInt64: 字段是重复类型，不支持此方法访问\n");
                return false;
            }

            printf("[DEBUG] GetInt64: 尝试获取字段值\n");
            *out = msg->GetReflection()->GetInt64(*msg, field);
            printf("[DEBUG] GetInt64: 成功获取字段值 %lld\n", *out);
            return true;
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] GetInt64: 异常 - %s\n", e.what());
            return false;
        }
        catch (...)
        {
            printf("[DEBUG] GetInt64: 未知异常\n");
            return false;
        }
    }

    inline bool GetFloat(const char* pszFieldName, float* out)
    {
        if (msg == nullptr)
        {
            printf("[DEBUG] GetFloat: msg是空指针\n");
            return false;
        }
        printf("[DEBUG] GetFloat: 开始获取字段 %s\n", pszFieldName);

        try
        {
            const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
            if (!field)
            {
                printf("[DEBUG] GetFloat: 找不到字段 %s\n", pszFieldName);
                return false;
            }
            printf("[DEBUG] GetFloat: 找到字段 %s, cpp类型=%d\n", pszFieldName, field->cpp_type());

            if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_FLOAT)
            {
                printf("[DEBUG] GetFloat: 字段类型不是FLOAT，实际类型=%d\n", field->cpp_type());
                return false;
            }

            if (field->label() == protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                printf("[DEBUG] GetFloat: 字段是重复类型，不支持此方法访问\n");
                return false;
            }

            printf("[DEBUG] GetFloat: 尝试获取字段值\n");
            *out = msg->GetReflection()->GetFloat(*msg, field);
            printf("[DEBUG] GetFloat: 成功获取字段值 %f\n", *out);
            return true;
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] GetFloat: 异常 - %s\n", e.what());
            return false;
        }
        catch (...)
        {
            printf("[DEBUG] GetFloat: 未知异常\n");
            return false;
        }
    }

    inline bool GetBool(const char* pszFieldName, bool* out)
    {
        if (msg == nullptr)
        {
            printf("[DEBUG] GetBool: msg是空指针\n");
            return false;
        }
        printf("[DEBUG] GetBool: 开始获取字段 %s\n", pszFieldName);

        try
        {
            const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
            if (!field)
            {
                printf("[DEBUG] GetBool: 找不到字段 %s\n", pszFieldName);
                return false;
            }
            printf("[DEBUG] GetBool: 找到字段 %s, cpp类型=%d\n", pszFieldName, field->cpp_type());

            if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_BOOL)
            {
                printf("[DEBUG] GetBool: 字段类型不是BOOL，实际类型=%d\n", field->cpp_type());
                return false;
            }

            if (field->label() == protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                printf("[DEBUG] GetBool: 字段是重复类型，不支持此方法访问\n");
                return false;
            }

            printf("[DEBUG] GetBool: 尝试获取字段值\n");
            *out = msg->GetReflection()->GetBool(*msg, field);
            printf("[DEBUG] GetBool: 成功获取字段值 %d\n", *out);
            return true;
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] GetBool: 异常 - %s\n", e.what());
            return false;
        }
        catch (...)
        {
            printf("[DEBUG] GetBool: 未知异常\n");
            return false;
        }
    }

    inline bool GetString(const char* pszFieldName, char* out, int maxLength)
    {
        const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
        if (!field)
        {
            return false;
        }

        if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_STRING)
        {
            return false;
        }

        if (field->label() == protobuf::FieldDescriptor::LABEL_REPEATED)
        {
            return false;
        }

        std::string str = msg->GetReflection()->GetString(*msg, field);

        size_t copyLen = str.length() < static_cast<size_t>(maxLength - 1) ? str.length() : static_cast<size_t>(maxLength - 1);
        memcpy(out, str.c_str(), copyLen);
        out[copyLen] = '\0';

        return true;
    }

    inline int GetRepeatedFieldCount(const char* pszFieldName)
    {
        if (msg == nullptr)
        {
            printf("[DEBUG] GetRepeatedFieldCount: msg是空指针\n");
            return -1;
        }
        printf("[DEBUG] GetRepeatedFieldCount: 开始获取重复字段 %s 的数量\n", pszFieldName);

        try
        {
            const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
            if (!field)
            {
                printf("[DEBUG] GetRepeatedFieldCount: 找不到字段 %s\n", pszFieldName);
                return -1;
            }
            printf("[DEBUG] GetRepeatedFieldCount: 找到字段 %s\n", pszFieldName);

            if (field->label() != protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                printf("[DEBUG] GetRepeatedFieldCount: 字段不是重复类型\n");
                return -1;
            }

            int size = msg->GetReflection()->FieldSize(*msg, field);
            printf("[DEBUG] GetRepeatedFieldCount: 成功获取重复字段大小 %d\n", size);
            return size;
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] GetRepeatedFieldCount: 异常 - %s\n", e.what());
            return -1;
        }
        catch (...)
        {
            printf("[DEBUG] GetRepeatedFieldCount: 未知异常\n");
            return -1;
        }
    }

    inline bool GetRepeatedInt32(const char* pszFieldName, int index, int32* out)
    {
        if (msg == nullptr)
        {
            printf("[DEBUG] GetRepeatedInt32: msg是空指针\n");
            return false;
        }
        printf("[DEBUG] GetRepeatedInt32: 开始获取重复字段 %s 索引 %d\n", pszFieldName, index);

        try
        {
            const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
            if (!field)
            {
                printf("[DEBUG] GetRepeatedInt32: 找不到字段 %s\n", pszFieldName);
                return false;
            }
            printf("[DEBUG] GetRepeatedInt32: 找到字段 %s, cpp类型=%d\n", pszFieldName, field->cpp_type());

            if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_INT32 &&
                field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_ENUM)
            {
                printf("[DEBUG] GetRepeatedInt32: 字段类型不是INT32或ENUM，实际类型=%d\n", field->cpp_type());
                return false;
            }

            if (field->label() != protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                printf("[DEBUG] GetRepeatedInt32: 字段不是重复类型\n");
                return false;
            }

            auto& reflection = *msg->GetReflection();
            int fieldSize = reflection.FieldSize(*msg, field);
            printf("[DEBUG] GetRepeatedInt32: 字段大小 = %d\n", fieldSize);

            if (index < 0 || index >= fieldSize)
            {
                printf("[DEBUG] GetRepeatedInt32: 索引越界 %d >= %d\n", index, fieldSize);
                return false;
            }

            *out = reflection.GetRepeatedInt32(*msg, field, index);
            printf("[DEBUG] GetRepeatedInt32: 成功获取字段值 %d\n", *out);
            return true;
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] GetRepeatedInt32: 异常 - %s\n", e.what());
            return false;
        }
        catch (...)
        {
            printf("[DEBUG] GetRepeatedInt32: 未知异常\n");
            return false;
        }
    }

    inline bool GetRepeatedInt64(const char* pszFieldName, int index, int64* out)
    {
        if (msg == nullptr)
        {
            printf("[DEBUG] GetRepeatedInt64: msg是空指针\n");
            return false;
        }
        printf("[DEBUG] GetRepeatedInt64: 开始获取重复字段 %s 索引 %d\n", pszFieldName, index);

        try
        {
            const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
            if (!field)
            {
                printf("[DEBUG] GetRepeatedInt64: 找不到字段 %s\n", pszFieldName);
                return false;
            }
            printf("[DEBUG] GetRepeatedInt64: 找到字段 %s, cpp类型=%d\n", pszFieldName, field->cpp_type());

            if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_INT64)
            {
                printf("[DEBUG] GetRepeatedInt64: 字段类型不是INT64，实际类型=%d\n", field->cpp_type());
                return false;
            }

            if (field->label() != protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                printf("[DEBUG] GetRepeatedInt64: 字段不是重复类型\n");
                return false;
            }

            auto& reflection = *msg->GetReflection();
            int fieldSize = reflection.FieldSize(*msg, field);
            printf("[DEBUG] GetRepeatedInt64: 字段大小 = %d\n", fieldSize);

            if (index < 0 || index >= fieldSize)
            {
                printf("[DEBUG] GetRepeatedInt64: 索引越界 %d >= %d\n", index, fieldSize);
                return false;
            }

            *out = reflection.GetRepeatedInt64(*msg, field, index);
            printf("[DEBUG] GetRepeatedInt64: 成功获取字段值 %lld\n", *out);
            return true;
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] GetRepeatedInt64: 异常 - %s\n", e.what());
            return false;
        }
        catch (...)
        {
            printf("[DEBUG] GetRepeatedInt64: 未知异常\n");
            return false;
        }
    }

    inline bool GetRepeatedFloat(const char* pszFieldName, int index, float* out)
    {
        const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
        if (!field)
        {
            return false;
        }

        if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_FLOAT)
        {
            return false;
        }

        if (field->label() != protobuf::FieldDescriptor::LABEL_REPEATED)
        {
            return false;
        }

        int elemCount = msg->GetReflection()->FieldSize(*msg, field);
        if (elemCount == 0 || index >= elemCount || index < 0)
        {
            return false;
        }

        *out = msg->GetReflection()->GetRepeatedFloat(*msg, field, index);
        return true;
    }

    inline bool GetRepeatedBool(const char* pszFieldName, int index, bool* out)
    {
        const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
        if (!field)
        {
            return false;
        }

        if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_BOOL)
        {
            return false;
        }

        if (field->label() != protobuf::FieldDescriptor::LABEL_REPEATED)
        {
            return false;
        }

        int elemCount = msg->GetReflection()->FieldSize(*msg, field);
        if (elemCount == 0 || index >= elemCount || index < 0)
        {
            return false;
        }

        *out = msg->GetReflection()->GetRepeatedBool(*msg, field, index);
        return true;
    }

    inline bool GetRepeatedString(const char* pszFieldName, int index, char* out, int maxLength)
    {
        const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
        if (!field)
        {
            return false;
        }

        if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_STRING)
        {
            return false;
        }

        if (field->label() != protobuf::FieldDescriptor::LABEL_REPEATED)
        {
            return false;
        }

        int elemCount = msg->GetReflection()->FieldSize(*msg, field);
        if (elemCount == 0 || index >= elemCount || index < 0)
        {
            return false;
        }

        std::string str = msg->GetReflection()->GetRepeatedString(*msg, field, index);

        size_t copyLen = str.length() < static_cast<size_t>(maxLength - 1) ? str.length() : static_cast<size_t>(maxLength - 1);
        memcpy(out, str.c_str(), copyLen);
        out[copyLen] = '\0';

        return true;
    }

    inline std::string GetTypeName() { return msg->GetTypeName(); }

    inline std::string GetDebugString() { return msg->DebugString(); }

    // 用于调试目的，获取内部消息指针
    inline protobuf::Message* GetMessagePointer() const { return msg; }

    // 获取子消息
    inline bool GetMessage(const char* pszFieldName, ProtoParser** out)
    {
        if (msg == nullptr)
        {
            printf("[DEBUG] GetMessage: msg是空指针\n");
            return false;
        }
        printf("[DEBUG] GetMessage: 开始获取子消息字段 %s\n", pszFieldName);

        try
        {
            const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
            if (!field)
            {
                printf("[DEBUG] GetMessage: 找不到字段 %s\n", pszFieldName);
                return false;
            }
            printf("[DEBUG] GetMessage: 找到字段 %s, cpp类型=%d\n", pszFieldName, field->cpp_type());

            if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                printf("[DEBUG] GetMessage: 字段类型不是消息，实际类型=%d\n", field->cpp_type());
                return false;
            }

            if (field->label() == protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                printf("[DEBUG] GetMessage: 字段是重复类型，不支持此方法访问\n");
                return false;
            }

            printf("[DEBUG] GetMessage: 尝试获取子消息\n");
            protobuf::Message* submsg = msg->GetReflection()->MutableMessage(msg, field);
            printf("[DEBUG] GetMessage: 获取子消息成功，地址 = %p\n", static_cast<const void*>(submsg));

            *out = new ProtoParser(submsg);
            printf("[DEBUG] GetMessage: 创建ProtoParser成功，地址 = %p\n", static_cast<const void*>(*out));
            return true;
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] GetMessage: 异常 - %s\n", e.what());
            return false;
        }
        catch (...)
        {
            printf("[DEBUG] GetMessage: 未知异常\n");
            return false;
        }
    }

    inline bool GetRepeatedMessage(const char* pszFieldName, int index, ProtoParser** out)
    {
        if (msg == nullptr)
        {
            printf("[DEBUG] GetRepeatedMessage: msg是空指针\n");
            return false;
        }
        printf("[DEBUG] GetRepeatedMessage: 开始获取重复字段 %s 索引 %d\n", pszFieldName, index);

        try
        {
            const protobuf::FieldDescriptor* field = msg->GetDescriptor()->FindFieldByName(pszFieldName);
            if (!field)
            {
                printf("[DEBUG] GetRepeatedMessage: 找不到字段 %s\n", pszFieldName);
                return false;
            }
            printf("[DEBUG] GetRepeatedMessage: 找到字段 %s, cpp类型=%d\n", pszFieldName, field->cpp_type());

            if (field->cpp_type() != protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                printf("[DEBUG] GetRepeatedMessage: 字段类型不是消息，实际类型=%d\n", field->cpp_type());
                return false;
            }

            if (field->label() != protobuf::FieldDescriptor::LABEL_REPEATED)
            {
                printf("[DEBUG] GetRepeatedMessage: 字段不是重复类型\n");
                return false;
            }

            printf("[DEBUG] GetRepeatedMessage: 查询元素数量\n");
            int elemCount = msg->GetReflection()->FieldSize(*msg, field);
            printf("[DEBUG] GetRepeatedMessage: 元素数量 = %d\n", elemCount);

            if (elemCount == 0 || index >= elemCount || index < 0)
            {
                printf("[DEBUG] GetRepeatedMessage: 索引超出范围，索引 = %d, 总元素 = %d\n", index, elemCount);
                return false;
            }

            printf("[DEBUG] GetRepeatedMessage: 尝试获取重复消息\n");
            protobuf::Message* submsg = msg->GetReflection()->MutableRepeatedMessage(msg, field, index);
            printf("[DEBUG] GetRepeatedMessage: 获取重复消息成功，地址 = %p\n", static_cast<const void*>(submsg));

            *out = new ProtoParser(submsg);
            printf("[DEBUG] GetRepeatedMessage: 创建ProtoParser成功，地址 = %p\n", static_cast<const void*>(*out));
            return true;
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] GetRepeatedMessage: 异常 - %s\n", e.what());
            return false;
        }
        catch (...)
        {
            printf("[DEBUG] GetRepeatedMessage: 未知异常\n");
            return false;
        }
    }
};

extern std::vector<ProtoParser*> managed_proto_parsers;

} // namespace counterstrikesharp
