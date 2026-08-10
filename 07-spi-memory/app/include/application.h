#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>
#include <stdint.h>

extern volatile uint8_t application_memory_manufacturer_id;
extern volatile uint8_t application_memory_type_id;
extern volatile uint8_t application_memory_capacity_id;

extern volatile uint32_t application_memory_test_address;
extern volatile uint32_t application_memory_test_sequence;
extern volatile uint32_t application_memory_error_count;

extern volatile bool application_memory_erase_ok;
extern volatile bool application_memory_program_ok;
extern volatile bool application_memory_verify_ok;
extern volatile bool application_memory_test_passed;

extern volatile uint8_t application_memory_first_mismatch_index;
extern volatile uint8_t application_memory_readback_first_byte;
extern volatile uint8_t application_memory_readback_last_byte;

bool application_init(void);
void application_process(void);

#endif
