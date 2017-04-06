#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;

void vmm_init (FILE *log)
{
    // Initialise le fichier de journal.
    vmm_log = log;
}


// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
        unsigned int laddress, /* Logical address. */
        unsigned int page,
        unsigned int frame,
        unsigned int offset,
        unsigned int paddress, /* Physical address.  */
        char c) /* Caractère lu ou écrit.  */
{
    if (out)
        fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
                page, offset, frame, paddress);
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
    char c = '!';
    int frame = -1;
    int paddress = -1;
    
    int page = laddress >> 8;       // 8 most significant bits
    int offset = laddress & 0xFF;   // 8 least significant bits
    
    
    if ((frame = tlb_lookup(page, false)) < 0)
        frame = pt_lookup(page);
    
    if (frame < 0) {
        srand(time(NULL));
        int index = rand() % NUM_FRAMES;
        
        
    } else {
        paddress = (frame << 8) + offset;
        pm_read(paddress);
    }
    
    tlb_add_entry(page, frame, false);
    
    vmm_log_command (
            stdout, "READING", laddress, page, frame, offset, paddress, c);
    
    read_count++;
    return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
    int frame = -1;
    int paddress = -1;
    
    int page = laddress >> 8;       // 8 most significant bits
    int offset = laddress & 0xFF;   // 8 least significant bits
    
    if ((frame = tlb_lookup(page, true)) < 0)
        frame = pt_lookup(page);
    
    if (frame < 0) {
        printf("ERREUR: Tentative d'écriture à une adresse invalide\n");
    } else {
        paddress = (frame << 8) + offset;
        pm_write(paddress, c);
    }
    
    tlb_add_entry(page, frame, false);
    
    vmm_log_command (
            stdout, "WRITING", laddress, page, frame, offset, paddress, c);
    
    write_count++;
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
    fprintf (stdout, "VM reads : %4u\n", read_count);
    fprintf (stdout, "VM writes: %4u\n", write_count);
}
