#include "ProtoParser.h"

namespace counterstrikesharp {

std::vector<ProtoParser*> managed_proto_parsers;

ProtoParser::ProtoParser(void* protoAddress)
{
    // 狗屎编码问题，懒得改了，直接调控制台编码得了
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    printf("[DEBUG] ProtoParser::ProtoParser: 传入地址 = %p\n", static_cast<const void*>(protoAddress));

    if (protoAddress == nullptr)
    {
        printf("[DEBUG] ProtoParser::ProtoParser: 传入地址为空\n");
        msg = nullptr;
        ownsMessage = false;
        return;
    }

    msg = static_cast<protobuf::Message*>(protoAddress);
    printf("[DEBUG] ProtoParser::ProtoParser: 转换后的msg指针 = %p\n", static_cast<const void*>(msg));

    try
    {
        if (msg != nullptr)
        {
            printf("[DEBUG] ProtoParser::ProtoParser: 消息类型名 = %s\n", msg->GetTypeName().c_str());
        }
    }
    catch (const std::exception& e)
    {
        printf("[DEBUG] ProtoParser::ProtoParser: 获取类型名异常 - %s\n", e.what());
    }
    catch (...)
    {
        printf("[DEBUG] ProtoParser::ProtoParser: 获取类型名未知异常\n");
    }

    ownsMessage = false; // 未拥有这个消息，不要在析构函数中删除它
}

ProtoParser::~ProtoParser()
{
    if (ownsMessage && msg != nullptr)
    {
        delete msg;
    }
    msg = nullptr;
}

} // namespace counterstrikesharp
