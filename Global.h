#pragma once

#include <tuple>

//I worked so hard to find this number
constexpr int DOCK_STATE_CHANGED_EVENT = 65528;

//FWD
class MObject;

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
