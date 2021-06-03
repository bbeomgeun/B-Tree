// 12161104 �ڹ��� �����ͺ��̽� b+tree ���� ����

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
		//��� write (blockSize, rootBID = 1, Depth = 0)
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
	fstream readFile;             //���� ������ ���� ����
	readFile.open(filename);    //���� ����
	if (readFile.is_open())    //������ ���ȴ��� Ȯ��
	{
		while (!readFile.eof())    //���� ������ �о����� Ȯ��
		{
			char* context = NULL;
			string tempLine ="";
			getline(readFile, tempLine);    //���پ� �о����
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
	readFile.close();    //���� �ݱ�
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
	// insert �� ������� blockSize�� ��Ʈ �о���� 

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
		// ���� �̸��� ���� ������ key, id �о insert ����
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
		// ���� �̸��� ���� ������ key�� search, ��� result�� �Է�
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
		// ���� �̸��� ���� ������ key�� search, ��� result�� �Է�
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
