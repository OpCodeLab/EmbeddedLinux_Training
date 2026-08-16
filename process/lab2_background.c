#include <stdio.h>
#include <unistd.h>

int main() 
{
    int count = 0;
    
    while(1)
     {
        printf("Program running... count: %d\n", count);
        count++;
        sleep(5);  // Wait 5 seconds
        
        // Optional: exit after 100 iterations to prevent infinite loop
        if(count >= 100) {
            printf("Program finished after 100 iterations\n");
            break;
        }
    }
    
    return 0;
}