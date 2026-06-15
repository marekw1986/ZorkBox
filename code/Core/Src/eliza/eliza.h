#ifndef ELIZA_H
#define ELIZA_H

/* Call once at startup (or to restart the session). */
void eliza_init(void);

/* Call every main-loop iteration alongside vga_handle(). */
void eliza_handle(void);

#endif /* ELIZA_H */
