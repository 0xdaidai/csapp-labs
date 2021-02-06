#include<stdio.h>

int isTmax(int x) {
  // tmin + tmax + 1 == 0
  return !(~x ^ (x + 1))&x;
}

int main(){
    int x;
    while (1){
        scanf("%d",&x);
        printf("x = %d\nres: %d\n", x, isTmax(x));
    }
    return 0;
}