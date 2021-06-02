// 12161104 박범근 데이터베이스 b+tree 구현 과제

#include <iostream>
#include <fstream>
#include <stdio.h>

using namespace std;

class BTree {
public:
	BTree(const char *fileName, int blockSize) {

	}

	void creation(const char* fileName, int blockSize) {
		ofstream binaryFile("btree.bin", ios::out | ios::binary);
		//헤더 write (blockSize, rootBID = 1, Depth = 0)
		binaryFile.write(reinterpret_cast<char*>(&blockSize), sizeof(int));
		binaryFile.write(reinterpret_cast<char*>(&blockSize), sizeof(int));
		binaryFile.write(reinterpret_cast<char*>(&blockSize), sizeof(int));
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
	BTree * myBtree = new BTree();

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
