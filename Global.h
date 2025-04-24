#pragma once

#include <maya/MNodeMessage.h>

//I worked so hard to find this number
constexpr int DOCK_STATE_CHANGED_EVENT = 65528;

namespace Commands
{
constexpr char LAYOUT_UV[] = "LayoutUV";
constexpr char DELETE_SELECTION[] = "doDelete";
}

namespace OptionVars
{
//1: Off, 2: 3D, 3: UV
constexpr char SHELL_PRE_SCALING[] = "Unfold3DLayoutPreScale";
}

//FWD
class MObject;
class MPlug;

namespace wrappers
{
template<class>
struct member_function_class;

template<class T, typename Ret, typename... Args>
struct member_function_class<Ret(T::*)(Args...)>
{
    using type = T;
};

// Also handle const member functions if needed
template<class T, typename Ret, typename... Args>
struct member_function_class<Ret(T::*)(Args...) const>
{
    using type = T;
};
}

template<auto MemberFunctionPtr>
void MBasicFunction_wrapper(void* clientData)
{
    using T = typename wrappers::member_function_class<decltype(MemberFunctionPtr)>::type;
    auto target = reinterpret_cast<T*>(clientData);
    if (target) (target->*MemberFunctionPtr)();
}

template<auto MemberFunctionPtr>
void MNodeFunction_wrapper(MObject& node, void* clientData)
{
    using T = typename wrappers::member_function_class<decltype(MemberFunctionPtr)>::type;
    auto target = reinterpret_cast<T*>(clientData);
    if (target) (target->*MemberFunctionPtr)(node);
}

template <auto MemberFunctionPtr>
void MAttr2PlugFunction_wrapper(MNodeMessage::AttributeMessage attributeMessage_enum, MPlug& plug, MPlug& otherPlug, void* clientData)
{
    using T = typename wrappers::member_function_class<decltype(MemberFunctionPtr)>::type;
    auto target = reinterpret_cast<T*>(clientData);
    if (target) (target->*MemberFunctionPtr)(attributeMessage_enum, plug, otherPlug);
}

template <auto MemberFunctionPtr>
void MAttrPlugFunction_wrapper(MNodeMessage::AttributeMessage attributeMessage_enum, MPlug& plug,void* clientData)
{
    using T = typename wrappers::member_function_class<decltype(MemberFunctionPtr)>::type;
    auto target = reinterpret_cast<T*>(clientData);
    if (target) (target->*MemberFunctionPtr)(attributeMessage_enum, plug);
}
