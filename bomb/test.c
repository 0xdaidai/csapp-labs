#include<stdio.h>

int main(){
    int b = 14, c = 0;
    int t = (b - c) >> 31;
    int l = ((b - c) + t) >> 1;
    b = l + c;
    printf("%d\n", b);
}