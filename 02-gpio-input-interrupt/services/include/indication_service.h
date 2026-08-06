#ifndef INDICATION_SERVICE_H
#define INDICATION_SERVICE_H

#include <stdbool.h>

typedef enum
{
    INDICATION_STATUS = 0
} indication_id_t;

void indication_service_init(void);
void indication_service_set(indication_id_t indication, bool enabled);
void indication_service_toggle(indication_id_t indication);

#endif
