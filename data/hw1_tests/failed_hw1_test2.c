#include <stdio.h>

void main(){
    int i;
    int j;

    for(i=0;i<2;i++){
        for(j=2;j>0;j--){
            printf("%d\n",i);
            printf("%d\n",j);
        }
    }

    i=0;
    while(i<2){
        j=2;
        while(j>0){
            printf("%d\n",i);
            printf("%d\n",j);
            j--;
        }
        i++;
    }

    i=0;
    do{
        j=2;
        do{
            printf("%d\n",i);
            printf("%d\n",j);
            j--;
        }while(j>0);
        i++;
    }while(i<2);

    return;
}
