#include "system.h"

#include "application.h"
#include "board.h"

bool system_init(void)
{
    /*
     * Composition root:
     * initialize lower layers first, then services, then application.
     */
    if (!board_init())
    {
        return false;
    }

    application_init();
    return true;
}
