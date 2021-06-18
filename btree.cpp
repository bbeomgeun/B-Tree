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

bool compareLeaf(DataEntry* a, DataEntry* b) {
	return a->key < b->key;
}

bool compareNonLeaf(IndexEntry* a, IndexEntry* b) {
	return a->key < b->key;
}

class BTree {
public:
	FILE* pFile;
	const char* btreeFileName;
	int blockSize;
	int blockCount;

	BTree(const char* binaryFile) {
		this->btreeFileName = binaryFile;
		this->blockSize = getHeader(Header::blockSize);
		this->blockCount = getBlockCount();
	} // 파일이름을 초기화하고, blockSize를 항상 읽어온다.
		// 파일이 없다면 getHeader는 -1리턴

	int getBlockCount() {
		pFile = fopen(this->btreeFileName, "rb");
		if (pFile != NULL) {
			fseek(pFile, 0, SEEK_END);
			int fileSize = ftell(pFile);
			fclose(pFile);
			return (fileSize - 12) / this->blockSize; // 헤더사이즈빼고 블럭사이즈로 나누면 블럭 갯수가 나온다.
		}
		return -1;
	}

	int getHeader(Header mode) { // 헤더 읽어오는 함수 + 모드와 함께
		FILE* tempFile = fopen(this->btreeFileName, "rb"); // read binary
		int block[3]; // buffer
		if (tempFile != NULL) {
			fread(block, sizeof(int), 3, tempFile); // block buffer에 int 사이즈로 3개 데이터 읽어오기
			fclose(tempFile);
			return block[static_cast<int>(mode)]; // blockSize = 0, rootBID = 1, depth = 2
		}
		return -1;
	}

	void setHeader(int rootBID, int depth) { // blockSize는 안바뀜, 헤더의 rootBID와 depth 변경 
		pFile = fopen(this->btreeFileName, "r+b"); // write binary
		fseek(pFile, 4, SEEK_SET);
		fwrite(&rootBID, sizeof(int), 1, pFile);
		fwrite(&depth, sizeof(int), 1, pFile);
		fclose(pFile);
	}

	LeafNode*  getLeafNode(int blockID) { // leaf block 전체를 긁어오는 작업
		FILE* tempFile = fopen(this->btreeFileName, "rb");
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // 동적할당 및 0으로 초기화
		int blockLocation = getBlockOffset(blockID);
		fseek(tempFile, blockLocation, SEEK_SET); // 파일 처음부터 blockLocation 만큼 가서 찾기
		fread(bufferArray, sizeof(int), bufferSize, tempFile); // 버퍼에 최대 갯수만큼 가져오기
		bufferSize = getNumberPerNode(); // read를 하면서 bufferSize가 가져온 데이터 개수만큼 줄어들어서 다시 초기화
		LeafNode* leafNode = new LeafNode(); // 리프노드 담을 메모리
		for (int i = 0; i < bufferSize-1; i += 2) {  // 0이 아닌 데이터만 insert한다. 마지막 nextBID 제외하기 위해 size-1
			if (bufferArray[i] == 0) {
				leafNode->location = i*4; // i * 2 / 8, 0번이면 처음부터, 1번이면 +8byte (i는 2씩 올라간다)
				break;
			}
			else{
				DataEntry* dataEntry = new DataEntry(bufferArray[i], bufferArray[i + 1]);
				leafNode->dataEntries.push_back(dataEntry);
			}
		}
		leafNode->nextLeafNode = bufferArray[bufferSize-1]; // 제일 끝
		delete[] bufferArray;
		return leafNode;
	}

	NonLeafNode*  getNonLeafNode(int blockID) { // non leaf block 전체를 긁어오는 작업
		FILE* tempFile = fopen(this->btreeFileName, "rb");
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // 동적할당 및 0으로 초기화
		int blockLocation = getBlockOffset(blockID);
		fseek(tempFile, blockLocation, SEEK_SET); // 파일 처음부터 blockLocation 만큼 가서 찾기
		fread(bufferArray, sizeof(int), bufferSize, tempFile); // 버퍼에 최대 갯수만큼 가져오기
		bufferSize = getNumberPerNode(); // read를 하면서 bufferSize가 가져온 데이터 개수만큼 줄어들어서 다시 초기화
		NonLeafNode* nonLeafNode = new NonLeafNode(); // 리프노드 담을 메모리
		nonLeafNode->BIDpointer = bufferArray[0]; // 제일 먼저
		for (int i = 1; i < bufferSize; i += 2) { // 0이 아닌 데이터만 insert한다.
			if (bufferArray[i] == 0) {
				nonLeafNode->location = i*4; // 4+(i - 1) / 2 * 8, 1부터 버퍼시작이라 1 빼고(4 +) 한쌍을 위해 2로 나누고 8바이트
				break;
			}
			else {
				IndexEntry* indexEntry = new IndexEntry(bufferArray[i], bufferArray[i + 1]);
				nonLeafNode->indexEntries.push_back(indexEntry);
			}
		}
		delete[] bufferArray;
		return nonLeafNode;
	}

	void makeNewBlock(int blockID) { // 파일 끝에 블럭사이즈만큼 0으로 추가한다
		pFile = fopen(this->btreeFileName, "r+b"); // writeBinary
		int blockLocation = getBlockOffset(blockID);
		fseek(pFile, blockLocation, SEEK_SET); // 파일 처음부터 blockLocation 만큼 가서 찾기
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // 동적할당 및 0으로 초기화
		fwrite(bufferArray, sizeof(int), bufferSize, pFile); // blockSize(byte)만큼 0으로 채운다
		blockCount++;
		delete[] bufferArray;
		fclose(pFile);
	}

	int getNumberPerNode() { // getNode에서 fread시 sizeof(int)로 긁어올때 사용하는 갯수 -> 4byte로 계산해야한다.
		return floor((this->blockSize)/4); // 4byte로 갯수 세기 - 단순 blockSize를 int 갯수로 나눈 것
	}
	
	int getEntryPerNode() { // split여부 시 사용, Entry Size로 생각(8byte 단위로 insert해야하므로)
		return floor((this->blockSize - 4) / 8); // 8byte entry단위, leaf는 nextBID, nonLeaf는 nextLevelBID 4byte 제외
	}

	int getBlockOffset(int blockID) { // 해당 blockID를 통해 entry위치로
		return 12 + ((blockID - 1) * this->blockSize); // 헤더 12byte + blockSize ex) 1번블럭은 12byte부터 시작
	}

	bool isLeafSplit(LeafNode * temp) {
		return temp->dataEntries.size() > getEntryPerNode(); // 현재 갯수로 split이 일어나는지 체크
	}
	bool isNonLeafSplit(NonLeafNode * temp) {
		return temp->indexEntries.size() > getEntryPerNode(); // 1개 entry 삽입되면 split이 일어나는지 체크
	}
	IndexEntry*  leafSplit(int insertLocation, LeafNode * tempLeafNode) {
		int splitBlockID = this->blockCount + 1;
		LeafNode* splitLeafNode = new LeafNode(); // 새 splitNode
		splitLeafNode->nextLeafNode = tempLeafNode->nextLeafNode; // 기존 block이 가리키던 ID를 새 block에게
		makeNewBlock(splitBlockID); // 새 splitNode 생성
		tempLeafNode->nextLeafNode = splitBlockID; // 기존 노드에 nextLeaf 연결
		// nextBID 지정, splitNode에 반절 data 옮기기, 상위 노드에 pointer 추가하기
		int leftSize = tempLeafNode->dataEntries.size(); // 기존 노드 사이즈, 이미 추가된 상태라 (제한 + 1개 추가 entry)
		// 데이터 옮기는 과정
		for (int i = leftSize / 2; i < leftSize; i++) {
			splitLeafNode->dataEntries.push_back(tempLeafNode->dataEntries[i]);
		} // 반을 옮기고 기존은 반부터 끝까지 삭제한다.
		tempLeafNode->dataEntries.erase(tempLeafNode->dataEntries.begin() + leftSize / 2, tempLeafNode->dataEntries.end());
		updateLeafData(insertLocation, tempLeafNode); // 파일에 변경된 기존 노드 작성(내부에서 0으로 초기화되고 write)
		updateLeafData(splitBlockID, splitLeafNode); // 파일에 splitNode 작성(내부에서 0으로 초기화되고 write)
		//---------------------여기까지 리프노드 split -> blockCount만 늘어나고 root나 depth는 관련 X
		int newIndexKey = splitLeafNode->dataEntries.front()->key; // 포인터용 key + blockID는 splitBlockID 사용
		IndexEntry * newIndexEntry = new IndexEntry(newIndexKey, splitBlockID);

		return newIndexEntry;
	}

	IndexEntry*  nonLeafSplit(int tempUpBlockID, NonLeafNode * tempNonLeafNode) {
		int splitBlockID = this->blockCount + 1;
		makeNewBlock(splitBlockID);
		NonLeafNode* splitNonLeafNode = new NonLeafNode();
		int leftSize = tempNonLeafNode->indexEntries.size();
		IndexEntry* newIndexEntry = new IndexEntry(tempNonLeafNode->indexEntries[leftSize/2]->key, splitBlockID);
		//위로 올라갈 nonLeafEntry(오른쪽 첫번째 key값, 오른쪽 new blockID)를 먼저 빼준다 (오른쪽 노드에 들어가진 않으므로)
		// 추가로 오른쪽 새로 생긴 블럭의 blockID를 지정해줘야한다.
		splitNonLeafNode->BIDpointer = tempNonLeafNode->indexEntries[leftSize/2]->BIDpointer;
		//올라가는 entry가 가리키고 있던 하위레벨 blockID를 splitNode가 가리키고 올라가는 entry는 이 block을 가리킴
		for (int i = leftSize / 2+1; i < leftSize; i++) { // 올라가는 entry 제외하고 옮겨준다.
			splitNonLeafNode->indexEntries.push_back(tempNonLeafNode->indexEntries[i]);
		}
		tempNonLeafNode->indexEntries.erase(tempNonLeafNode->indexEntries.begin() + leftSize / 2, tempNonLeafNode->indexEntries.end());
		updateNonLeafData(tempUpBlockID, tempNonLeafNode);
		updateNonLeafData(splitBlockID, splitNonLeafNode);

		return newIndexEntry;
	}

	void updateLeafData(int blockID, LeafNode* _leafNode) {
		// writeFile -> 해당 블럭을 다 지우고, Entry에 해당하는 key value를 모두 다시 작성
		pFile = fopen(this->btreeFileName, "r+b"); // write binary
		fseek(pFile, getBlockOffset(blockID), SEEK_SET); // 정렬된 순서를 write해야하므로 매번 block처음부터 wirte해주자.
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // 동적할당 및 0으로 초기화
		fwrite(bufferArray, sizeof(int), bufferSize, pFile); // blockSize(byte)만큼 0으로 초기화
		delete[] bufferArray;
		fseek(pFile, getBlockOffset(blockID), SEEK_SET); // 다시 offset으로 가서 새로운 entry 추가 (현재 0으로 초기화)
		for (int i = 0; i < _leafNode->dataEntries.size(); i++) { // 새로운 entry가 추가 +정렬되어 있는 상황. 파일 업데이트
			int key = _leafNode->dataEntries[i]->key;
			int value = _leafNode->dataEntries[i]->value;
			fwrite(&key, sizeof(int), 1, pFile);
			fwrite(&value, sizeof(int), 1, pFile);
		}
		fseek(pFile, getBlockOffset(blockID + 1) - 4, SEEK_SET);
		fwrite(&_leafNode->nextLeafNode, sizeof(int), 1, pFile);
		fclose(pFile);
	}

	void updateNonLeafData(int blockID, NonLeafNode * _nonLeafNode) { // split시 이용(0으로 다 지우고 다시 write)
		pFile = fopen(this->btreeFileName, "r+b"); // write binary
		fseek(pFile, getBlockOffset(blockID), SEEK_SET); // 정렬된 순서를 write해야하므로 매번 block처음부터 wirte해주자.
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // 동적할당 및 0으로 초기화
		fwrite(bufferArray, sizeof(int), bufferSize, pFile); // blockSize(byte)만큼 0으로 초기화
		delete[] bufferArray;
		fseek(pFile, getBlockOffset(blockID), SEEK_SET); // 다시 offset으로 가서 다시 작성
		fwrite(&_nonLeafNode->BIDpointer, sizeof(int), 1, pFile); // nonLeafNode는 BIDPointer부터
		for (int i = 0; i < _nonLeafNode->indexEntries.size(); i++) { // 새로운 entry가 추가 +정렬되어 있는 상황. 파일 업데이트
			int key = _nonLeafNode->indexEntries[i]->key;
			int BIDpointer = _nonLeafNode->indexEntries[i]->BIDpointer;
			fwrite(&key, sizeof(int), 1, pFile);
			fwrite(&BIDpointer, sizeof(int), 1, pFile);
		}
		fclose(pFile);
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
		if (getHeader(Header::rootBID) == 0) { // 루트가 없는 경우는 setRoot(1) + block1생성
			int newRoot = 1;
			setHeader(newRoot, 0); // 루트 blockID 1로 update
			makeNewBlock(newRoot); // block #1 생성
		} // 여기까지 트리 첫 초기화 과정(아무 블럭도 없을 경우)
		 // 초기화해주고나서 삽입과정 시작
		stack<int> trackID;
		trackID = searchBlock(key); // 삽입해야할 block 찾기 및 찾아온 경로
		int insertLocation = trackID.top();
		trackID.pop();
		LeafNode* tempLeafNode = getLeafNode(insertLocation); // 해당 노드를 찾아서 LeafNode에 저장(dataEntry + nextBID)
		DataEntry* newDataEntry = new DataEntry(key, value);
		tempLeafNode->dataEntries.push_back(newDataEntry);
		sort(tempLeafNode->dataEntries.begin(), tempLeafNode->dataEntries.end(), compareLeaf);

		if (isLeafSplit(tempLeafNode)) { // 1개 추가한 후 blockSize를 넘는 경우, 정렬된 상태
			IndexEntry* newIndexEntry = leafSplit(insertLocation, tempLeafNode); // 리턴 : key랑 splitblockID
			// leaf Split = split용 블럭 생성, nextBlockBID 연결, 데이터 반씩 나누기, 파일에 기존노드, split노드 write, indexEntry 반환
			while(1){ // nonLeaf도 진행
				// case 1. 상위 nonLeafNode 없는 경우 생성해주고 삽입 후 종료
				if (trackID.empty()) { 
					int nonLeafAfterSplit = this->blockCount + 1; // 새 상위 노드용 blockID
					makeNewBlock(nonLeafAfterSplit); // 상위 노드 생성
					setHeader(nonLeafAfterSplit, getHeader(Header::depth) + 1); // 새로 생겼으므로 count로 rootBID, depth도 update
					NonLeafNode* nonLeafNode = new NonLeafNode();
					nonLeafNode->BIDpointer = insertLocation;
					nonLeafNode->indexEntries.push_back(newIndexEntry);
					updateNonLeafData(nonLeafAfterSplit, nonLeafNode); // BIDpointer랑 indexEntry 한꺼번에 write
					break; // 종료
				} // 여기까지 상위레벨 없는 경우 노드 생성, root,depth, BIDpointer와 새로운 indexEntry write까지
				//case 2. 상위 nonLeafNode 있는 경우  삽입 후 split 여부 판단
				else { // stack에 내 상위 레벨 노드 BID가 있는 경우 거기에 삽입을 진행한다. 계속 타고 올라가면서 split 발생 시 계속 반복
					int tempUpBlockID = trackID.top();
					trackID.pop();
									//int tempUpUpBlockID = 0;
									//if (!trackID.empty()) { // 두 레벨 위 노드의 존재 여부 확인용(있으면 split시 그곳에 insert, 없으면 new block)
									//	tempUpUpBlockID = trackID.top();
									//}
					NonLeafNode* tempNonLeafNode = getNonLeafNode(tempUpBlockID); // 상위노드 가져오기
					//IndexEntry* newIndexEntry = new IndexEntry(); // 새로 생긴 block의 첫 key 값과 blockID;
					// 일단 위에서 만든거 사용
					tempNonLeafNode->indexEntries.push_back(newIndexEntry); // 상위 노드에 index 추가
					sort(tempNonLeafNode->indexEntries.begin(), tempNonLeafNode->indexEntries.end(), compareNonLeaf);
					// split 진행하면서 안의 내용들만 바뀔뿐 -> 따라서 index를 key값에 맞춰서만 넣어주고 pointer는 자기가 가리키던거 계속
					if (isNonLeafSplit(tempNonLeafNode)) { // 현재 삽입 + 정렬상태 -> 크기가 맞지 않으면 split
						//split 발생 시 leaf처럼 똑같이 1/2 해주고 오른쪽 노드의 첫 번째 index entry를 위로 올려준다
						//nonLeaf만의 과정 진행 + 또 위에다가 insert하되 없으면 newBlock 만들기, 
						newIndexEntry = nonLeafSplit(tempUpBlockID, tempNonLeafNode); // 새로 올라가야하는 entry에 저장, write
						insertLocation = tempUpBlockID; // 바꿔줘야 상위 root 생성하고 root의 왼쪽 블럭이 올바르게 저장됨(split 되기 전 blockID)
						// while문 돌면서 비어있다면 case1 이후 종료, 아니라면 case2로 와서 상위 노드에 삽입, 계속 진행
					}
					else {// 아니면 삽입된 상위노드 write해주기
						updateNonLeafData(tempUpBlockID, tempNonLeafNode); // split 발생 안하면 insert한채로 파일 write
						break; // split 발생 안하므로 insert 종료
					}
				}
			}
		}
		else { // entry 1개 추가해도 split 발생하지 않는 경우 write해주고 종료
			updateLeafData(insertLocation, tempLeafNode);
			return;
		}
	}
	// search는 헤더에서 루트bid 가져와서 루트부터 내려가자
	// Node * nowNode = getNode(rootBID);
	// nowNode의 pointer들 key들을 받아서 searchKey보다 큰 key를 만나면 해당 pointer타고 내려간다
	// 1 3 5 7이고 searchKey가 4라면 5에서 멈추고 5의 pointer(3 <= x < 5)를 통해 내려감
	// 리프까지 가고 리프노드에서도 역시 sequential하게 찾는다.

	stack<int> searchBlock(int searchKey) { // 트리를 타고 내려가면서 데이터가 있는 리프노드까지 타고 간다. 루트가 리프라면 루트ID 리턴
		int curNodeID = getHeader(Header::rootBID);
		int treeDepth = getHeader(Header::depth);
		int tempDepth = 0;
		// 내려가는 blockID들 벡터나 스택에 저장해서 리턴하기, 
		stack<int> trackID;
		trackID.push(curNodeID);
		while (treeDepth != tempDepth) { // depth가 같을때까지
			NonLeafNode* tempNonLeaf = getNonLeafNode(curNodeID); // 루트부터 읽으면서 내려온다.
			int searchFlag = false;
			for (int i = 0; i < tempNonLeaf->indexEntries.size(); i++) { // 블럭의 entry를 sequential scan하면서
				int tempBID = 0;
				int tempKey = 0;
				if (i == 0) {
					tempBID = tempNonLeaf->BIDpointer;
					tempKey = tempNonLeaf->indexEntries.front()->key;
				}
				else {
					tempBID = tempNonLeaf->indexEntries[i - 1]->BIDpointer; // 왼쪽 포인터 저장
					tempKey = tempNonLeaf->indexEntries[i]->key; // 현재 key값보다 searchKey가 더 크면 다음key본다
				}
				if (searchKey < tempKey) { // key가 현재 key보다 작으면 왼쪽 포인터로 내려간다.
					curNodeID = tempBID;
					trackID.push(curNodeID);
					tempDepth++;
					searchFlag = true;
					break; // for문 종료하고 다음 노드로 넘어가자
				}
			}
			if (!searchFlag) {// 끝까지 못 찾은 경우 제일 마지막 blockID 넣어주자
				curNodeID = tempNonLeaf->indexEntries.back()->BIDpointer;
				trackID.push(curNodeID);
				tempDepth++;
			} // 다음 depth로 넘어가기
		}
		return trackID;
	}

	pair<int, int> pointSearch(int searchKey) {
		// 단일 블럭 search 후 pointSearch
		stack<int> trackID;
		trackID = searchBlock(searchKey);
		int curNodeID = trackID.top();
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
		stack<int> trackID;
		trackID = searchBlock(startRange);
		int curNodeID = trackID.top();
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
			myBtree->insert(inputData[i],inputData[i + 1]);
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
