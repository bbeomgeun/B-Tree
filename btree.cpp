// 12161104 �ڹ��� �����ͺ��̽� b+tree ���� ����
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <cstring>
#include <queue>

using namespace std;

enum class Header { // ��� ������ ���� enum class(mode��)
	blockSize,
	rootBID,
	depth
};

class BTree {
public:
	const char * btreeFileName;
	int blockSize;
	FILE* pFile;
	
	BTree(const char* binaryFile) { 
		this->btreeFileName = binaryFile;
		blockSize = getHeader(this->btreeFileName, Header::blockSize);
		cout << blockSize;
		cout << getHeader(this->btreeFileName, Header::rootBID);
		cout << getHeader(this->btreeFileName, Header::depth);
	} // �����̸��� �ʱ�ȭ�ϰ�, blockSize�� �׻� �о�´�.
	 // ������ ���ٸ� getHeader�� -1����

	int getHeader(const char* fileName, Header mode) { // ��� �о���� �Լ� + ���� �Բ�
		FILE* tempFile = fopen(fileName, "rb"); // read binary
		int block[3]; // buffer
		fread(block, sizeof(int), 3, tempFile); // block buffer�� int ������� 3�� ������ �о����
		if (tempFile != NULL) {
			if (mode == Header::blockSize) {
				return block[0];
			}
			else if (mode == Header::rootBID) {
				return block[1];
			}
			else if (mode == Header::depth) {
				return block[2];
			}
			fclose(tempFile);
		}
	}

	void creation(string fileName, int blockSize) { // ��� write (blockSize, rootBID = 1, Depth = 0)
		pFile = fopen(this->btreeFileName, "wb");
		int rootBID = 0; // �ʱ� root null
		int rootDepth = 0; // depth 0
		// fwrite(blcokSize �����ּ�, �������� ���� ũ�� byte��, ������ ������ ����, ���� ������)
		fwrite(&blockSize, sizeof(int),1, pFile); 
		fwrite(&rootBID, sizeof(int),1, pFile);
		fwrite(&rootDepth, sizeof(int), 1, pFile);
		// ������� write
		fclose(pFile);
	}

	bool insert(int key, int rid) {
		// 
		

	}
	int* pointSearch(int searchKey) {
		// search�� ������� ��Ʈbid �����ͼ� ��Ʈ���� ��������
		// Node * nowNode = getNode(rootBID);
		// nowNode�� pointer�� key���� �޾Ƽ� searchKey���� ū key�� ������ �ش� pointerŸ�� ��������
		// 1 3 5 7�̰� searchKey�� 4��� 5���� ���߰� 5�� pointer(3 <= x < 5)�� ���� ������
		// �������� ���� ������忡���� ���� sequential�ϰ� ã�´�.

	}
	int* rangeSearch(int startRange, int endRange) {


	}
	void print() {
	
	}

	//Node* getNode(int blockID) { // block ��ü�� �ܾ���� �۾�
	//	FILE* tempFile = fopen(this->btreeFileName, "rb");
	//	int blockLocation = 12 + (blockID - 1) * 8; // ��� 12byte + �� ���� 8byte ex) 1������ 12byte���� ����
	//	fseek(tempFile, blockLocation, SEEK_SET); // ���� ó������ blockLocation ��ŭ ���� ã��
	// fread( int�� ����, sizeof(int), ����, ����������)
	// cout<<ftell(tempFile) // ���� Ŀ���� ��ġ�� ������ �� ó���������� �����ش�.
	// fseek(tempFile, 4, 0); // ó�� ��ġ���� 4byte ���� ��ġ�� �̵�
	// fgets(block, 4, tempFile); // 4~8byte rootBID

	//	// block�� ������ �������� 4byte��ŭ ������ 0���� ������ ������ ������
	//	// getNode�� ���� blockID�� �ش��ϴ� ���(��)�� �� �о pointer(���� blockID ����Ű��)
	//	// key���� �����ؼ� ������ ��ġ �°� insert����
	//	// ���� 1 4 6�ִµ� value�� 5�̸� 4�� 6�����̹Ƿ� 6�� pointer�� ����Ű�� bid�� �������� �̵�
	//	// �׷� �� �� bid�� �̿��ؼ� getNode�ϰ� ��������, (leaf�� �����ʹ� �ٸ��Ÿ� ����Ű��
	//	//�װɷ� �������� Ȯ�� �Ǵ� ����� depth �����س�ɷ� ��) �������鼭 �ش� bid�� stack�� �����ҰŴϱ� stack size�� ���ص� �ɵ�
	//	// �̷��� �޸𸮻� Ʈ���� ���� split������ ó���ϰ� �ٽ� disk�� write�ϰ�
	//	// ���������� split�Ͼ�� ����� �����ְ�(fseek����)
	//}
};

vector<int> fileOpen(const char * filename, string mode) { 
	// mode�� ���� fileOpen, insert/range search�� key value / key key��, point search�� key �ϳ�
	vector<int> fileData;
	FILE* readFile = fopen(filename, "r");           //���� ������ ���� ����
	if (readFile != NULL)    //������ ���ȴ��� Ȯ��
	{
		char line[255]; // �ӽ� buffer ����
		while (fgets(line, sizeof(line), readFile) != NULL) { // �� �پ� ���ۿ� �о�´�
			line[strlen(line) - 1] = '\0'; // ���� ���� ����
			char* context = NULL; // ��ū�� ����
			if (line[0] == '\0') // �о�°� ���ٸ� ����
				break;
			if (mode == "s") { // point search�� ��ūȭ �ʿ����
				fileData.push_back(stoi(line));
			}
			else { // �������� ���� �ΰ��̹Ƿ� �����ֱ�
				char* ptr = strtok_s(line, ", ", &context); // [, ]�� �������� �����ֱ�
				while (ptr != NULL) {
					fileData.push_back(stoi((string)ptr));
					ptr = strtok_s(NULL, ", ", &context);
				}
			}
		} // ���������� 1���� ���Ϳ� �����
		fclose(readFile);    //���� �ݱ�
	}
	return fileData; // ���� ����
}

// test
// creation : c Btree.bin 36
// inseart : i Btree.bin sample_insertion_input.txt
// point search : s Btree.bin sample_search_input.txt result.txt
// range search : r Btree.bin sample_range_search.txt result.txt
// print : p Btree.bin result.txt
int main(int argc, char* argv[]){

	char command = argv[1][0];
	const char* binaryFileName = argv[2];
	int blockSize = 0;
	const char * insertFileName = "";
	const char* searchFileName = "";
	const char* resultFileName = "";
	BTree* myBtree = new BTree(binaryFileName); //argument�� ���� index btree���Ϸ� btree ����
	vector<int> inputData;

	switch (command)
	{
	case 'c':
		// create index file
		blockSize = atoi(argv[3]);
		myBtree->creation(binaryFileName, blockSize); // creation�� creation�� ���� ��� �ʱ�ȭ
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
