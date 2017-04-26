#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "tlb.h"

#include "conf.h"

struct tlb_entry
{
    unsigned int page_number;
    int frame_number;             /* Invalide si négatif.  */
    bool readonly : 1;
};

static FILE *tlb_log = NULL;
static struct tlb_entry tlb_entries[TLB_NUM_ENTRIES]; 

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;

static unsigned int tlb_refs[TLB_NUM_ENTRIES] = {0};

/* Initialise le TLB, et indique où envoyer le log des accès.  */
void tlb_init (FILE *log)
{
    for (int i = 0; i < TLB_NUM_ENTRIES; i++)
        tlb_entries[i].frame_number = -1;
    tlb_log = log;
}

/******************** ¡ NE RIEN CHANGER CI-DESSUS !  ******************/

/* Recherche dans le TLB.
 * Renvoie le `frame_number`, si trouvé, ou un nombre négatif sinon.  */
static int tlb__lookup (unsigned int page_number, bool write)
{
    int frame = -1;
    int i = 0;
    
    while (i < TLB_NUM_ENTRIES)
    {
        tlb_refs[i] >>= 1;  // Fait 'vieillir' l'entrée.
        
        if (tlb_entries[i].page_number == page_number &&
                tlb_entries[i].frame_number != -1)
        {
            frame = tlb_entries[i].frame_number;
            tlb_entries[i].readonly &= !write;  // Si write, set readonly à faux, sinon ne fait rien.
            tlb_refs[i++] |= 0x8000;            // Indique un accès récent pour la politique de remplacement.
            break;
        }
        
        i++;
    }
    
    // Fait vieillir les entrées qui n'ont pas encore vieilli.
    while (i < TLB_NUM_ENTRIES){
        tlb_refs[i++] >>= 1;
    }
    
    return frame;
}

/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
        unsigned int frame_number, bool readonly)
{
    int victim = 0;
    
    for (int i = 0; i < TLB_NUM_ENTRIES; i++)
    {
        if (tlb_refs[i] < tlb_refs[victim]) {
            victim = i;
        }
        
        tlb_refs[i] >>= 1;  // Fait 'vieillir' l'entrée.
    }
    
    tlb_refs[victim] = 0x8000;   // Valeur initiale pour la politique LRU.
    tlb_entries[victim].page_number = page_number;
    tlb_entries[victim].frame_number = frame_number;
    tlb_entries[victim].readonly = readonly;
}

/******************** ¡ NE RIEN CHANGER CI-DESSOUS !  ******************/

void tlb_add_entry (unsigned int page_number,
        unsigned int frame_number, bool readonly)
{
    tlb_mod_count++;
    tlb__add_entry (page_number, frame_number, readonly);
}

int tlb_lookup (unsigned int page_number, bool write)
{
    int fn = tlb__lookup (page_number, write);
    (*(fn < 0 ? &tlb_miss_count : &tlb_hit_count))++;
    return fn;
}

/* Imprime un sommaires des accès.  */
void tlb_clean (void)
{
    fprintf (stdout, "TLB misses   : %3u\n", tlb_miss_count);
    fprintf (stdout, "TLB hits     : %3u\n", tlb_hit_count);
    fprintf (stdout, "TLB changes  : %3u\n", tlb_mod_count);
    fprintf (stdout, "TLB miss rate: %.1f%%\n",
            100 * tlb_miss_count
            /* Ajoute 0.01 pour éviter la division par 0.  */
            / (0.01 + tlb_hit_count + tlb_miss_count));
}
