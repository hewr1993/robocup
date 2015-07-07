#include<iostream>
#include<fstream>
#include<stdio.h>
using namespace std;

int copyfile(const char* source, const char* dest)
{
char ch;
ifstream from(source);

ofstream to(dest);
if(!from) return 1;
if(!to) return 2;
while(from.get(ch))
{
if(!to.put(ch)) return 3;
}
return 0;
}

int main()
{
	int t = copyfile("colors0.txt","../vision/colors0.txt");
	copyfile("colors1.txt","../vision/colors1.txt");
copyfile("colors0.txt","../net_viewer/colors0.txt");
copyfile("colors1.txt","../net_viewer/colors1.txt");
copyfile("colors0.txt","../colors0.txt");
copyfile("colors1.txt","../colors1.txt");
}

