#include <stdio.h>
#include <string.h>


#include "conf.h"
#include "pm.h"

static FILE *pm_backing_store;
static FILE *pm_log;
static char pm_memory[PHYSICAL_MEMORY_SIZE];
static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;

// Initialise la mémoire physique
void pm_init (FILE *backing_store, FILE *log)
{
    pm_backing_store = backing_store;
    pm_log = log;
    memset (pm_memory, '\0', sizeof (pm_memory));
}

// Charge la page demandée du backing store
void pm_download_page (unsigned int page_number, unsigned int frame_number)
{
    char buffer[PAGE_FRAME_SIZE + 1];
    memset(buffer, '\0', PAGE_FRAME_SIZE + 1);
    
    if (fseek(pm_backing_store, page_number * PAGE_FRAME_SIZE, SEEK_SET))
    {
        printf("ERREUR: fseek a échoué dans la fonction pm_download_page");
        return;
    }
    
    if (fread(buffer, PAGE_FRAME_SIZE, 1, pm_backing_store) < 1)
    {
        printf("ERREUR: fread a échoué dans la fonction pm_download_page");
        return;
    }
    
    strncpy(&pm_memory[frame_number * PAGE_FRAME_SIZE], 
            buffer, 
            PAGE_FRAME_SIZE);
    
    download_count++;
}

// Sauvegarde la frame spécifiée dans la page du backing store
void pm_backup_frame (unsigned int frame_number, unsigned int page_number)
{
    char buffer[PAGE_FRAME_SIZE + 1];
    memset(buffer, '\0', PAGE_FRAME_SIZE + 1);
    
    if (fseek(pm_backing_store, page_number * PAGE_FRAME_SIZE, SEEK_SET))
    {
        printf("ERREUR: fseek a échoué dans la fonction pm_backup_frame");
        return;
    }
    
    strncpy(buffer, 
            &pm_memory[frame_number * PAGE_FRAME_SIZE], 
            PAGE_FRAME_SIZE);
    
    fputs(buffer, pm_backing_store);
    backup_count++;
}

char pm_read (unsigned int physical_address)
{
    if (physical_address < PHYSICAL_MEMORY_SIZE && physical_address >= 0) {
        read_count++;
        return pm_memory[physical_address];
    } else {
        printf("ERREUR: tentative de lecture à une adresse physique invalide");
        return '!';
    }
}

void pm_write (unsigned int physical_address, char c)
{
    if (physical_address < PHYSICAL_MEMORY_SIZE && physical_address >= 0) {
        write_count++;
        pm_memory[physical_address] = c;
    } else {
        printf("ERREUR: tentative d'écriture à une adresse physique invalide");
    }
}


void pm_clean (void)
{
    // Enregistre l'état de la mémoire physique.
    if (pm_log)
    {
        for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++)
	{
            if (i % 80 == 0)
                fprintf (pm_log, "%c\n", pm_memory[i]);
            else
                fprintf (pm_log, "%c", pm_memory[i]);
	}
    }
    fprintf (stdout, "Page downloads: %2u\n", download_count);
    fprintf (stdout, "Page backups  : %2u\n", backup_count);
    fprintf (stdout, "PM reads : %4u\n", read_count);
    fprintf (stdout, "PM writes: %4u\n", write_count);
}
