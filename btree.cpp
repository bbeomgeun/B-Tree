// 12161104 박범근 데이터베이스 b+tree 구현 과제

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <string>
#include <cstring>

using namespace std;

class BTree {
public:
	BTree() {
		//const char *fileName, int blockSize
	}

	void creation(string fileName, int blockSize) {
		ofstream binaryFile(fileName, ios::out | ios::binary);
		//헤더 write (blockSize, rootBID = 1, Depth = 0)
		int rootBID = 1;
		int rootDepth = 0;
		binaryFile.write(reinterpret_cast<char*>(&blockSize), sizeof(int));
		binaryFile.write(reinterpret_cast<char*>(&rootBID), sizeof(int));
		binaryFile.write(reinterpret_cast<char*>(&rootDepth), sizeof(int));
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

vector<int> fileOpen(string filename, string mode) {
	vector<int> fileData;
	fstream readFile;             //읽을 목적의 파일 선언
	readFile.open(filename);    //파일 열기
	if (readFile.is_open())    //파일이 열렸는지 확인
	{
		while (!readFile.eof())    //파일 끝까지 읽었는지 확인
		{
			char* context = NULL;
			string tempLine ="";
			getline(readFile, tempLine);    //한줄씩 읽어오기
			if (tempLine == "")
				break;
			if (mode == "s") {
				fileData.push_back(stoi(tempLine));
			}
			else {
				auto c_string = tempLine.c_str();
				char* ptr = strtok_s((char*)c_string, ", ", &context );
				while (ptr != NULL) {
					fileData.push_back(stoi((string)ptr));
					ptr = strtok_s(NULL, ", ", &context);
				}
			}
		}
	}
	readFile.close();    //파일 닫기
	return fileData;
}
// Test
int main(int argc, char* argv[]){

	char command = argv[1][0];
	string binaryFileName = argv[2];
	int blockSize = 0;
	string insertFileName = "";
	string searchFileName = "";
	string resultFileName = "";
	BTree* myBtree = new BTree();
	vector<int> inputData;
	// insert 시 헤더에서 blockSize와 루트 읽어오고 

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
		// 파일 이름을 통해 파일의 key, id 읽어서 insert 실행
		inputData = fileOpen(insertFileName, "i");
		for (int i = 0; i < inputData.size(); i+=2) {
			cout << inputData[i] << " " << inputData[i + 1] << "\n";
		}
		//myBtree->insert();
		break;
	case 's':
		// search keys in [input file] and print results to [output file]
		searchFileName = argv[3];
		resultFileName = argv[4];
		// 파일 이름을 통해 파일의 key로 search, 결과 result에 입력
		inputData = fileOpen(searchFileName, "s");
		for (int i = 0; i < inputData.size(); i ++) {
			cout << inputData[i] << "\n";
		}
		//myBtree->pointSearch();
		break;
	case 'r':
		// search keys in [input file] and print results to [output file]
		searchFileName = argv[3];
		resultFileName = argv[4];
		// 파일 이름을 통해 파일의 key로 search, 결과 result에 입력
		inputData = fileOpen(searchFileName, "r");
		for (int i = 0; i < inputData.size(); i += 2) {
			cout << inputData[i] << " " << inputData[i + 1] << "\n";
		}
		//myBtree->rangeSearch();
		break;
	case 'p':
		// print B+-Tree structure to [output file]
		resultFileName = argv[3];
		//myBtree->print();
		break;
	}

}
