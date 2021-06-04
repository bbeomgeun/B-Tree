// 12161104 박범근 데이터베이스 b+tree 구현 과제
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <cstring>

using namespace std;

class Node {
public:
	int key;
	int value;
};

class BTree {
public:
	const char * btreeFile;
	int blockSize;
	FILE* pFile;
	
	BTree(const char* binaryFile) {
		this->btreeFile = binaryFile;
		blockSize = 0;
		char block[10];
		pFile = fopen(this->btreeFile, "rb");
		if (pFile != NULL) {
			fgets(block, 4, pFile);
			blockSize = atoi(block); // ascii
			fclose(pFile);
		}
	} // 파일이름을 초기화하고, 블럭사이즈를 항상 읽어온다.

	void creation(string fileName, int blockSize) { // 헤더 write (blockSize, rootBID = 1, Depth = 0)
		pFile = fopen(this->btreeFile, "wb");
		int rootBID = 0; // 초기 root null
		int rootDepth = 0; // depth 0
		fwrite(reinterpret_cast<char*>(&blockSize), sizeof(int),1, pFile);
		fwrite(reinterpret_cast<char*>(&rootBID), sizeof(int),1, pFile);
		fwrite(reinterpret_cast<char*>(&rootDepth), sizeof(int), 1, pFile);
		fclose(pFile);
	}

	bool insert(int key, int rid) {
		

	}
	int* pointSearch(int key) {


	}
	int* rangeSearch(int startRange, int endRange) {


	}
	void print() {
	
	}
};

vector<int> fileOpen(const char * filename, string mode) {
	vector<int> fileData;
	FILE* readFile = fopen(filename, "r");           //읽을 목적의 파일 선언
	if (readFile != NULL)    //파일이 열렸는지 확인
	{
		char line[255];
		while (fgets(line, sizeof(line), readFile) != NULL) {
			line[strlen(line) - 1] = '\0';
			char* context = NULL;
			if (line[0] == '\0')
				break;
			if (mode == "s") {
				fileData.push_back(stoi(line));
			}
			else {
				char* ptr = strtok_s(line, ", ", &context);
				while (ptr != NULL) {
					fileData.push_back(stoi((string)ptr));
					ptr = strtok_s(NULL, ", ", &context);
				}
			}
		}
		fclose(readFile);    //파일 닫기
	}
	return fileData;
}
// Test
int main(int argc, char* argv[]){

	char command = argv[1][0];
	const char* binaryFileName = argv[2];
	int blockSize = 0;
	const char * insertFileName = "";
	const char* searchFileName = "";
	const char* resultFileName = "";
	BTree* myBtree = new BTree(binaryFileName);
	vector<int> inputData;

	switch (command)
	{
	case 'c':
		// create index file
		blockSize = atoi(argv[3]);
		myBtree->creation(binaryFileName, blockSize);
		break;
	case 'i':
		// insert records from [records data file], ex) records.txt
		insertFileName = argv[3];
		inputData = fileOpen(insertFileName, "i");
		for (int i = 0; i < inputData.size(); i+=2) {
			//myBtree->insert(inputData[i],inputData[i + 1]);
		}
		break;
	case 's':
		// search keys in [input file] and print results to [output file]
		searchFileName = argv[3];
		resultFileName = argv[4];
		inputData = fileOpen(searchFileName, "s");
		for (int i = 0; i < inputData.size(); i ++) {
			//myBtree->pointSearch(inputData[i]);
		}
		break;
	case 'r':
		// search keys in [input file] and print results to [output file]
		searchFileName = argv[3];
		resultFileName = argv[4];
		inputData = fileOpen(searchFileName, "r");
		for (int i = 0; i < inputData.size(); i += 2) {
			//myBtree->rangeSearch(inputData[i], inputData[i + 1]);
		}
		break;
	case 'p':
		// print B+-Tree structure to [output file]
		resultFileName = argv[3];
		//myBtree->print();
		break;
	}

}
