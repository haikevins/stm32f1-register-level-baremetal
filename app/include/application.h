#ifndef APPLICATION_H
#define APPLICATION_H

/*
 * Public contract of the Application layer.
 *
 * Application may depend only on public Service APIs and portable Common types.
 */
void application_init(void);
void application_process(void);

#endif
