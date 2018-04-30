#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;

//Tableau de frames alloues dans la pm
static int frame_entries[NUM_FRAMES]; 

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

void frame_init()
{
  for (int i = 0; i < NUM_FRAMES; i++)
    frame_entries[i] = -1;
}

//Pousser les frames dans la liste pour pouvoir rajouter
//un nouveau frame selon l'algorithme fifo
void push_frame_entries(int i)
{
    for (int j=i; j > 0; j--)
    {
      frame_entries[j+1] = frame_entries[j];
    }
}

//Alloue un frame si il y a le besoin de load un frame du backstore
int give_frame()
{
  for(int i=0; i < NUM_FRAMES; i++){
    //Si il y a un frame non utilise, on l'alloue
    if(frame_entries[i] < 0){
      return i;
    }
  }
  //Sinon on alloue le dernier frame dans la liste car c'est le moins utilise
  return (NUM_FRAMES-1);
}

//Update la table des frames pour pouvoir utiliser l'algorithme fifo
//dans le choix des frames a utiliser dans la memoire physique
void update_frame_entries(int frame_number){
  
  for (int i=0; i < NUM_FRAMES; i++){
    //Si la frame etait deja dans la liste, on fait juste
    //la ramener a l'avant de la liste en poussant le reste de la liste
    if(frame_entries[i] == frame_number){
      push_frame_entries(i-1);
      goto pushnewframe;
    }
  }
  
  //Sinon, on pousse toute la liste
  push_frame_entries(NUM_FRAMES-1);
  pushnewframe: 
    frame_entries[0] = frame_number;
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c = '!';
  //les 8 bits les moins significatifs
  unsigned int page = ((laddress >> 8) & 511); 
  //les 8 bits les plus significatifs
  unsigned int offset = ((laddress >> 0) & 511);
  
  unsigned int paddress = (page * PAGE_FRAME_SIZE) + offset;
  
  read_count++;
  int frame_number = tlb_lookup(page, 0);

  if (frame_number < 0)
  {
    printf("Not found in tlb");
    frame_number = pt_lookup(page);
    
    if (frame_number < 0) 
    {
      printf("Not found in page table");
      
      //Si la page demandee nest pas dans le tlb ni la table de pages
      //Il faut la chercher dans le back_store et lui assigner un frame_number
      //pour pouvoir la stocker dans la memoire physique
      frame_number = give_frame();
      pm_download_page(page, frame_number);
    }    
  }
  
  update_frame_entries(frame_number);
  
  c = pm_read(paddress);
  tlb_add_entry(page, frame_number, 0);
  
  vmm_log_command (stdout, "READING", laddress, page, frame_number, offset, paddress, c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  
  //les 8 bits les moins significatifs
  unsigned int page = ((laddress >> 8) & 511); 
  //les 8 bits les plus significatifs
  unsigned int offset = ((laddress >> 0) & 511);
  unsigned int paddress = (page * PAGE_FRAME_SIZE) + offset;
  
  /*-----Meme processus que dans vmm_ride pour trouver/allouer un frame----*/
  int frame_number = tlb_lookup(page, 0);
  if (frame_number < 0)
  {
    printf("Not found in tlb");
    frame_number = pt_lookup(page);
    
    if (frame_number < 0) 
    {
      printf("Not found in page table");
      frame_number = give_frame();
    }    
  }
  update_frame_entries(frame_number);
  /*-----------------------------------------------------------------------*/
  
  pm_write(paddress, c);
  pt_set_entry(page, frame_number);
  tlb_add_entry(page, frame_number, 1);
  
  /*faire un backup de la page car c'est une ecriture*/
  pm_backup_page(frame_number, page);
  
  vmm_log_command (stdout, "WRITING", laddress, page, frame_number, offset, paddress, c);
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
