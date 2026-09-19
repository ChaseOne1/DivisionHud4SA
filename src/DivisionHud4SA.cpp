#include "plugin.h"
#include "CMessages.h"

using namespace plugin;

class DivisionHud4SA
{
public:
    DivisionHud4SA()
    {
        Events::initGameEvent += [] {
            CMessages::AddMessageJumpQ("DivisionHud4SA loaded", 2000, 0);
        };
    }
} divisionHud4SA;
