#include "core/ProtoParser.h"
#include "core/globals.h"
#include <stdio.h>
#include "scripting/autonative.h"

namespace counterstrikesharp {

#define GET_PARSER_OR_ERR()                                     \
    auto parser = scriptContext.GetArgument<ProtoParser*>(0);   \
    if (parser == nullptr)                                      \
    {                                                           \
        scriptContext.ThrowNativeError("Invalid proto parser"); \
        return;                                                 \
    }

#define GET_FIELD_NAME_OR_ERR() const char* fieldName = scriptContext.GetArgument<const char*>(1);

static void ProtoParserFromAddress(ScriptContext& scriptContext)
{
    // 狗屎编码问题，懒得改了，直接调控制台编码得了
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    printf("[DEBUG] ProtoParserFromAddress: 开始执行\n");

    auto protoAddress = scriptContext.GetArgument<void*>(0);
    printf("[DEBUG] ProtoParserFromAddress: 传入地址 = %p\n", protoAddress);

    if (protoAddress == nullptr)
    {
        printf("[DEBUG] ProtoParserFromAddress: 传入地址为空\n");
        scriptContext.ThrowNativeError("Invalid proto address");
        return;
    }

    for (auto& existing_parser : managed_proto_parsers)
    {
        if (existing_parser->GetMessagePointer() == protoAddress)
        {
            printf("[DEBUG] ProtoParserFromAddress: 找到已存在的parser，地址 = %p\n", static_cast<void*>(existing_parser));
            scriptContext.SetResult(existing_parser);
            printf("[DEBUG] ProtoParserFromAddress: 成功设置返回值\n");
            return;
        }
    }

    try
    {
        printf("[DEBUG] ProtoParserFromAddress: 创建ProtoParser对象\n");
        auto parser = new ProtoParser(protoAddress);
        printf("[DEBUG] ProtoParserFromAddress: ProtoParser对象创建成功，地址 = %p\n", static_cast<void*>(parser));

        managed_proto_parsers.push_back(parser);
        printf("[DEBUG] ProtoParserFromAddress: 添加到managed_proto_parsers，当前数量 = %zu\n", managed_proto_parsers.size());

        scriptContext.SetResult(parser);
        printf("[DEBUG] ProtoParserFromAddress: 成功设置返回值\n");
    }
    catch (const std::exception& e)
    {
        printf("[DEBUG] ProtoParserFromAddress: 异常 - %s\n", e.what());
        scriptContext.ThrowNativeError("Exception when creating ProtoParser: %s", e.what());
        return;
    }
    catch (...)
    {
        printf("[DEBUG] ProtoParserFromAddress: 未知异常\n");
        scriptContext.ThrowNativeError("Unknown exception when creating ProtoParser");
        return;
    }
}

static void ProtoParserDelete(ScriptContext& scriptContext)
{
    printf("[DEBUG] ProtoParserDelete: 开始执行\n");

    auto parser = scriptContext.GetArgument<ProtoParser*>(0);
    printf("[DEBUG] ProtoParserDelete: 尝试删除parser = %p\n", static_cast<void*>(parser));

    if (parser == nullptr)
    {
        printf("[DEBUG] ProtoParserDelete: 传入的parser为空\n");
        scriptContext.ThrowNativeError("Invalid proto parser");
        return;
    }

    try
    {
        printf("[DEBUG] ProtoParserDelete: 在managed_proto_parsers中查找parser，当前数量=%zu\n", managed_proto_parsers.size());
        auto it = std::find(managed_proto_parsers.begin(), managed_proto_parsers.end(), parser);
        if (it == managed_proto_parsers.end())
        {
            printf("[DEBUG] ProtoParserDelete: parser不在管理列表中\n");
            scriptContext.ThrowNativeError("Proto parser is not managed");
            return;
        }

        printf("[DEBUG] ProtoParserDelete: 成功找到parser，从列表中移除\n");
        managed_proto_parsers.erase(it);
        printf("[DEBUG] ProtoParserDelete: 删除parser对象\n");
        delete parser;
        printf("[DEBUG] ProtoParserDelete: 删除成功，当前管理列表数量=%zu\n", managed_proto_parsers.size());
        scriptContext.SetResult(true);
    }
    catch (const std::exception& e)
    {
        printf("[DEBUG] ProtoParserDelete: 异常 - %s\n", e.what());
        scriptContext.ThrowNativeError("Exception when deleting ProtoParser: %s", e.what());
    }
    catch (...)
    {
        printf("[DEBUG] ProtoParserDelete: 未知异常\n");
        scriptContext.ThrowNativeError("Unknown exception when deleting ProtoParser");
    }
}

static void ProtoParserReadInt(ScriptContext& scriptContext)
{
    try
    {
        printf("[DEBUG] ProtoParserReadInt: 开始执行\n");

        // 检查参数数量
        if (scriptContext.GetArgumentCount() < 3)
        {
            printf("[DEBUG] ProtoParserReadInt: 参数数量不足，需要至少3个参数\n");
            scriptContext.ThrowNativeError("Expected at least 3 arguments");
            return;
        }

        // 检查parser参数
        ProtoParser* parser = nullptr;
        try
        {
            parser = scriptContext.GetArgument<ProtoParser*>(0);
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取parser参数异常 - %s\n", e.what());
            scriptContext.ThrowNativeError("Failed to get parser argument: %s", e.what());
            return;
        }
        catch (...)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取parser参数未知异常\n");
            scriptContext.ThrowNativeError("Unknown exception when getting parser argument");
            return;
        }

        if (parser == nullptr)
        {
            printf("[DEBUG] ProtoParserReadInt: parser为空\n");
            scriptContext.ThrowNativeError("Invalid proto parser (null)");
            return;
        }
        printf("[DEBUG] ProtoParserReadInt: parser地址 = %p\n", static_cast<void*>(parser));

        // 检查fieldName参数
        const char* fieldName = nullptr;
        try
        {
            fieldName = scriptContext.GetArgument<const char*>(1);
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取fieldName参数异常 - %s\n", e.what());
            scriptContext.ThrowNativeError("Failed to get fieldName argument: %s", e.what());
            return;
        }
        catch (...)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取fieldName参数未知异常\n");
            scriptContext.ThrowNativeError("Unknown exception when getting fieldName argument");
            return;
        }

        if (fieldName == nullptr)
        {
            printf("[DEBUG] ProtoParserReadInt: fieldName为空\n");
            scriptContext.ThrowNativeError("Invalid field name (null)");
            return;
        }
        printf("[DEBUG] ProtoParserReadInt: fieldName = %s\n", fieldName);

        // 检查index参数
        int index = 0;
        try
        {
            index = scriptContext.GetArgument<int>(2);
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取index参数异常 - %s\n", e.what());
            scriptContext.ThrowNativeError("Failed to get index argument: %s", e.what());
            return;
        }
        catch (...)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取index参数未知异常\n");
            scriptContext.ThrowNativeError("Unknown exception when getting index argument");
            return;
        }

        printf("[DEBUG] ProtoParserReadInt: index = %d\n", index);

        // 检查消息指针
        protobuf::Message* msg = nullptr;
        try
        {
            msg = parser->GetMessagePointer();
            if (msg == nullptr)
            {
                printf("[DEBUG] ProtoParserReadInt: 消息指针为空\n");
                scriptContext.SetResult(0); // 安全返回默认值
                return;
            }
            printf("[DEBUG] ProtoParserReadInt: msg地址 = %p\n", static_cast<void*>(msg));
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取消息指针异常 - %s\n", e.what());
            scriptContext.SetResult(0); // 安全返回默认值
            return;
        }
        catch (...)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取消息指针未知异常\n");
            scriptContext.SetResult(0); // 安全返回默认值
            return;
        }

        // 获取字段值
        int32 returnValue = 0;
        bool success = false;

        try
        {
            if (index < 0)
            {
                printf("[DEBUG] ProtoParserReadInt: 尝试获取非重复字段\n");
                success = parser->GetInt32(fieldName, &returnValue);
            }
            else
            {
                printf("[DEBUG] ProtoParserReadInt: 尝试获取重复字段[%d]\n", index);
                success = parser->GetRepeatedInt32(fieldName, index, &returnValue);
            }

            if (!success)
            {
                printf("[DEBUG] ProtoParserReadInt: 获取字段值失败，返回默认值0\n");
                returnValue = 0; // 使用默认值
            }
            else
            {
                printf("[DEBUG] ProtoParserReadInt: 获取字段值成功: %d\n", returnValue);
            }
        }
        catch (const std::exception& e)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取字段值异常 - %s，返回默认值0\n", e.what());
            returnValue = 0; // 使用默认值
        }
        catch (...)
        {
            printf("[DEBUG] ProtoParserReadInt: 获取字段值未知异常，返回默认值0\n");
            returnValue = 0; // 使用默认值
        }

        // 设置返回值
        printf("[DEBUG] ProtoParserReadInt: 返回值 = %d\n", returnValue);
        try
        {
            scriptContext.SetResult(returnValue);
            printf("[DEBUG] ProtoParserReadInt: 成功设置返回值\n");
        }
        catch (...)
        {
            // 如果连设置返回值都失败了，那就只能尝试设置一个0值
            try
            {
                scriptContext.SetResult(0);
                printf("[DEBUG] ProtoParserReadInt: 设置默认返回值0\n");
            }
            catch (...)
            {
                printf("[DEBUG] ProtoParserReadInt: 无法设置任何返回值，可能会崩溃\n");
                // 已经尽力了
            }
        }
    }
    catch (...)
    {
        printf("[DEBUG] ProtoParserReadInt: 整体函数未知异常，尝试设置默认值0\n");
        // 尝试在最外层异常中设置默认返回值
        try
        {
            scriptContext.SetResult(0);
        }
        catch (...)
        {
            // 无能为力了。。。
        }
    }
}

static void ProtoParserReadInt64(ScriptContext& scriptContext)
{
    printf("[DEBUG] ProtoParserReadInt64: 开始执行\n");

    auto parser = scriptContext.GetArgument<ProtoParser*>(0);
    if (parser == nullptr)
    {
        printf("[DEBUG] ProtoParserReadInt64: parser为空\n");
        scriptContext.ThrowNativeError("Invalid proto parser");
        return;
    }
    printf("[DEBUG] ProtoParserReadInt64: parser地址 = %p\n", static_cast<void*>(parser));

    const char* fieldName = scriptContext.GetArgument<const char*>(1);
    if (fieldName == nullptr)
    {
        printf("[DEBUG] ProtoParserReadInt64: fieldName为空\n");
        scriptContext.ThrowNativeError("Invalid field name");
        return;
    }
    printf("[DEBUG] ProtoParserReadInt64: fieldName = %s\n", fieldName);

    int64 returnValue = 0;
    auto index = scriptContext.GetArgument<int>(2);
    printf("[DEBUG] ProtoParserReadInt64: index = %d\n", index);

    printf("[DEBUG] ProtoParserReadInt64: msg地址 = %p\n", static_cast<void*>(parser->GetMessagePointer()));

    if (index < 0)
    {
        printf("[DEBUG] ProtoParserReadInt64: 尝试获取非重复字段\n");
        if (!parser->GetInt64(fieldName, &returnValue))
        {
            printf("[DEBUG] ProtoParserReadInt64: 获取字段值失败: %s\n", fieldName);
            scriptContext.ThrowNativeError("Invalid field \"%s\" for proto", fieldName);
            return;
        }
        printf("[DEBUG] ProtoParserReadInt64: 获取字段值成功: %lld\n", returnValue);
    }
    else
    {
        printf("[DEBUG] ProtoParserReadInt64: 尝试获取重复字段[%d]\n", index);
        if (!parser->GetRepeatedInt64(fieldName, index, &returnValue))
        {
            printf("[DEBUG] ProtoParserReadInt64: 获取重复字段值失败: %s[%d]\n", fieldName, index);
            scriptContext.ThrowNativeError("Invalid field \"%s\"[%d] for proto", fieldName, index);
            return;
        }
        printf("[DEBUG] ProtoParserReadInt64: 获取重复字段值成功: %lld\n", returnValue);
    }

    printf("[DEBUG] ProtoParserReadInt64: 设置返回值: %lld\n", returnValue);
    printf("[DEBUG] ProtoParserReadInt64: 设置返回值: %lld\n", returnValue);
    scriptContext.SetResult(returnValue);
}

static void ProtoParserReadFloat(ScriptContext& scriptContext)
{
    GET_PARSER_OR_ERR();
    GET_FIELD_NAME_OR_ERR();

    float returnValue;
    auto index = scriptContext.GetArgument<int>(2);

    if (index < 0)
    {
        if (!parser->GetFloat(fieldName, &returnValue))
        {
            scriptContext.ThrowNativeError("Invalid field \"%s\" for proto", fieldName);
            return;
        }
    }
    else
    {
        if (!parser->GetRepeatedFloat(fieldName, index, &returnValue))
        {
            scriptContext.ThrowNativeError("Invalid field \"%s\"[%d] for proto", fieldName, index);
            return;
        }
    }

    scriptContext.SetResult(returnValue);
}

static void ProtoParserReadBool(ScriptContext& scriptContext)
{
    GET_PARSER_OR_ERR();
    GET_FIELD_NAME_OR_ERR();

    bool returnValue;
    auto index = scriptContext.GetArgument<int>(2);

    if (index < 0)
    {
        if (!parser->GetBool(fieldName, &returnValue))
        {
            scriptContext.ThrowNativeError("Invalid field \"%s\" for proto", fieldName);
            return;
        }
    }
    else
    {
        if (!parser->GetRepeatedBool(fieldName, index, &returnValue))
        {
            scriptContext.ThrowNativeError("Invalid field \"%s\"[%d] for proto", fieldName, index);
            return;
        }
    }

    scriptContext.SetResult(returnValue);
}

static void ProtoParserReadString(ScriptContext& scriptContext)
{
    GET_PARSER_OR_ERR();
    GET_FIELD_NAME_OR_ERR();

    auto bufferSize = scriptContext.GetArgument<int>(2);
    if (bufferSize <= 0)
    {
        scriptContext.ThrowNativeError("Invalid buffer size %d", bufferSize);
        return;
    }

    auto buffer = scriptContext.GetArgument<char*>(3);
    if (buffer == nullptr)
    {
        scriptContext.ThrowNativeError("Invalid buffer");
        return;
    }

    auto index = scriptContext.GetArgument<int>(4);

    if (index < 0)
    {
        if (!parser->GetString(fieldName, buffer, bufferSize))
        {
            scriptContext.ThrowNativeError("Invalid field \"%s\" for proto", fieldName);
            return;
        }
    }
    else
    {
        if (!parser->GetRepeatedString(fieldName, index, buffer, bufferSize))
        {
            scriptContext.ThrowNativeError("Invalid field \"%s\"[%d] for proto", fieldName, index);
            return;
        }
    }
}

static void ProtoParserHasField(ScriptContext& scriptContext)
{
    GET_PARSER_OR_ERR();
    GET_FIELD_NAME_OR_ERR();

    scriptContext.SetResult(parser->HasField(fieldName));
}

static void ProtoParserGetRepeatedFieldCount(ScriptContext& scriptContext)
{
    GET_PARSER_OR_ERR();
    GET_FIELD_NAME_OR_ERR();

    int count = parser->GetRepeatedFieldCount(fieldName);
    if (count < 0)
    {
        scriptContext.ThrowNativeError("Invalid field \"%s\" for proto", fieldName);
        return;
    }

    scriptContext.SetResult(count);
}

static void ProtoParserGetDebugString(ScriptContext& scriptContext)
{
    GET_PARSER_OR_ERR();

    scriptContext.SetResult(parser->GetDebugString().c_str());
}

static void ProtoParserGetTypeName(ScriptContext& scriptContext)
{
    GET_PARSER_OR_ERR();

    scriptContext.SetResult(parser->GetTypeName().c_str());
}

static void ProtoParserGetMessage(ScriptContext& scriptContext)
{
    GET_PARSER_OR_ERR();
    GET_FIELD_NAME_OR_ERR();

    ProtoParser* subParser = nullptr;

    auto index = scriptContext.GetArgument<int>(2);

    if (index < 0)
    {
        if (!parser->GetMessage(fieldName, &subParser))
        {
            scriptContext.ThrowNativeError("Invalid field \"%s\" for proto", fieldName);
            return;
        }
    }
    else
    {
        if (!parser->GetRepeatedMessage(fieldName, index, &subParser))
        {
            scriptContext.ThrowNativeError("Invalid field \"%s\"[%d] for proto", fieldName, index);
            return;
        }
    }

    managed_proto_parsers.push_back(subParser);
    scriptContext.SetResult(subParser);
}

REGISTER_NATIVES(protoparser, {
    ScriptEngine::RegisterNativeHandler("PROTO_FROM_ADDRESS", ProtoParserFromAddress);
    ScriptEngine::RegisterNativeHandler("PROTO_DELETE", ProtoParserDelete);
    ScriptEngine::RegisterNativeHandler("PROTO_HASFIELD", ProtoParserHasField);
    ScriptEngine::RegisterNativeHandler("PROTO_READINT", ProtoParserReadInt);
    ScriptEngine::RegisterNativeHandler("PROTO_READINT64", ProtoParserReadInt64);
    ScriptEngine::RegisterNativeHandler("PROTO_READFLOAT", ProtoParserReadFloat);
    ScriptEngine::RegisterNativeHandler("PROTO_READBOOL", ProtoParserReadBool);
    ScriptEngine::RegisterNativeHandler("PROTO_READSTRING", ProtoParserReadString);
    ScriptEngine::RegisterNativeHandler("PROTO_GETREPEATEDFIELDCOUNT", ProtoParserGetRepeatedFieldCount);
    ScriptEngine::RegisterNativeHandler("PROTO_GETDEBUGSTRING", ProtoParserGetDebugString);
    ScriptEngine::RegisterNativeHandler("PROTO_GETTYPENAME", ProtoParserGetTypeName);
    ScriptEngine::RegisterNativeHandler("PROTO_GETMESSAGE", ProtoParserGetMessage);
})

} // namespace counterstrikesharp
