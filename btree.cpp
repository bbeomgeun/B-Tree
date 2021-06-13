// 12161104 �ڹ��� �����ͺ��̽� b+tree ���� ����
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <cstring>
#include <stack>
#include <algorithm>
#include <cmath>

using namespace std;

enum class Header { // ��� ������ ���� enum class(mode��)
	blockSize,
	rootBID,
	depth
};

class DataEntry {
public:
	int key;
	int value;

	DataEntry(int _key, int _value) { // ������� ����鼭 key value �ֱ�
		key = _key;
		value = _value;
	}
};

class IndexEntry {
public:
	int key;
	int BIDpointer; // key�� ������ BID ������ key <= x 

	IndexEntry(int _key, int _BIDpointer) { 
		BIDpointer = _BIDpointer;
		key = _key;
	}
};

class LeafNode { // �������� key, value�� nextBID�� ����
	public:
		int nextLeafNode; // ���� ���� ��� ����Ű�� ������
		vector<DataEntry*> dataEntries;
		int location; // ��忡 ���� ������ �����Ͱ� ����Ǿ� �ִ���

	LeafNode() {
		nextLeafNode = 0;
		location = 0; // 4���� byte����
	}
};

class NonLeafNode { // non�������� pointer�� key�� ����, key n��, ������ n+1��
public:
	int BIDpointer; // ���� ���� ������
	vector<IndexEntry*> indexEntries;
	int location; // ��忡 ���� ������ �����Ͱ� ����Ǿ� �ִ���

	NonLeafNode() {
		BIDpointer = 0;
		location = 0; // 4���� byte ����
	}
};

class BTree {
public:
	const char* btreeFileName;
	int blockSize;
	FILE* pFile;

	BTree(const char* binaryFile) {
		this->btreeFileName = binaryFile;
		this->blockSize = getHeader(Header::blockSize);
	} // �����̸��� �ʱ�ȭ�ϰ�, blockSize�� �׻� �о�´�.
		// ������ ���ٸ� getHeader�� -1����

	int getHeader(Header mode) { // ��� �о���� �Լ� + ���� �Բ�
		FILE* tempFile = fopen(this->btreeFileName, "rb"); // read binary
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

	void setHeader(int rootBID, int depth) { // blockSize�� �ȹٲ�, ����� rootBID�� depth ���� 
		pFile = fopen(this->btreeFileName, "rw"); // write binary
		fseek(pFile, 4, SEEK_SET);
		fwrite(&rootBID, sizeof(int), 1, pFile);
		fwrite(&depth, sizeof(int), 1, pFile);
		fclose(pFile);
	}

	void insertData(int blockID, int location, int key, int value) { // writeFile -> key�� value�� ������ �ش� block�� �Է����ִ� �Լ�
		pFile = fopen(this->btreeFileName, "rw"); // write binary
		fseek(pFile, getBlockOffset(blockID) + location, SEEK_SET); // + �����ۺ��Ͱ� �ƴ϶� �����Ͱ� ���� ������ �־���ҵ�.
		fwrite(&key, sizeof(int), 1, pFile);
		fwrite(&value, sizeof(int), 1, pFile);
		fclose(pFile);
	}

	LeafNode*  getLeafNode(int blockID) { // leaf block ��ü�� �ܾ���� �۾�
		FILE* tempFile = fopen(this->btreeFileName, "rb");
		vector<int> buffer;
		int blockLocation = getBlockOffset(blockID);
		fseek(tempFile, blockLocation, SEEK_SET); // ���� ó������ blockLocation ��ŭ ���� ã��
		fread(&buffer, sizeof(int), getNumberPerNode(), tempFile); // ���ۿ� �ִ� ������ŭ ��������
		LeafNode* leafNode = new LeafNode(); // ������� ���� �޸�
		for (int i = 0; i < buffer.size(); i += 2) {  // 0�� �ƴ� �����͸� insert�Ѵ�.
			if (buffer[i] == 0) {
				leafNode->location = i*4; // i * 2 / 8, 0���̸� ó������, 1���̸� +8byte (i�� 2�� �ö󰣴�)
				break;
			}
			else{
				DataEntry* dataEntry = new DataEntry(buffer[i], buffer[i + 1]);
				leafNode->dataEntries.push_back(dataEntry);
			}
		}
		leafNode->nextLeafNode = buffer.back();

		return leafNode;
	}

	NonLeafNode*  getNonLeafNode(int blockID) { // non leaf block ��ü�� �ܾ���� �۾�
		FILE* tempFile = fopen(this->btreeFileName, "rb");
		vector<int> buffer;
		int blockLocation = getBlockOffset(blockID);
		fseek(tempFile, blockLocation, SEEK_SET); // ���� ó������ blockLocation ��ŭ ���� ã��
		fread(&buffer, sizeof(int), getNumberPerNode(), tempFile); // ���ۿ� �ִ� ������ŭ ��������
		NonLeafNode* nonLeafNode = new NonLeafNode(); // ������� ���� �޸�
		nonLeafNode->BIDpointer = buffer.front();
		for (int i = 1; i < buffer.size(); i += 2) { // 0�� �ƴ� �����͸� insert�Ѵ�.
			if (buffer[i] == 0) {
				nonLeafNode->location = i*4; // 4+(i - 1) / 2 * 8, 1���� ���۽����̶� 1 ����(4 +) �ѽ��� ���� 2�� ������ 8����Ʈ
				break;
			}
			else {
				IndexEntry* indexEntry = new IndexEntry(buffer[i], buffer[i + 1]);
				nonLeafNode->indexEntries.push_back(indexEntry);
			}
		}
		return nonLeafNode;
	}

	void makeNewNode() { // ���� ���� �������ŭ 0���� �߰��Ѵ�
		pFile = fopen(this->btreeFileName, "ab"); // append binary (���� ������)
		int param = 0;
		fwrite(&param, sizeof(int), getNumberPerNode(), pFile); // blockSize(byte)��ŭ 0���� ä���
		fclose(pFile);
	}

	int getNumberPerNode() {
		return floor((this->blockSize-4)/8); // 8byte�� ���� ����
	}

	int getBlockOffset(int blockID) { // �ش� blockID�� ���� entry��ġ��
		return 12 + ((blockID - 1) * this->blockSize); // ��� 12byte + blockSize ex) 1������ 12byte���� ����
	}

	bool isLeafSplit(LeafNode * temp) {
		return temp->dataEntries.size() > getNumberPerNode();
	}
	bool isNonLeafSplit(NonLeafNode * temp) {
		return temp->indexEntries.size() > getNumberPerNode();
	}
	void leafSplit() {
	}
	void nonLeafSplit() {
	}

	void creation(string fileName, int blockSize) { // ��� write (blockSize, rootBID = 1, Depth = 0)
		pFile = fopen(this->btreeFileName, "wb");
		int rootBID = 0; // �ʱ� root null
		int rootDepth = 0; // depth 0
		// fwrite(blcokSize �����ּ�, �������� ���� ũ�� byte��, ������ ������ ����, ���� ������)
		fwrite(&blockSize, sizeof(int), 1, pFile);
		fwrite(&rootBID, sizeof(int), 1, pFile);
		fwrite(&rootDepth, sizeof(int), 1, pFile);
		// ������� write
		fclose(pFile);
	}

	void insert(int key, int value) {
		if (getHeader(Header::rootBID) == 0) { // �� Tree�� root�� ��Ʈ�� �ʱ�ȭ
			setHeader(1, 0);
			makeNewNode();
		}
		int tempRootBID = getHeader(Header::rootBID);
		LeafNode* tempLeafNode = getLeafNode(tempRootBID); // �ش� ��带 ã�Ƽ� LeafNode�� ����(dataEntry + nextBID)
		if (isLeafSplit(tempLeafNode)) {

		}
		else {
			insertData(tempRootBID, tempLeafNode->location, key, value);
		}
	}
	// search�� ������� ��Ʈbid �����ͼ� ��Ʈ���� ��������
	// Node * nowNode = getNode(rootBID);
	// nowNode�� pointer�� key���� �޾Ƽ� searchKey���� ū key�� ������ �ش� pointerŸ�� ��������
	// 1 3 5 7�̰� searchKey�� 4��� 5���� ���߰� 5�� pointer(3 <= x < 5)�� ���� ������
	// �������� ���� ������忡���� ���� sequential�ϰ� ã�´�.

	int searchBlock(int searchKey) { // Ʈ���� Ÿ�� �������鼭 �����Ͱ� �ִ� ���������� Ÿ�� ����. ��Ʈ�� ������� ��ƮID ����
		int curNodeID = getHeader(Header::rootBID);
		int treeDepth = getHeader(Header::depth);
		int tempDepth = 0;
		while (treeDepth != treeDepth) { // depth�� ����������
			NonLeafNode* tempNonLeaf = getNonLeafNode(curNodeID); // ��Ʈ���� �����鼭 �����´�.
			for (int i = 1; i < tempNonLeaf->indexEntries.size(); i++) { // ���� entry�� sequential scan�ϸ鼭
				int tempBID = tempNonLeaf->indexEntries[i - 1]->BIDpointer; // ���� ������ ����
				int tempKey = tempNonLeaf->indexEntries[i]->key; // ���� key������ searchKey�� �� ũ�� ����key����
				if (searchKey < tempKey) { // key�� ���� key���� ������ ���� �����ͷ� ��������.
					curNodeID = tempBID;
					tempDepth++;
				}
			}
		}
		return curNodeID;
	}

	pair<int, int> pointSearch(int searchKey) {
		// ���� �� search �� pointSearch
		int curNodeID = searchBlock(searchKey);
		LeafNode* tempLeaf = getLeafNode(curNodeID); // depth�� == �������
		for (int i = 0; i < tempLeaf->dataEntries.size(); i++) {
			int tempLeafKey = tempLeaf->dataEntries[i]->key;
			int tempLeafValue = tempLeaf->dataEntries[i]->value;
			if (tempLeafKey == searchKey) {
				return make_pair(tempLeafKey, tempLeafValue);
			}
		}
		return make_pair(-1, -1);
	}

	vector<pair<int, int>> rangeSearch(int startRange, int endRange) { // 1 3 5 7 9���� 5~8 -> 5 7
		// ���� ������ ���� ��츦 ����
		int curNodeID = searchBlock(startRange);
		vector<pair<int, int>> rangeResult;
		LeafNode* tempLeaf = getLeafNode(curNodeID); 
		while (1) { // ���� ���� ���� ���� �ݺ���
			for (int i = 0; i < tempLeaf->dataEntries.size(); i++) {
				int tempLeafKey = tempLeaf->dataEntries[i]->key;
				int tempLeafValue = tempLeaf->dataEntries[i]->value;
				if (tempLeafKey >= startRange && tempLeafKey <= endRange) {
					rangeResult.push_back(make_pair(tempLeafKey, tempLeafValue));
				}
				else if (tempLeafKey > endRange) {
					return rangeResult;
				}
			}
			tempLeaf = getLeafNode(tempLeaf->nextLeafNode); // ����� ������ ���� ���� �̵�
		}
		return rangeResult;
	}

	void print() { // Level 0(��Ʈ)�� level 1�� ���, depth ���ذ��鼭 leaf���� nonleaf����
		// ����1�� Leaf���� �׳� nextBID Ÿ�� �� ����ϸ� �Ǵµ� nonLeaf�� ��Ʈ ����� nextLevelBID Ÿ�� ��������� ����
		int tempRoot = getHeader(Header::rootBID);
		int treeDepth = getHeader(Header::depth);
		if (treeDepth == 0) {
			LeafNode* leafNode = getLeafNode(tempRoot);
			cout << "<0>" << "\n";
			for (int i = 0; i < leafNode->dataEntries.size(); i++) {
				int tempKey = leafNode->dataEntries[i]->key;
				cout<< tempKey << " ";
			}// ���� 0 leaf key ���� ��� & ����1�� �������� ����
		}

		else { // depth�� 0�� �ƴ� ���
			NonLeafNode* rootNode = getNonLeafNode(tempRoot); // ��Ʈ�� nonLeaf
			cout << "<0>" << "\n";
			for (int i = 0; i < rootNode->indexEntries.size(); i++) {
				int tempKey = rootNode->indexEntries[i]->key;
				cout << tempKey << " ";
			} // ���� 0 non leaf key ���� ���
			cout << "\n";

			if (treeDepth > 1) { // ���� 1�� nonLeaf�̴�. -> �� ��� ��Ʈ��忡�� BID ���鼭 ����ؾ��Ѵ�.
				NonLeafNode* firstNonLeaf = getNonLeafNode(rootNode->BIDpointer);
				for (int i = 0; i < firstNonLeaf->indexEntries.size(); i++) {
					int tempKey = firstNonLeaf->indexEntries[i]->key;
					cout << tempKey << " ";
				} // ���� ù��° ���� 1
				for (int i = 0; i < rootNode->indexEntries.size(); i++) { // ��Ʈ���� Ÿ�� ����
					int nextLevelBID = rootNode->indexEntries[i]->BIDpointer;
					NonLeafNode* tempNonLeaf = getNonLeafNode(nextLevelBID);

					for (int i = 0; i < tempNonLeaf->indexEntries.size(); i++) {
						int tempKey = tempNonLeaf->indexEntries[i]->key;
						cout << tempKey << " ";
					}
				}
			}
			else if (treeDepth == 1) { // ����1�� leafNode
				LeafNode* leafNode = getLeafNode(rootNode->BIDpointer); // ���� ���� ���� ������
				cout << "<1>" << "\n";
				while (leafNode->nextLeafNode != 0) { // ���� ������尡 ���� ������
					for (int i = 0; i < leafNode->dataEntries.size(); i++) {
						int tempKey = leafNode->dataEntries[i]->key;
						cout<< tempKey<<" ";
					}
					leafNode = getLeafNode(leafNode->nextLeafNode);
				}// ���� 1 leaf key ���� ���
			}
		}
	}
};

	vector<int> fileOpen(const char* filename, string mode) {
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
	int main(int argc, char* argv[]) {

		char command = argv[1][0];
		const char* binaryFileName = argv[2];
		int blockSize = 0;
		const char* insertFileName = "";
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
			for (int i = 0; i < inputData.size(); i += 2) {
				//myBtree->insert(inputData[i],inputData[i + 1]);
			}
			break;
		case 's':
			// search keys in [input file] and print results to [output file]
			searchFileName = argv[3];
			resultFileName = argv[4];
			inputData = fileOpen(searchFileName, "s");
			for (int i = 0; i < inputData.size(); i++) {
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
