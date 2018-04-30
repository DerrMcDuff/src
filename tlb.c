
#include <stdint.h>
#include <stdio.h>

#include "tlb.h"

#include "conf.h"

struct tlb_entry
{
  unsigned int page_number;
  int frame_number; /* Invalide si négatif. On ne veut pas de ta négativité! */
  bool readonly : 1;
};

static FILE *tlb_log = NULL;
static struct tlb_entry tlb_entries[TLB_NUM_ENTRIES]; 

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;

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
  for (int i = 0; i < TLB_NUM_ENTRIES; i++) {
    if (tlb_entries[i].page_number == page_number) {
      tlb_hit_count = tlb_hit_count + 1;
      return tlb_entries[i].frame_number;
    }
  }
  tlb_miss_count = tlb_miss_count + 1;
  return -1;
}

static void tlb__push_entries()
{
  for (int i=0; i < TLB_NUM_ENTRIES-1; i++)
  {
    tlb_entries[i+1] = tlb_entries[i];
  }
}

/*Tableau pour stocker les donnees parcourues dans le log*/
//static struct tlb_entry log_last_entries[TLB_NUM_ENTRIES-1];


/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
                            unsigned int frame_number, bool readonly)
{


  struct tlb_entry new_entry;
  new_entry.page_number = page_number;
  new_entry.frame_number = frame_number;
  new_entry.readonly = readonly;
  tlb__push_entries();
  tlb_mod_count = tlb_mod_count + 1;
  //chaque fois qu'il y a une nouvelle entree
  //elle est placee en avant de la liste
  tlb_entries[0] = new_entry;

  fprintf(tlb_log, 
          "Page:%d Frame:%d Action:%d \n", 
          page_number, frame_number, readonly);

}


/******************** ¡ NE RIEN CHANGER CI-DESSOUS !  ******************/
/* D'accord */

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
