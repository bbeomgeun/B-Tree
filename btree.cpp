// 12161104 박범근 데이터베이스 b+tree 구현 과제

#include <iostream>
#include <stdio.h>

using namespace std;

class BTree {
public:
	BTree(const char* fileName, int blockSize) {

	}
	bool insert(int key, int rid) {

	}
	void print() {
	
	}
	int* pointSearch(int key) {
	}
	int* rangeSearch(int startRange, int endRange) {
	}
};
// Test
int main(int argc, char* argv[])
{
	char command = argv[1][0];
	BTree myBtree = new BTree(any parameter);

	switch (command)
	{
	case 'c':
		// create index file
		break;
	case 'i':
		// insert records from [records data file], ex) records.txt
		break;
	case 's':
		// search keys in [input file] and print results to [output file]
		break;
	case 'r':
		// search keys in [input file] and print results to [output file]
		break;
	case 'p':
		// print B+-Tree structure to [output file]
		break;
	}
