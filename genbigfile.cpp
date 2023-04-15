#include<iostream>
#include <fstream>
#include <time.h>
using namespace std;


int main(){
    srand(time(0));
    ofstream bigfile("BF");
    for(long i=0; i<320000;i++){
        bigfile<<rand()%15000000;
    }
    bigfile.close();
}