
#include "scope_guard.h"

namespace water{
namespace component{

ScopeGuard::ScopeGuard(std::function<void()> onExitScope_)
: onExitScope(onExitScope_), dismissed(false)
{   
}   

ScopeGuard::~ScopeGuard()
{   
    if(!dismissed)
    {   
        onExitScope();
    }   
}   

void ScopeGuard::dismiss()
{   
    dismissed = true;
}   

}}
