#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
enum { SIZE = 5 };
int main(void)
{
    double a[SIZE] = {1, 2, 3, 4, 5};
    FILE *f1 = fopen("server_files/client1/a.txt","wb");
    assert(f1);
    int count = 0;
    do {
      size_t r1 = fwrite(a + count, sizeof a[0], 1, f1);
      printf("wrote %zu elements out of %d requested\n", r1,  1);
      count++;
    } while (count < 5);
    fclose(f1);

    double b[SIZE];
    FILE *f2 = fopen("server_files/client1/a.txt","rb");
    size_t r2;
    count =0;
    do {
      r2 = fread(b+ count, sizeof b[0], 1, f2);
      count++;
    } while (count < 5);
    fclose(f2);
    printf("read back: ");
    for(size_t i = 0; i < SIZE; i++)
        printf("%0.2f ", b[i]);
}

