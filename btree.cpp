// 12161104 박범근 데이터베이스 b+tree 구현 과제
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

enum class Header { // 헤더 변수를 위한 enum class(mode용)
	blockSize,
	rootBID,
	depth
};

class DataEntry {
public:
	int key;
	int value;

	DataEntry(int _key, int _value) { // 리프노드 만들면서 key value 넣기
		key = _key;
		value = _value;
	}
};

class IndexEntry {
public:
	int key;
	int BIDpointer; // key의 오른쪽 BID 포인터 key <= x 

	IndexEntry(int _key, int _BIDpointer) { 
		BIDpointer = _BIDpointer;
		key = _key;
	}
};

class LeafNode { // 리프노드는 key, value와 nextBID로 구성
	public:
		int nextLeafNode; // 다음 리프 노드 가리키는 포인터
		vector<DataEntry*> dataEntries;
		int location; // 노드에 현재 어디까지 데이터가 저장되어 있는지

	LeafNode() {
		nextLeafNode = 0;
		location = 0; // 4단위 byte형식
	}
};

class NonLeafNode { // non리프노드는 pointer와 key로 구성, key n개, 포인터 n+1개
public:
	int BIDpointer; // 가장 왼쪽 포인터
	vector<IndexEntry*> indexEntries;
	int location; // 노드에 현재 어디까지 데이터가 저장되어 있는지

	NonLeafNode() {
		BIDpointer = 0;
		location = 0; // 4단위 byte 형식
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
	} // 파일이름을 초기화하고, blockSize를 항상 읽어온다.
		// 파일이 없다면 getHeader는 -1리턴

	int getHeader(Header mode) { // 헤더 읽어오는 함수 + 모드와 함께
		FILE* tempFile = fopen(this->btreeFileName, "rb"); // read binary
		int block[3]; // buffer
		fread(block, sizeof(int), 3, tempFile); // block buffer에 int 사이즈로 3개 데이터 읽어오기
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

	void setHeader(int rootBID, int depth) { // blockSize는 안바뀜, 헤더의 rootBID와 depth 변경 
		pFile = fopen(this->btreeFileName, "rw"); // write binary
		fseek(pFile, 4, SEEK_SET);
		fwrite(&rootBID, sizeof(int), 1, pFile);
		fwrite(&depth, sizeof(int), 1, pFile);
		fclose(pFile);
	}

	void insertData(int blockID, int location, int key, int value) { // writeFile -> key와 value만 받으면 해당 block에 입력해주는 함수
		pFile = fopen(this->btreeFileName, "rw"); // write binary
		fseek(pFile, getBlockOffset(blockID) + location, SEEK_SET); // + 블럭시작부터가 아니라 데이터가 없는 곳부터 넣어야할듯.
		fwrite(&key, sizeof(int), 1, pFile);
		fwrite(&value, sizeof(int), 1, pFile);
		fclose(pFile);
	}

	LeafNode*  getLeafNode(int blockID) { // leaf block 전체를 긁어오는 작업
		FILE* tempFile = fopen(this->btreeFileName, "rb");
		vector<int> buffer;
		int blockLocation = getBlockOffset(blockID);
		fseek(tempFile, blockLocation, SEEK_SET); // 파일 처음부터 blockLocation 만큼 가서 찾기
		fread(&buffer, sizeof(int), getNumberPerNode(), tempFile); // 버퍼에 최대 갯수만큼 가져오기
		LeafNode* leafNode = new LeafNode(); // 리프노드 담을 메모리
		for (int i = 0; i < buffer.size(); i += 2) {  // 0이 아닌 데이터만 insert한다.
			if (buffer[i] == 0) {
				leafNode->location = i*4; // i * 2 / 8, 0번이면 처음부터, 1번이면 +8byte (i는 2씩 올라간다)
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

	NonLeafNode*  getNonLeafNode(int blockID) { // non leaf block 전체를 긁어오는 작업
		FILE* tempFile = fopen(this->btreeFileName, "rb");
		vector<int> buffer;
		int blockLocation = getBlockOffset(blockID);
		fseek(tempFile, blockLocation, SEEK_SET); // 파일 처음부터 blockLocation 만큼 가서 찾기
		fread(&buffer, sizeof(int), getNumberPerNode(), tempFile); // 버퍼에 최대 갯수만큼 가져오기
		NonLeafNode* nonLeafNode = new NonLeafNode(); // 리프노드 담을 메모리
		nonLeafNode->BIDpointer = buffer.front();
		for (int i = 1; i < buffer.size(); i += 2) { // 0이 아닌 데이터만 insert한다.
			if (buffer[i] == 0) {
				nonLeafNode->location = i*4; // 4+(i - 1) / 2 * 8, 1부터 버퍼시작이라 1 빼고(4 +) 한쌍을 위해 2로 나누고 8바이트
				break;
			}
			else {
				IndexEntry* indexEntry = new IndexEntry(buffer[i], buffer[i + 1]);
				nonLeafNode->indexEntries.push_back(indexEntry);
			}
		}
		return nonLeafNode;
	}

	void makeNewNode() { // 파일 끝에 블럭사이즈만큼 0으로 추가한다
		pFile = fopen(this->btreeFileName, "ab"); // append binary (파일 끝에서)
		int param = 0;
		fwrite(&param, sizeof(int), getNumberPerNode(), pFile); // blockSize(byte)만큼 0으로 채운다
		fclose(pFile);
	}

	int getNumberPerNode() {
		return floor((this->blockSize-4)/8); // 8byte로 갯수 세기
	}

	int getBlockOffset(int blockID) { // 해당 blockID를 통해 entry위치로
		return 12 + ((blockID - 1) * this->blockSize); // 헤더 12byte + blockSize ex) 1번블럭은 12byte부터 시작
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

	void creation(string fileName, int blockSize) { // 헤더 write (blockSize, rootBID = 1, Depth = 0)
		pFile = fopen(this->btreeFileName, "wb");
		int rootBID = 0; // 초기 root null
		int rootDepth = 0; // depth 0
		// fwrite(blcokSize 시작주소, 데이터의 단위 크기 byte로, 저장할 데이터 개수, 파일 포인터)
		fwrite(&blockSize, sizeof(int), 1, pFile);
		fwrite(&rootBID, sizeof(int), 1, pFile);
		fwrite(&rootDepth, sizeof(int), 1, pFile);
		// 순서대로 write
		fclose(pFile);
	}

	void insert(int key, int value) {
		if (getHeader(Header::rootBID) == 0) { // 빈 Tree면 root와 루트블럭 초기화
			setHeader(1, 0);
			makeNewNode();
		}
		int tempRootBID = getHeader(Header::rootBID);
		LeafNode* tempLeafNode = getLeafNode(tempRootBID); // 해당 노드를 찾아서 LeafNode에 저장(dataEntry + nextBID)
		if (isLeafSplit(tempLeafNode)) {

		}
		else {
			insertData(tempRootBID, tempLeafNode->location, key, value);
		}
	}
	// search는 헤더에서 루트bid 가져와서 루트부터 내려가자
	// Node * nowNode = getNode(rootBID);
	// nowNode의 pointer들 key들을 받아서 searchKey보다 큰 key를 만나면 해당 pointer타고 내려간다
	// 1 3 5 7이고 searchKey가 4라면 5에서 멈추고 5의 pointer(3 <= x < 5)를 통해 내려감
	// 리프까지 가고 리프노드에서도 역시 sequential하게 찾는다.

	int searchBlock(int searchKey) { // 트리를 타고 내려가면서 데이터가 있는 리프노드까지 타고 간다. 루트가 리프라면 루트ID 리턴
		int curNodeID = getHeader(Header::rootBID);
		int treeDepth = getHeader(Header::depth);
		int tempDepth = 0;
		while (treeDepth != treeDepth) { // depth가 같을때까지
			NonLeafNode* tempNonLeaf = getNonLeafNode(curNodeID); // 루트부터 읽으면서 내려온다.
			for (int i = 1; i < tempNonLeaf->indexEntries.size(); i++) { // 블럭의 entry를 sequential scan하면서
				int tempBID = tempNonLeaf->indexEntries[i - 1]->BIDpointer; // 왼쪽 포인터 저장
				int tempKey = tempNonLeaf->indexEntries[i]->key; // 현재 key값보다 searchKey가 더 크면 다음key본다
				if (searchKey < tempKey) { // key가 현재 key보다 작으면 왼쪽 포인터로 내려간다.
					curNodeID = tempBID;
					tempDepth++;
				}
			}
		}
		return curNodeID;
	}

	pair<int, int> pointSearch(int searchKey) {
		// 단일 블럭 search 후 pointSearch
		int curNodeID = searchBlock(searchKey);
		LeafNode* tempLeaf = getLeafNode(curNodeID); // depth끝 == 리프노드
		for (int i = 0; i < tempLeaf->dataEntries.size(); i++) {
			int tempLeafKey = tempLeaf->dataEntries[i]->key;
			int tempLeafValue = tempLeaf->dataEntries[i]->value;
			if (tempLeafKey == searchKey) {
				return make_pair(tempLeafKey, tempLeafValue);
			}
		}
		return make_pair(-1, -1);
	}

	vector<pair<int, int>> rangeSearch(int startRange, int endRange) { // 1 3 5 7 9에서 5~8 -> 5 7
		// 다음 블럭까지 가는 경우를 생각
		int curNodeID = searchBlock(startRange);
		vector<pair<int, int>> rangeResult;
		LeafNode* tempLeaf = getLeafNode(curNodeID); 
		while (1) { // 다음 블럭을 가기 위해 반복문
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
			tempLeaf = getLeafNode(tempLeaf->nextLeafNode); // 연결된 오른쪽 리프 노드로 이동
		}
		return rangeResult;
	}

	void print() { // Level 0(루트)와 level 1을 출력, depth 비교해가면서 leaf인지 nonleaf인지
		// 레벨1이 Leaf노드면 그냥 nextBID 타고 쭉 출력하면 되는데 nonLeaf면 루트 노드의 nextLevelBID 타고 재귀적으로 수행
		int tempRoot = getHeader(Header::rootBID);
		int treeDepth = getHeader(Header::depth);
		if (treeDepth == 0) {
			LeafNode* leafNode = getLeafNode(tempRoot);
			cout << "<0>" << "\n";
			for (int i = 0; i < leafNode->dataEntries.size(); i++) {
				int tempKey = leafNode->dataEntries[i]->key;
				cout<< tempKey << " ";
			}// 레벨 0 leaf key 파일 출력 & 레벨1은 존재하지 않음
		}

		else { // depth가 0이 아닌 경우
			NonLeafNode* rootNode = getNonLeafNode(tempRoot); // 루트는 nonLeaf
			cout << "<0>" << "\n";
			for (int i = 0; i < rootNode->indexEntries.size(); i++) {
				int tempKey = rootNode->indexEntries[i]->key;
				cout << tempKey << " ";
			} // 레벨 0 non leaf key 파일 출력
			cout << "\n";

			if (treeDepth > 1) { // 레벨 1도 nonLeaf이다. -> 이 경우 루트노드에서 BID 보면서 출력해야한다.
				NonLeafNode* firstNonLeaf = getNonLeafNode(rootNode->BIDpointer);
				for (int i = 0; i < firstNonLeaf->indexEntries.size(); i++) {
					int tempKey = firstNonLeaf->indexEntries[i]->key;
					cout << tempKey << " ";
				} // 제일 첫번째 레벨 1
				for (int i = 0; i < rootNode->indexEntries.size(); i++) { // 루트에서 타고 들어가기
					int nextLevelBID = rootNode->indexEntries[i]->BIDpointer;
					NonLeafNode* tempNonLeaf = getNonLeafNode(nextLevelBID);

					for (int i = 0; i < tempNonLeaf->indexEntries.size(); i++) {
						int tempKey = tempNonLeaf->indexEntries[i]->key;
						cout << tempKey << " ";
					}
				}
			}
			else if (treeDepth == 1) { // 레벨1은 leafNode
				LeafNode* leafNode = getLeafNode(rootNode->BIDpointer); // 제일 왼쪽 리프 노드부터
				cout << "<1>" << "\n";
				while (leafNode->nextLeafNode != 0) { // 다음 리프노드가 없을 때까지
					for (int i = 0; i < leafNode->dataEntries.size(); i++) {
						int tempKey = leafNode->dataEntries[i]->key;
						cout<< tempKey<<" ";
					}
					leafNode = getLeafNode(leafNode->nextLeafNode);
				}// 레벨 1 leaf key 파일 출력
			}
		}
	}
};

	vector<int> fileOpen(const char* filename, string mode) {
		// mode에 따라서 fileOpen, insert/range search는 key value / key key고, point search는 key 하나
		vector<int> fileData;
		FILE* readFile = fopen(filename, "r");           //읽을 목적의 파일 선언
		if (readFile != NULL)    //파일이 열렸는지 확인
		{
			char line[255]; // 임시 buffer 역할
			while (fgets(line, sizeof(line), readFile) != NULL) { // 한 줄씩 버퍼에 읽어온다
				line[strlen(line) - 1] = '\0'; // 개행 문자 제거
				char* context = NULL; // 토큰용 버퍼
				if (line[0] == '\0') // 읽어온게 없다면 종료
					break;
				if (mode == "s") { // point search는 토큰화 필요없다
					fileData.push_back(stoi(line));
				}
				else { // 나머지는 값이 두개이므로 나눠주기
					char* ptr = strtok_s(line, ", ", &context); // [, ]를 기준으로 나눠주기
					while (ptr != NULL) {
						fileData.push_back(stoi((string)ptr));
						ptr = strtok_s(NULL, ", ", &context);
					}
				}
			} // 최종적으로 1차원 벡터에 저장됨
			fclose(readFile);    //파일 닫기
		}
		return fileData; // 벡터 리턴
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
		BTree* myBtree = new BTree(binaryFileName); //argument를 통해 index btree파일로 btree 생성
		vector<int> inputData;

		switch (command)
		{
		case 'c':
			// create index file
			blockSize = atoi(argv[3]);
			myBtree->creation(binaryFileName, blockSize); // creation은 creation을 통해 헤더 초기화
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
