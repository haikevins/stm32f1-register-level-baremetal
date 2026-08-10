#ifndef SERVICE_CONFIG_H
#define SERVICE_CONFIG_H

/*
 * W25Q64 internal program/erase completion is polled through Status
 * Register-1. These are bounded poll counts rather than SysTick timeouts,
 * so initialization remains safe while global interrupts are still disabled.
 */
#define W25Q64_READY_POLL_LIMIT          (10000UL)
#define W25Q64_PAGE_PROGRAM_POLL_LIMIT   (100000UL)
#define W25Q64_SECTOR_ERASE_POLL_LIMIT   (1000000UL)

#endif
