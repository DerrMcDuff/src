
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
      return tlb_entries[i].frame_number;
    }
  }
  return -1;
}

<<<<<<< HEAD
static void tlb__push_entries()
{
    for (int i=0; i < TLB_NUM_ENTRIES-1; i++)
    {
      tlb_entries[i] = tlb_entries[i+1];
    }
}

=======

/*Tableau pour stocker les donnees parcourues dans le log*/
static struct tlb_entry log_last_entries[TLB_NUM_ENTRIES-1];

static struct tlb_entry parse_line(char* line){
  
  int page_number, frame_number;
  sscanf(line, "%d %d", &page_number, &frame_number);
  
  /*Utilisation de la structure de tlb_entry pour stocker la valeur de 
  la page et du frame qui sont dans le log*/
  struct tlb_entry t;
  t.page_number = page_number;
  t.frame_number = frame_number;
  return t;
}

static int fifo_algo();
static int fifo_algo(){

  /*D'abord on regarde si il y a une case vide dans le tlb*/
    for (int i = 0; i < TLB_NUM_ENTRIES; i++){
      if(tlb_entries[i].frame_number == -1)
        return i;
    }
    /*A regarder dans le log pour trouver le entry le moins utilise*/
    
    for (int i = 0; i < TLB_NUM_ENTRIES-1; i++)
      log_last_entries[i].page_number = -1;
    
    struct tlb_entry t;
    
    if(tlb_log != NULL){
      
      /*Parcours d'une ligne a la fois dans le fichier log*/
      char line[128];
      while (fgets(line, sizeof(line), tlb_log)){
        
        t = parse_line(line);
        
        /*parcours du tableau ou on store les dernieres entry du log*/
        for (int i=0; i < TLB_NUM_ENTRIES-1; i++){
          
          //verifier si la case dans le tableau contient deja cet element
          if(log_last_entries[i].page_number == t.page_number &&
             log_last_entries[i].frame_number == t.frame_number) {
              goto next_line;
          }
          else if(log_last_entries[i].page_number == -1) {
            log_last_entries[i] = t;
            goto next_line;
          }
        }
        
        stop_reading: break;
        next_line: continue;//continue serves no purpose here except aesthetics
      }
    }
    
    for (int i=0; i< TLB_NUM_ENTRIES; i++){
      if(tlb_entries[i].page_number == t.page_number &&
          tlb_entries[i].frame_number == t.frame_number){
        return i;
      }
    }
    
    return NULL;
}


>>>>>>> 22e3bac5ca7032b4c84b385d6e288ef0ba9d19fc
/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
                            unsigned int frame_number, bool readonly)
{
<<<<<<< HEAD
  struct tlb_entry new_entry;
  new_entry.page_number = page_number;
  new_entry.frame_number = frame_number;
  new_entry.readonly = readonly;
  tlb__push_entries();
  tlb_entries[0] = new_entry;
=======
  /*Algo Fifo*/
  int new_entry = fifo_algo();
  tlb_entries[new_entry].page_number = page_number;
  tlb_entries[new_entry].frame_number = frame_number;
  tlb_entries[new_entry].readonly = readonly;
  fprintf(tlb_log, 
          "Page:%d Frame:%d Action:%d \n", 
          page_number, frame_number, readonly);
>>>>>>> 22e3bac5ca7032b4c84b385d6e288ef0ba9d19fc
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
