#define TOOL_NAME               "countchar"
#define TOOL_SHORT_NAME         "countchar"
#define TOOL_VERSION            "0.0.1"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

char testo[4096];

int primo = 33; // non conteggiare lo spazio! ascii 32
int ultimo = 126;
int indice = 0;
int occorrenze[94] = { 0 }; // da 32 a 126

int main (int argc, char *argv[])
{
    if ( argv[1])  strcpy(testo , argv[1]);
    else {
        TraceLog(LOG_INFO, "Nessun testo da analizzare: verrà usato un testo standard di esempio!.");
        strcpy(testo,"Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem. Nulla consequat massa quis enim. Donec pede justo, fringilla vel, aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, imperdiet a, venenatis vitae, justo. Nullam dictum felis eu pede mollis pretium. Integer tincidunt. Cras dapibus. ");
    }
  
  long int lunghezza = strlen(testo);
  
  printf("\nLunghezza testo da analizzare %li caratteri (spazi inclusi!)\n",lunghezza);
  // printf("%s\n", testo);
  printf("\nNumero di occorrenze per ogni lettera: (spazi non inclusi!)\n");

  // loop

    while (indice < lunghezza) {
    
        // Find the ASCII value of a character using typecasting
        int asciiValue = (int)testo[indice];
    
            for (int i=primo; i<=ultimo; i++) {
                
                    if (i == asciiValue)  {
                      occorrenze[i]++;
                      break;
                  }
            }
        indice++;
    }
    
    int max=0;
    int car=0;
    // stampa risultati
    for (int i = primo; i<=ultimo; i++) { 
     if (occorrenze[i] != 0) { // produce output se ce almeno 1 lettera
        // trova il valore massimo
        if (occorrenze[i]>max) max=occorrenze[i],car=i;
        
       printf("lettera [ %c ] presente %2i volte (%06.2f %%) : ", i, occorrenze[i],(float)((occorrenze[i]*100.00f)/lunghezza)); 
          for (int c = 0;  c < occorrenze[i];  c++, putchar('#'));
        putchar('\n');
      }
    }
  printf("\nlettera più presente : %c, per %i volte!\n(altre lettere potrebbero essere presenti con lo stesso numero di occorrenze)\n",car,max);
  return 0;
}

